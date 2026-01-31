#include "g4_bootloader.h"

uint8_t ChipN[1] = { 0x35 }; // <<<<< номер чипа указать здесь (который запишется в FLASH в первый раз)
volatile uint8_t ChipNumber[1] = {};
__attribute__((aligned(8))) uint8_t bin_data[2048] = {}; // массив для получения прошивки
const uint32_t bin_data_length = sizeof(bin_data);

__attribute__((aligned(32))) const volatile uint32_t ChipN_32[1] = { 0x35 }; // <<<<< номер чипа указать здесь (который запишется в FLASH в первый раз)
__attribute__((aligned(32))) volatile uint32_t ChipNumber_32[1] = {}; // <<<<< номер чипа - использовать в коде
__attribute__((aligned(32))) volatile uint32_t bin_data_32[512] = {};
const uint32_t bin_data_32_length = sizeof(bin_data_32); // 2 kB


static const uint32_t ADRESS_CHIP_NUMBER = 0x08003800; // здесь храним номер чипа
static volatile uint32_t MAIN_FIRMWARE = 0x08010000; // здесь основная прошивка // TODO MAIN_FIRMWARE?

static inline void uint32_to_bytes_pointer(uint32_t value, uint8_t* bytes);
static inline uint32_t bytes_to_uint32_pointer(const uint8_t* bytes);

// settings
__attribute__((aligned(8))) uint8_t tx_s[5] = { };
const int tx_settings_length = sizeof(tx_s);
__attribute__((aligned(8))) uint8_t rx_s[5] = { };
const int rx_settings_length = sizeof(rx_s);

void g4_bootloader() {

    // считываем номер чипа из памяти (или если надо записываем)
    // #define testt
#ifdef testt
    FlashPageInfo_t info = GetPageAndBank(ADRESS_CHIP_NUMBER, 1);
    Erase(info.start_page, info.bank); // очистить номер чипа, если надо обновить номер
#endif
    Read(ADRESS_CHIP_NUMBER, const_cast<uint8_t*>(ChipNumber), 1u);
    if (ChipNumber[0] == 0xFF) {
        FlashPageInfo_t info = GetPageAndBank(ADRESS_CHIP_NUMBER, 1);
        Erase(info.start_page, info.bank); // очистить номер чипа
        // Erase(PAGE_CHIP, BANK_N_CHIP); 
        Flash(ADRESS_CHIP_NUMBER, reinterpret_cast<uint64_t*>(ChipN), 1u);
        ChipNumber[0] = ChipN[0];
    }

    // UART init
    LL_USART_DisableDirectionTx(USART2); // один или несколько MCU должны быть подтянуты к pull_UP ! // надо ли? проверить..
    LL_USART_Disable(USART2);            // // надо ли? проверить..
    LL_USART_ConfigNodeAddress(USART2, LL_USART_ADDRESS_DETECT_7B, ChipNumber[0]);
    LL_USART_ClearFlag_TC(USART2);
    LL_USART_EnableIT_RXNE_RXFNE(USART2);  // включение прерывания по приему данных
    LL_USART_EnableMuteMode(USART2);       // включение MuteMode
    LL_USART_RequestEnterMuteMode(USART2); // ставит USART в MuteMode и сбрасывает флаг RWU.
    LL_USART_Enable(USART2);
    LL_USART_DisableDirectionTx(USART2);   // один или несколько MCU должны быть подтянуты к pull_UP !
    //---------------------------------

    tx_s[0] = ChipNumber[0];
    rx_s[0] = ChipNumber[0];




    while (1) {}
}

void USART2_IRQ() {
    if (LL_USART_IsActiveFlag_RXNE_RXFNE(USART2)) { // здесь прерывание, если адрес UART совпал
        communication();
        // LL_USART_RequestRxDataFlush(USART2);   // сбрасывает RXNE флаг
        LL_USART_RequestEnterMuteMode(USART2); // ставит USART в MuteMode и сбрасывает флаг RWU.
    }
}

void USART2_Receive_settings2();

