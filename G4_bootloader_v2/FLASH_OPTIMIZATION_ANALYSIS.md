# Анализ функции Flash() для STM32G474CBT6

## Вызов функции (строка 406)
```cpp
Flash(addr, (uint64_t*)(void*)bin_data, bin_data_length);
```

## Критические проблемы

### 1. ⚠️ БЛОКИРУЮЩИЕ ОПЕРАЦИИ В ПРЕРЫВАНИИ
**Проблема**: Функция использует `HAL_FLASH_Program()` и `FLASH_WaitForLastOperation()` - это блокирующие операции!

```cpp
// Строка 244
HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_address, current_data);
// Строка 249
FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
```

**Время выполнения**:
- Запись 1 DOUBLEWORD (8 байт): ~20-50 мкс
- Массив bin_data: 2048 байт = 256 DOUBLEWORD
- **Общее время: 256 × 50мкс = 12.8 мс в прерывании!**

**Последствия**:
- Блокировка других прерываний на 12+ мс
- Потеря данных UART
- Нарушение реального времени
- Возможные watchdog сбросы

### 2. 🐛 НЕЭФФЕКТИВНОЕ КОПИРОВАНИЕ ДАННЫХ

```cpp
// Строки 229-241: Побайтное копирование
uint8_t* dst = (uint8_t*)&current_data;
for (uint32_t j = 0; j < bytes_to_copy; j++) {
    dst[j] = src_bytes[i * 8 + j];
}
```

**Проблема**: Копирование по 1 байту вместо 8-байтового чтения
**Влияние**: 8× больше операций доступа к памяти

### 3. 🔴 МНОЖЕСТВЕННЫЕ БАРЬЕРЫ ПАМЯТИ

Барьеры `__DMB()` вызываются внутри цикла на каждой итерации:

```cpp
for (uint32_t i = 0; i < doublewords_count; i++) {
    HAL_FLASH_Program(...);
    __DMB();  // <-- Барьер на КАЖДОЙ итерации!
    FLASH_WaitForLastOperation(...);
}
```

**Проблема**: Избыточная синхронизация - достаточно барьера только после цикла

### 4. ⚠️ ПРИВЕДЕНИЕ ТИПОВ С VOLATILE

```cpp
// Строка 406
Flash(addr, (uint64_t*)(void*)bin_data, bin_data_length);
```

**Проблема**: Двойное приведение `(void*)` обходит volatile, компилятор может оптимизировать!

### 5. 🔴 ОТСУТСТВИЕ ТАЙМАУТА

Функция может зависнуть в `FLASH_WaitForLastOperation()` если Flash неисправна.

## Рекомендации

### А. Изменить архитектуру (КРИТИЧНО для прерывания)

**Вариант 1: Асинхронная запись через DMA**
- Не поддерживается напрямую STM32G4
- Требует альтернативной реализации

**Вариант 2: Запись вне прерывания** ⭐ РЕКОМЕНДУЕТСЯ
```cpp
// В прерывании только установить флаг
volatile bool flash_write_requested = false;

void Copy_array_to_flash() {
    flash_write_requested = true;
    USART2_Send_responce(response::pending);
}

// В main() в цикле
void g4_bootloader() {
    while (1) {
        if (flash_write_requested) {
            flash_write_requested = false;
            const uint32_t addr = 0x08008000;
            FlashPageInfo_t info = GetPageAndBank(addr, bin_data_length);
            Erase(info.start_page, info.bank);
            Flash_Optimized(addr, bin_data, bin_data_length);
        }
    }
}
```

### Б. Оптимизация функции Flash()

#### 1. Прямое чтение uint64_t (если адрес выровнен)

```cpp
void Flash_Optimized(uint32_t Address, volatile uint8_t* Data, uint32_t size) {
    if (Address % 8 != 0 || size == 0 || Data == NULL) {
        return;
    }

    Unlock();
    __DMB(); // Один барьер перед началом

    uint32_t doublewords_count = (size + 7) / 8;
    
    // Проверяем выравнивание Data
    bool is_aligned = ((uintptr_t)Data % 8) == 0;
    
    for (uint32_t i = 0; i < doublewords_count; i++) {
        uint32_t current_address = Address + (i * 8);
        uint64_t current_data;
        
        if (is_aligned && (i < doublewords_count - 1 || (size % 8) == 0)) {
            // Быстрое чтение - прямой доступ к uint64_t
            volatile uint64_t* src_ptr = (volatile uint64_t*)(&Data[i * 8]);
            current_data = *src_ptr;
        } else {
            // Медленное чтение для последнего неполного DOUBLEWORD
            uint32_t bytes_to_copy = (i == doublewords_count - 1 && (size % 8) != 0) 
                                      ? (size % 8) : 8;
            current_data = 0xFFFFFFFFFFFFFFFF;
            
            uint8_t* dst = (uint8_t*)&current_data;
            for (uint32_t j = 0; j < bytes_to_copy; j++) {
                dst[j] = Data[i * 8 + j];
            }
        }
        
        HAL_StatusTypeDef status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_DOUBLEWORD, 
            current_address, 
            current_data
        );
        
        if (status != HAL_OK) {
            __DMB();
            Lock();
            return;
        }
    }
    
    __DMB(); // Один барьер после завершения
    Lock();
}
```

#### 2. Использовать memcpy для копирования

```cpp
// Вместо побайтного копирования
memcpy(&current_data, &Data[i * 8], bytes_to_copy);
```

#### 3. Убрать лишние барьеры из цикла

Оставить барьеры только:
- Один перед началом записи
- Один после завершения

### В. Улучшить безопасность

```cpp
// Добавить проверку таймаута с возвратом ошибки
typedef enum {
    FLASH_OK = 0,
    FLASH_ERROR,
    FLASH_TIMEOUT,
    FLASH_PARAM_ERROR
} FlashStatus_t;

FlashStatus_t Flash_Safe(uint32_t Address, volatile uint8_t* Data, 
                         uint32_t size, uint32_t timeout_ms) {
    // ... проверки параметров ...
    
    uint32_t start_tick = HAL_GetTick();
    
    for (uint32_t i = 0; i < doublewords_count; i++) {
        // Проверка таймаута
        if ((HAL_GetTick() - start_tick) > timeout_ms) {
            __DMB();
            Lock();
            return FLASH_TIMEOUT;
        }
        
        // ... запись ...
    }
    
    return FLASH_OK;
}
```

## Выводы

### Текущие проблемы:
1. ❌ Недопустимо долго блокирует прерывание (12+ мс)
2. ❌ Неэффективное копирование данных (8× медленнее)
3. ❌ Избыточные барьеры памяти в цикле
4. ⚠️ Потенциальная потеря volatile через (void*)
5. ⚠️ Нет защиты от зависания

### Рекомендуемые действия:
1. **КРИТИЧНО**: Перенести запись Flash из прерывания в main loop
2. Оптимизировать копирование данных (прямой доступ к uint64_t)
3. Убрать барьеры из цикла
4. Добавить таймауты и возврат ошибок
5. Использовать правильное volatile-приведение

### Ожидаемое улучшение:
- **Скорость**: 3-5× быстрее за счет прямого чтения uint64_t
- **Надежность**: Нет блокировки прерывания
- **Безопасность**: Таймауты и проверка ошибок
