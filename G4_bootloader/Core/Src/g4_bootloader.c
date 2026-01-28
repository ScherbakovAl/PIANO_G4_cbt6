#include "g4_bootloader.h"

void g4_bootloader() {

    // 1. Разблокировка Flash
    Unlock();
    
    // 2. Стирание страницы (с ожиданием завершения операции)
    Erase();
    
    // 3. Блокировка Flash после стирания
    Lock();

    // 4. Разблокировка Flash для записи
    Unlock();

    // ВАЖНО: Адрес для DOUBLEWORD должен быть выровнен по 8 байт!
    uint32_t address = 0x0800A000; // 0x08009FF0 - смотреть с этого // с адреса 0x0804 0000 - bank2
    uint64_t data = 0xFAFBECEDABAC8888;
    
    // КРИТИЧНО: При записи DOUBLEWORD адрес увеличивается на 8 байт (размер uint64_t), а не на 1!
    // for (int i = 0; i < 0xFF; ++i) {
        Flash(address, data);
        address += 8;  // Увеличиваем адрес на 8 байт (размер DOUBLEWORD)
        data = 0xFAFBECEDABAC1111;
        Flash(address, data);
                address += 8;  // Увеличиваем адрес на 8 байт (размер DOUBLEWORD)
        data = 0xFAFBECEDABAC2222;
        Flash(address, data);
    // }

    // 5. Блокировка Flash после записи
    Lock();


// пометочки..
// .......................................................

// uint64_t data[4] = {
//     0x1122334455667788,  // В памяти: 0x88 0x77 0x66 0x55 0x44 0x33 0x22 0x11
//     0x8877665544332211,  // В памяти: 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
//     0xAABBCCDDEEFF0011,  // В памяти: 0x11 0x00 0xFF 0xEE 0xDD 0xCC 0xBB 0xAA
//     0x11FFEEDDCCBBAA00   // В памяти: 0x00 0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x11
// };

// Адрес должен быть кратен 32 байтам (256 бит)
// __ALIGNED(32) uint32_t data[8]; // Для FLASHWORD

// После записи проверьте данные:
// if (*(__IO uint32_t*)address != data[0]) {
    // Ошибка записи
// }
// .......................................................

    while (1) {}
}

void Unlock() {
    HAL_FLASH_Unlock();
}

void Lock() {
    HAL_FLASH_Lock();
}

void Erase() {
    // Использование низкоуровневой функции FLASH_PageErase требует ручного управления!
    FLASH_PageErase(20, FLASH_BANK_1);
    
    // КРИТИЧНО: Необходимо дождаться завершения операции стирания!
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    
    // КРИТИЧНО: Очистить биты управления стиранием (PER и PNB)
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));
    
    // Очистить флаги ошибок, если они есть
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
}

void Read() {
  // FLASH READ: 
// uint8_t *MEM_If_Read_HS(uint8_t *src, uint8_t *dest, uint32_t Len)
// {
    uint8_t* src; // адрес
    uint8_t* dest; // результат чтения
    uint32_t Len = 10; // количество данных
    uint32_t i = 0; // счётчик
    uint8_t* psrc = src;
    for (i = 0; i < Len; i++) {
        dest[i] = *psrc++;
    }
    // return (uint8_t *) (dest);
    // }

    // или

      // if ((*(volatile uint32_t*)(Flash_Address + 0x640)) != key_to_change_memory) {
      // 	SaveToMemory();
      // }
      // else {
      // for (int i = 0; i < sizeCHART_BUFFER; ++i) {
      // 	compsCHART_0[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)));
      // 	compsCHART_1[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)) + 0x320);
      // }
}

void Flash(uint32_t Address, uint64_t Data) {
    // ВАЖНО: Проверяем, что адрес выровнен по 8 байт для DOUBLEWORD
    if (Address % 8 != 0) {
        // Ошибка выравнивания! Адрес должен быть кратен 8
        return;
    }
    
    // Выполняем запись с проверкой статуса
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, Data);
    
    // Можно проверить статус и обработать ошибку:
    // if (status != HAL_OK) {
    //     // Обработка ошибки записи
    //     uint32_t error = HAL_FLASH_GetError();
    // }
}