void communication() {

    USART2_Receive_settings();
    const uint8_t rx_command = rx_s[0];

    if (rx_command == command::reset) { // ok

        USART2_Send_responce(response::ok);
        Reset();
    }

    if (rx_command == command::jump_to_piano_g4) { // ok

        USART2_Send_responce(response::ok);
        Jump_to_app();
    }

    if (rx_command == command::set_number) { // ok

        Set_N(rx_s[1]);
        Set_tx_s(0, ChipNumber[0], 0, 0, 0);
        USART2_Send_settings();
        Set_tx_s(0, LL_USART_GetNodeAddress(USART2), 0, 0, 0);
        USART2_Send_settings();
    }

    if (rx_command == command::data_from_H7_to_array_g4) { // ок

        From_H7_to_array_g4();
    }

    if (rx_command == command::data_from_array_g4_to_H7) { // --

        From_array_g4_to_H7();
    }

    if (rx_command == command::copy_array_to_flash) { // TODO

        Copy_array_to_flash();
    }

    if (rx_command == command::echo) { // ok
        Set_tx_s(0, ChipNumber[0], 0, 0, 0);
        USART2_Send_settings();
    }
}

// UART Send-Recive
void USART2_Send_settings() {
    LL_USART_EnableDirectionTx(USART2); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    for (int i = 0; i < tx_settings_length; i++) {
        LL_USART_TransmitData9(USART2, tx_s[i]);
        while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    }
    while (!LL_USART_IsActiveFlag_TC(USART2)) {}
    LL_USART_DisableDirectionTx(USART2);
    LL_USART_ClearFlag_TC(USART2);
}

void USART2_Send_responce(response r) {
    LL_USART_EnableDirectionTx(USART2); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    tx_s[0] = ChipNumber[0];
    tx_s[1] = r;
    tx_s[2] = r;
    tx_s[3] = r;
    tx_s[4] = r;
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    for (int i = 0; i < tx_settings_length; i++) {
        LL_USART_TransmitData9(USART2, tx_s[i]);
        while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    }
    while (!LL_USART_IsActiveFlag_TC(USART2)) {}
    LL_USART_DisableDirectionTx(USART2);
    LL_USART_ClearFlag_TC(USART2);
}

void USART2_Receive_settings() {
    while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
    rx_s[0] = LL_USART_ReceiveData9(USART2);
    for (int i = 0; i < rx_settings_length; i++) {
        while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
        rx_s[i] = LL_USART_ReceiveData9(USART2);
    }
    LL_USART_RequestRxDataFlush(USART2);
    // LL_USART_RequestEnterMuteMode(USART2);
}

void USART2_Receive_settings2() {
    while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
    // rx_s[0] = LL_USART_ReceiveData9(USART2);
    for (int i = 0; i < rx_settings_length; i++) {
        while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
        rx_s[i] = LL_USART_ReceiveData9(USART2);
    }
    LL_USART_RequestRxDataFlush(USART2);
    // LL_USART_RequestEnterMuteMode(USART2);
}
//---------------------------------

void Unlock() {
    HAL_FLASH_Unlock();
}

void Lock() {
    HAL_FLASH_Lock();
}

void Erase(uint32_t Page, uint32_t Banks) {
    Unlock();
    // Использование низкоуровневой функции FLASH_PageErase требует ручного управления!
    FLASH_PageErase(Page, Banks);

    // Барьер памяти - предотвращает переупорядочивание операций
    __DMB();

    // КРИТИЧНО: Необходимо дождаться завершения операции стирания!
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    // КРИТИЧНО: Очистить биты управления стиранием (PER и PNB)
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));

    // Очистить флаги ошибок, если они есть
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    // Барьер памяти перед разблокировкой
    __DMB();
    Lock();
}

void Flash(uint32_t Address, uint64_t* Data, uint32_t size) {
    // ВАЖНО: Проверяем, что адрес выровнен по 8 байт для DOUBLEWORD
    if (Address % 8 != 0) {
        // Ошибка выравнивания! Адрес должен быть кратен 8
        return;
    }

    // Проверяем корректность параметра size (должен быть > 0)
    if (size == 0 || Data == NULL) {
        return;
    }

    Unlock();

    // Барьер памяти перед началом операций
    __DMB();

    // Вычисляем количество DOUBLEWORD для записи
    // Каждое DOUBLEWORD = 8 байт
    uint32_t doublewords_count = (size + 7) / 8; // Округление вверх

    // Создаем указатель на исходные данные как uint8_t для безопасного доступа
    volatile uint8_t* src_bytes = (volatile uint8_t*)Data;

    // Записываем данные в цикле
    for (uint32_t i = 0; i < doublewords_count; i++) {
        uint32_t current_address = Address + (i * 8);
        uint64_t current_data;

        // Безопасное копирование данных по байтам (избегаем проблем с выравниванием и volatile)
        uint8_t* dst = (uint8_t*)&current_data;
        uint32_t bytes_to_copy = 8;

        // Если это последний элемент и size не кратен 8, обрабатываем остаток
        if (i == doublewords_count - 1 && (size % 8) != 0) {
            bytes_to_copy = size % 8;
            current_data = 0xFFFFFFFFFFFFFFFF; // Заполняем FF (стертое состояние Flash)
        }

        // Копируем данные побайтно для избежания проблем с выравниванием
        for (uint32_t j = 0; j < bytes_to_copy; j++) {
            dst[j] = src_bytes[i * 8 + j];
        }

        // Выполняем запись с проверкой статуса
        HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_address, current_data);

        // Барьер памяти после записи
        __DMB();

        FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

        // Проверяем статус и обрабатываем ошибку:
        if (status != HAL_OK) {
            // Обработка ошибки записи
            // uint32_t error = HAL_FLASH_GetError();
            __DMB();
            Lock();
            return;
        }
    }

    // Барьер памяти перед завершением
    __DMB();
    Lock();
}

void Read(uint32_t Address, uint8_t* pData, uint32_t Size) {

    if (pData == NULL) {
        return;
    }

    // Проверка, что адрес находится в области флеш-памяти
    // Для STM32G474CBT6: флеш от 0x08000000 до 0x08020000 (128KB)
    if (Address < FLASH_BASE || Address >= (FLASH_BASE + FLASH_SIZE)) {
        return;
    }

    // Проверка, что чтение не выходит за границы флеш-памяти
    if ((Address + Size) > (FLASH_BASE + FLASH_SIZE)) {
        return;
    }

    // Прямое чтение из памяти побайтно
    uint8_t* pFlashAddr = (uint8_t*)Address;
    for (uint32_t i = 0; i < Size; i++) {
        pData[i] = pFlashAddr[i];
    }
}

/**
 * @brief Чтение данных из Flash памяти в формате uint32_t
 * @param Address: Адрес начала чтения во Flash (должен быть выровнен по 4 байта)
 * @param pData: Указатель на массив uint32_t для сохранения прочитанных данных
 * @param Size: Размер данных в БАЙТАХ (не в элементах uint32_t!)
 * @note  Функция безопасна для вызова из прерывания
 * @note  Размер Size будет округлен вверх до кратного 4
 *
 * Пример использования:
 * __attribute__((aligned(32))) volatile uint32_t bin_data_32[512];
 * Read_uint32(0x08008000, bin_data_32, 2048); // Прочитать 2048 байт (512 uint32_t)
 */
void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size) {

    if (pData == NULL || Size == 0) {
        return;
    }

    // Проверка, что адрес находится в области флеш-памяти
    if (Address < FLASH_BASE || Address >= (FLASH_BASE + FLASH_SIZE)) {
        return;
    }

    // Проверка выравнивания адреса по 4 байта для оптимальной работы
    if (Address % 4 != 0) {
        return;
    }

    // Вычисляем количество uint32_t элементов (округляем вверх)
    uint32_t count = (Size + 3) / 4;

    // Проверка, что чтение не выходит за границы флеш-памяти
    if ((Address + (count * 4)) > (FLASH_BASE + FLASH_SIZE)) {
        return;
    }

    // Прямое чтение из памяти uint32_t словами (быстрее чем побайтно)
    volatile uint32_t* pFlashAddr = (volatile uint32_t*)Address;

    // Используем критическую секцию для безопасности вызова из прерывания
    uint32_t primask = __get_PRIMASK();
    // __disable_irq();

    for (uint32_t i = 0; i < count; i++) {
        pData[i] = pFlashAddr[i];
    }

    __set_PRIMASK(primask);
}

/**
 * @brief Запись данных в Flash память в формате uint32_t
 * @param Address: Адрес начала записи во Flash (должен быть выровнен по 8 байт)
 * @param Data: Указатель на массив uint32_t с данными для записи
 * @param size: Размер данных в БАЙТАХ (не в элементах uint32_t!)
 * @note  Функция безопасна для вызова из прерывания
 * @note  Размер size будет округлен вверх до кратного 8 (т.к. запись doubleword)
 * @note  Перед записью необходимо стереть страницу Flash с помощью Erase()
 *
 * Пример использования:
 * __attribute__((aligned(32))) volatile uint32_t bin_data_32[512] = {данные};
 * uint32_t addr = 0x08008000;
 * FlashPageInfo_t info = GetPageAndBank(addr, 2048);
 * Erase(info.start_page, info.bank);
 * Flash_uint32(addr, bin_data_32, 2048); // Записать 2048 байт (512 uint32_t)
 */
void Flash_uint32(uint32_t Address, volatile uint32_t* Data, uint32_t size) {
    // Проверка выравнивания адреса по 8 байт для DOUBLEWORD
    if (Address % 8 != 0) {
        Set_tx_s(0x11, 0x11, 0x11, 0x11, 0x11);
        USART2_Send_settings();
        return;
    }

    // Проверяем корректность параметров
    if (size == 0 || Data == NULL) {
        Set_tx_s(0x22, 0x22, 0x22, 0x22, 0x22);
        USART2_Send_settings();
        return;
    }

    // Используем критическую секцию для безопасности вызова из прерывания
    uint32_t primask = __get_PRIMASK();
    // __disable_irq();

    Unlock();

    // Барьер памяти перед началом операций
    __DMB();

    // Вычисляем количество DOUBLEWORD для записи (каждое = 8 байт = 2 uint32_t)
    uint32_t doublewords_count = (size + 7) / 8;

    // Записываем данные в цикле
    for (uint32_t i = 0; i < doublewords_count; i++) {
        uint32_t current_address = Address + (i * 8);
        uint64_t current_data;

        // Индекс в массиве uint32_t (каждый doubleword = 2 uint32_t)
        uint32_t idx = i * 2;

        // Формируем 64-битное значение из двух 32-битных
        // Little Endian: младшие биты идут первыми
        if ((i * 8 + 8) <= size) {
            // Полный doubleword
            current_data = ((uint64_t)Data[idx]) | (((uint64_t)Data[idx + 1]) << 32);
        }
        else {
         // Последний неполный doubleword - заполняем FF
            current_data = 0xFFFFFFFFFFFFFFFF;
            uint32_t remaining_bytes = size - (i * 8);

            if (remaining_bytes >= 4) {
                // Первые 4 байта из Data[idx]
                current_data = (uint64_t)Data[idx];
                if (remaining_bytes > 4) {
                    // Частично второй uint32_t
                    uint32_t partial = Data[idx + 1];
                    uint32_t valid_bytes = remaining_bytes - 4;
                    uint32_t mask = (1UL << (valid_bytes * 8)) - 1;
                    partial &= mask;
                    current_data |= ((uint64_t)partial) << 32;
                }
            }
            else {
             // Меньше 4 байт - только часть первого uint32_t
                uint32_t partial = Data[idx];
                uint32_t mask = (1UL << (remaining_bytes * 8)) - 1;
                partial &= mask;
                current_data = (uint64_t)partial;
            }
        }

        // Выполняем запись
        HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_address, current_data);

        // Барьер памяти после записи
        __DMB();

        FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

        // Проверяем статус
        // if (status != HAL_OK) {
        //     __DMB();
        //     Lock();
        //     __set_PRIMASK(primask);
        //     return;
        // }
    }

    // Барьер памяти перед завершением
    __DMB();
    Lock();

    __set_PRIMASK(primask);
}

void Reset() {
    // LL_mDelay(20); // TODO ?
    NVIC_SystemReset();
}

void Jump_to_app() {
    // HAL_RCC_DeInit(); // TODO это надо?
    // HAL_DeInit(); // TODO это надо?
    // __disable_irq(); // TODO это надо?

    // Установка нового вектора прерываний
    SCB->VTOR = MAIN_FIRMWARE; // TODO это надо?

    // Установка указателя стека
    __set_MSP(*(__IO uint32_t*)MAIN_FIRMWARE); // TODO это надо?

    void (*JumpToApplication)(void);
    uint32_t JumpAddress;
    JumpAddress = *(__IO uint32_t*) (MAIN_FIRMWARE + 4); // здесь должно быть " + 4 ", а в скрипте компоновщика " - 8 " !!
    JumpToApplication = (void (*)(void))JumpAddress;
    JumpToApplication();
}

void Set_N(const int n) {
    ChipNumber[0] = 0;
    FlashPageInfo_t info = GetPageAndBank(ADRESS_CHIP_NUMBER, 1);
    Erase(info.start_page, info.bank); // очистить номер чипа
    uint64_t vrem[1] = { (uint64_t)n };
    Flash(ADRESS_CHIP_NUMBER, vrem, 1u);
    Read(ADRESS_CHIP_NUMBER, (uint8_t*)ChipNumber, 1u);
    LL_USART_Disable(USART2);
    LL_USART_ConfigNodeAddress(USART2, LL_USART_ADDRESS_DETECT_7B, ChipNumber[0]);
    LL_USART_Enable(USART2);
}

void From_H7_to_array_g4() {

    Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], rx_s[4]); // отвечает 0x11, 0x12, 0x13, 0x14
    USART2_Send_settings();

    for (uint32_t i = 0; i < bin_data_32_length / 4; ++i) {
        USART2_Receive_settings2();
        bin_data_32[i] = bytes_to_uint32_pointer(rx_s);
        Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], (uint8_t)i); // отвечает то, что получил
        USART2_Send_settings();
    }

    USART2_Send_responce(response::ok);
}

void From_array_g4_to_H7() {
    // for (int i = 0; i < bin_data_length;) {
    //     *reinterpret_cast<uint32_t*>(rx_s[i + 1]) = *reinterpret_cast<uint32_t*>(bin_data[i]);
    //     USART2_Send_settings();
    //     i += 4;
    // }
    // USART2_Send_responce(response::ok);
}



static inline void uint32_to_bytes_pointer(uint32_t value, uint8_t* bytes) {
    *((uint32_t*)bytes) = value; // требуется выравнивание !
}

static inline uint32_t bytes_to_uint32_pointer(const uint8_t* bytes) {
    return *((uint32_t*)bytes); // требуется выравнивание !
}


void Copy_array_to_flash() {

    Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], rx_s[4]); // отвечает 0x06 0x11 0x12 0x13 0x14
    USART2_Send_settings();

    // **  ****  ****  ****  ****  ****  ****  ****  ****  **


    // 1 получить адрес для памяти, куда сохранить массив
    USART2_Receive_settings2();


    // 2 послать адрес в ответ на проверку
    Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], rx_s[4]); // отвечает форматированным адресом
    USART2_Send_settings();
    // const uint32_t addr_raw = bytes_to_uint32_pointer(&rx_s[1]);
    const uint32_t addr = bytes_to_uint32_pointer(rx_s);


    // 3 если ок, то записываем в память..
    USART2_Receive_settings2();

    volatile int bug = 0;
    if (rx_s[0] == response::ok) {
        
        FlashPageInfo_t info = GetPageAndBank(addr, bin_data_32_length);
        Erase(info.start_page, info.bank);

        // 3.2
        Set_tx_s(0x32, 0x32, 0x32, 0x32, 0x32);
        USART2_Send_settings();

        // Гарантируем завершение Erase перед Flash
        __DSB();
        __ISB();

        Flash_uint32(addr, bin_data_32, bin_data_32_length);

        // Гарантируем завершение Flash перед чтением
        __DSB();
        __ISB();

        // 3.5
        Set_tx_s(0x35, 0x35, 0x35, 0x35, 0x35);
        USART2_Send_settings();

        // Проверка записанных данных
        volatile uint8_t test[1] = {};
        for (uint32_t i = 0; i < bin_data_32_length / 4; ++i) {
            Read(addr + i, const_cast<uint8_t*>((uint8_t*)test), 1u);
            if (bin_data_32[i] != test[0]) {
                ++bug;
            }
        }
    }

 // 4
    if (bug) {
        USART2_Send_responce(response::fail);
    }
    else {
        USART2_Send_responce(response::ok);
    }
}

void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
    tx_s[0] = a;
    tx_s[1] = b;
    tx_s[2] = c;
    tx_s[3] = d;
    tx_s[4] = e;
}

/**
 * @brief  Определяет страницу и банк памяти Flash для заданного диапазона адресов
 * @param  Address: Начальный адрес диапазона данных
 * @param  Size: Размер диапазона данных в байтах
 * @retval FlashPageInfo_t: Структура с информацией о странице и банке
 *
 * @note   Параметры Flash для STM32G474CBT6:
 *         - Общий размер Flash: 128 KB (0x08000000 - 0x0801FFFF)
 *         - Размер страницы: 2 KB (2048 байт)
 *         - Bank 1: 0x08000000 - 0x0800FFFF (страницы 0-31)
 *         - Bank 2: 0x08010000 - 0x0801FFFF (страницы 0-31)
 */
FlashPageInfo_t GetPageAndBank(uint32_t Address, uint32_t Size) {
    FlashPageInfo_t info = { 0 };

    // Размер страницы Flash для STM32G474
#define FLASH_PAGE_SIZE     2048    // 2 KB
#define FLASH_BANK1_BASE    0x08000000
#define FLASH_BANK2_BASE    0x08010000
#define FLASH_BANK_SIZE     (64 * 1024)  // 64 KB на банк

// Проверка, что адрес находится в области Flash памяти
    if (Address < FLASH_BASE || Address >= (FLASH_BASE + FLASH_SIZE)) {
        info.is_valid = 0;
        return info;
    }

    // Вычисляем конечный адрес диапазона
    uint32_t end_address = Address + Size - 1;

    // Проверка, что весь диапазон находится в области Flash
    if (end_address >= (FLASH_BASE + FLASH_SIZE)) {
        info.is_valid = 0;
        return info;
    }

    // Определяем банк для начального адреса
    if (Address < FLASH_BANK2_BASE) {
        info.bank = FLASH_BANK_1;
        // Вычисляем номер начальной страницы относительно начала Bank 1
        info.start_page = (Address - FLASH_BANK1_BASE) / FLASH_PAGE_SIZE;

        // Вычисляем номер конечной страницы
        if (end_address < FLASH_BANK2_BASE) {
            // Весь диапазон находится в Bank 1
            info.end_page = (end_address - FLASH_BANK1_BASE) / FLASH_PAGE_SIZE;
        }
        else {
         // Диапазон пересекает границу банков
         // Возвращаем последнюю страницу Bank 1
            info.end_page = 31;  // Последняя страница Bank 1
        }
    }
    else {
        info.bank = FLASH_BANK_2;
        // Вычисляем номер начальной страницы относительно начала Bank 2
        info.start_page = (Address - FLASH_BANK2_BASE) / FLASH_PAGE_SIZE;
        // Вычисляем номер конечной страницы
        info.end_page = (end_address - FLASH_BANK2_BASE) / FLASH_PAGE_SIZE;
    }

    info.is_valid = 1;

#undef FLASH_PAGE_SIZE
#undef FLASH_BANK1_BASE
#undef FLASH_BANK2_BASE
#undef FLASH_BANK_SIZE

    return info;
}



// пометочки..
/*
// .......................................................
    // BANK_1 -  0 to page 31
    // Erase(20, FLASH_BANK_1);
    // Erase(21, FLASH_BANK_1);
    // Erase(22, FLASH_BANK_1);
    // Erase(23, FLASH_BANK_1);

    // ВАЖНО: Адрес для DOUBLEWORD должен быть выровнен по 8 байт!
    // uint32_t addressIN = 0x08000000; // 0x08009FF0 - смотреть с этого // с адреса 0x0804 0000 - bank2
    // uint32_t addressOUT = 0x0800A000;
    // uint64_t data = 0xFAFBECEDABAC8888;

    // uint64_t data[4] = {
    //     0x1122334455667788,  // В памяти: 0x88 0x77 0x66 0x55 0x44 0x33 0x22 0x11
    //     0x8877665544332211,  // В памяти: 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
    //     0xAABBCCDDFEFE0011,  // В памяти: 0x11 0x00 0xFF 0xEE 0xDD 0xCC 0xBB 0xAA
    //     0x11FFEEDDCCBBAA00   // В памяти: 0x00 0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x11
    // };

    // uint32_t data[4] = {
    // 0x12345678,  // В памяти: 0x88 0x77 0x66 0x55 0x44 0x33 0x22 0x11
    // 0x87654321,  // В памяти: 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
    // 0xABCDEF11,  // В памяти: 0x11 0x00 0xFF 0xEE 0xDD 0xCC 0xBB 0xAA
    // 0x22FEDCBA   // В памяти: 0x00 0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x11
    // };

    // uint16_t data[4] = {
    // 0x1234,  // В памяти: 0x88 0x77 0x66 0x55 0x44 0x33 0x22 0x11
    // 0x8765,  // В памяти: 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
    // 0xABCD,  // В памяти: 0x11 0x00 0xFF 0xEE 0xDD 0xCC 0xBB 0xAA
    // 0x22FE   // В памяти: 0x00 0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x11
    // };

    // uint8_t data[4] = {
    // 0x12,  // В памяти: 0x88 0x77 0x66 0x55 0x44 0x33 0x22 0x11
    // 0x34,  // В памяти: 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
    // 0x56,  // В памяти: 0x11 0x00 0xFF 0xEE 0xDD 0xCC 0xBB 0xAA
    // 0x78   // В памяти: 0x00 0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x11
    // };

    // Записываем весь массив за один вызов
    // size = количество элементов * размер элемента = 4 * 8 = 32 байта
    // Flash(addressOUT, addressIN, 0x12A8);
// .......................................................



// пометочки 2..
// .......................................................
// Адрес должен быть кратен 32 байтам (256 бит)
// __ALIGNED(32) uint32_t data[8]; // Для FLASHWORD

// После записи проверьте данные:
// if (*(__IO uint32_t*)address != data[0]) {
    // Ошибка записи
// }

// Функция чтения
// void ReadFromFlash(uint64_t* buffer, uint32_t size) {
//     uint64_t* addr = (uint64_t*)FLASH_USER_START_ADDR;
//     for (uint32_t i = 0; i < size; i++) {
//         buffer[i] = addr[i];
//     }
// }

// void ReadIf() {
      // if ((*(volatile uint32_t*)(Flash_Address + 0x640)) != key_to_change_memory) {
      // 	SaveToMemory();
      // }
      // else {
      // for (int i = 0; i < sizeCHART_BUFFER; ++i) {
      // 	compsCHART_0[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)));
      // 	compsCHART_1[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)) + 0x320);
      // }
// }

// прыжок как пример:
// #define APP_START_ADDR 0x08010000

// typedef void (*pFunction)(void);
// pFunction JumpToApplication;

// void JumpToUserApp(void) {
//     uint32_t jump_address = *(__IO uint32_t*)(APP_START_ADDR + 4);

//     // Проверка стека (указатель должен быть в пределах RAM)
//     if ((*(__IO uint32_t*)APP_START_ADDR & 0x2FFE0000) == 0x20000000) {
//         // Деинициализация периферии
//         HAL_RCC_DeInit();
//         HAL_DeInit();

//         // Отключение прерываний
//         __disable_irq();

//         // Установка нового вектора прерываний
//         SCB->VTOR = APP_START_ADDR;

//         // Установка указателя стека
//         __set_MSP(*(__IO uint32_t*)APP_START_ADDR);

//         // Получение адреса Reset_Handler
//         JumpToApplication = (pFunction)jump_address;

//         // Переход в приложение
//         JumpToApplication();
//     }
// }
// .......................................................

*/

// ============================================================================
// ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ НОВЫХ ФУНКЦИЙ Flash_uint32 И Read_uint32
// ============================================================================

// ПРИМЕР 1: Конвертация bin_data (uint8_t) в bin_data_32 (uint32_t) и запись
void Example_Convert_And_Write() {
    // Шаг 1: Конвертируем uint8_t массив в uint32_t
    // bin_data[2048] -> bin_data_32[512]
    for (uint32_t i = 0; i < bin_data_length / 4; i++) {
        // Вариант через побитовые операции (универсальный)
        // bin_data_32[i] = bytes_to_uint32_shift(&bin_data[i * 4]);

        // Или вариант через прямое приведение (если bin_data выровнен по 4)
        // bin_data_32[i] = *((uint32_t*)(&bin_data[i * 4]));
    }

    // Шаг 2: Записываем в flash
    const uint32_t addr = 0x08008000;
    FlashPageInfo_t info = GetPageAndBank(addr, bin_data_32_length);
    Erase(info.start_page, info.bank);

    __DSB();  // Гарантируем завершение стирания
    __ISB();

    Flash_uint32(addr, bin_data_32, bin_data_32_length);
}

// ПРИМЕР 2: Чтение из flash в uint32_t массив и конвертация обратно в uint8_t
void Example_Read_And_Convert() {
    const uint32_t addr = 0x08008000;

    // Шаг 1: Читаем из flash в uint32_t массив
    Read_uint32(addr, bin_data_32, bin_data_32_length);

    // Шаг 2: Конвертируем uint32_t обратно в uint8_t
    for (uint32_t i = 0; i < bin_data_32_length / 4; i++) {
        // Вариант через побитовые операции
        // uint32_to_bytes_shift(bin_data_32[i], &bin_data[i * 4]);

        // Или прямое приведение (если bin_data выровнен)
        // *((uint32_t*)(&bin_data[i * 4])) = bin_data_32[i];
    }
}

// ПРИМЕР 3: Использование из прерывания (безопасно!)
void USART2_IRQ_Example() {
    // Эти функции безопасны для вызова из прерывания
    // т.к. используют критические секции внутри

    // Чтение
    Read_uint32(0x08008000, bin_data_32, 2048);

    // Запись (но убедитесь что страница предварительно стерта!)
    // Flash_uint32(0x08008000, bin_data_32, 2048);
}

// ПРИМЕР 4: Прямая работа без конвертации (только uint32_t)
void Example_Direct_uint32_Work() {
    // Заполняем данными напрямую
    for (uint32_t i = 0; i < 512; i++) {
        bin_data_32[i] = 0x12345678 + i;
    }

    // Записываем
    const uint32_t addr = 0x08008000;
    FlashPageInfo_t info = GetPageAndBank(addr, 2048);
    Erase(info.start_page, info.bank);
    __DSB();
    __ISB();
    Flash_uint32(addr, bin_data_32, 2048);

    // Проверяем чтением
    __attribute__((aligned(32))) volatile uint32_t verify_data[512] = {};
    Read_uint32(addr, verify_data, 2048);

    // Сравниваем
    for (uint32_t i = 0; i < 512; i++) {
        if (bin_data_32[i] != verify_data[i]) {
            // Ошибка!
            break;
        }
    }
}

// ============================================================================
