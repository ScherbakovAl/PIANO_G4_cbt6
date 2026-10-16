#include "g4_bootloader.h"

volatile uint32_t ChipN_32 = { 3 }; // <<<< номер чипа здесь указывается

__attribute__((aligned(32))) volatile uint32_t bin_data_32[512] = {};
const uint32_t bin_data_32_length = sizeof(bin_data_32); // 2 kB

static const uint32_t ADRESS_CHIP_NUMBER = 0x08004800; // здесь храним номер чипа
static const uint32_t MAIN_FIRMWARE = 0x08008000; // здесь основная прошивка
static volatile uint32_t ADRESS_FOR_JUMP = 0; // указать куда прыгнуть
extern "C" uint32_t g_pfnVectors[]; // начало этого загрузчика (startup): 0x08000000 у v1, 0x08002000 у v2
static const uint32_t BOOTLOADER_SIZE = 0x2000; // 4 страницы — LENGTH в STM32G474XX_FLASH.ld

// settings
__attribute__((aligned(8))) uint8_t tx_s[5] = { };
const int tx_settings_length = sizeof(tx_s);
__attribute__((aligned(8))) uint8_t rx_s[5] = { };
const int rx_settings_length = sizeof(rx_s);

volatile int flag_for_jump = 0;

void g4_bootloader() {

// #define testt
#ifdef testt // считываем номер чипа из памяти (или если надо записываем)
    FlashPageInfo_t info = GetPageAndBank(ADRESS_CHIP_NUMBER, 4);
    Erase(info.start_page, info.bank); // очистить номер чипа, если надо обновить номер
#endif

    uint32_t ChipN_t;
    Read_uint32(ADRESS_CHIP_NUMBER, &ChipN_t, 4);
    if (ChipN_t == 0xFFFFFFFF) {
        FlashPageInfo_t info = GetPageAndBank(ADRESS_CHIP_NUMBER, 4);
        Erase(info.start_page, info.bank); // очистить номер чипа
        Flash_uint32(ADRESS_CHIP_NUMBER, &ChipN_32, 4);
    }
    else {
        Read_uint32(ADRESS_CHIP_NUMBER, &ChipN_32, 4);
    }

    tx_s[0] = (uint8_t)ChipN_32;
    rx_s[0] = (uint8_t)ChipN_32;
    for (int i = 1; i < 5; ++i) {
        tx_s[i] = 0;
        rx_s[i] = 0;
    }

    LL_USART_RequestRxDataFlush(USART2); // TODO ?
    LL_mDelay(1);
    LL_USART_RequestRxDataFlush(USART2); // TODO ?

    // UART init
    LL_USART_DisableDirectionTx(USART2); // один или несколько MCU должны быть подтянуты к pull_UP ! // надо ли? проверить..
    LL_USART_Disable(USART2);            // надо ли? проверить..
    LL_USART_ConfigNodeAddress(USART2, LL_USART_ADDRESS_DETECT_7B, ChipN_32);
    LL_USART_ClearFlag_TC(USART2);
    LL_USART_EnableIT_RXNE_RXFNE(USART2);  // включение прерывания по приему данных
    LL_USART_EnableMuteMode(USART2);       // включение MuteMode
    LL_USART_RequestEnterMuteMode(USART2); // ставит USART в MuteMode и сбрасывает флаг RWU.
    LL_USART_Enable(USART2);
    LL_USART_DisableDirectionTx(USART2);   // один или несколько MCU должны быть подтянуты к pull_UP !

    LL_USART_RequestRxDataFlush(USART2); // TODO ?
    LL_mDelay(1);
    LL_USART_RequestRxDataFlush(USART2); // TODO ?

    while (1) {
        if (flag_for_jump == 1) {
            USART2_Send_responce(response::ok);
            Jump_to_app(MAIN_FIRMWARE);
        }
        if (flag_for_jump == 2) {
            USART2_Send_responce(response::ok);
            Jump_to_app(ADRESS_FOR_JUMP);
        }
    }
}

void communication() {

    USART2_Receive_settings_after_number();
    const uint8_t rx_command = rx_s[0];

    if (rx_command == (uint8_t)command_bootloader::echo) {
        Set_tx_s((uint8_t)ChipN_32, 0, 0, 0, (uint8_t)chip_states::boot1);
        USART2_Send_settings();
    }

    if (rx_command == (uint8_t)command_bootloader::reset_bootloader) {
        USART2_Send_responce(response::ok);
        Reset();
    }

    if (rx_command == (uint8_t)command_bootloader::data_from_H7_to_array_g4) {
        From_H7_to_array_g4();
    }

    if (rx_command == (uint8_t)command_bootloader::copy_array_to_flash_g4) {
        Copy_array_to_flash();
    }

    if (rx_command == (uint8_t)command_bootloader::jump_to_piano_g4) {
        USART2_Send_responce(response::ok);
        flag_for_jump = 1;
    }

    if (rx_command == (uint8_t)command_bootloader::jump_g4_to_adress) {
        USART2_Send_responce(response::ok);
        jump_to_adress();
    }
}

void Reset() {
    LL_mDelay(10);
    NVIC_SystemReset();
}

void From_H7_to_array_g4() {
    // *  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  **  *
    Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], rx_s[4]); // отвечает 0x11, 0x12, 0x13, 0x14
    USART2_Send_settings();
    // -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   - -   - - -   


    for (uint32_t i = 0; i < 512; ++i) {// 2kB (4*512) размер пакета с прошивкой
        USART2_Receive_settings();
        bin_data_32[i] = bytes_to_uint32_pointer(rx_s);
        Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], (uint8_t)i); // отвечает то, что получил
        USART2_Send_settings();
    }

    USART2_Send_responce(response::ok);
}

// Страницу можно стереть и записать: не этот загрузчик и не номер чипа (иначе — только SWD)
static bool address_allowed(const uint32_t addr) {
    const uint32_t self = (uint32_t)g_pfnVectors;
    return addr % bin_data_32_length == 0
        && addr >= FLASH_BASE && addr + bin_data_32_length <= FLASH_BASE + FLASH_SIZE
        && (addr + bin_data_32_length <= self || addr >= self + BOOTLOADER_SIZE)
        && addr != ADRESS_CHIP_NUMBER;
}

void Copy_array_to_flash() {

    Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], rx_s[4]); // отвечает 0x15 0x61 0x62 0x63 0x64
    USART2_Send_settings();

    // **  ****  ****  ****  ****  ****  ****  ****  ****  **


    // 1 получить адрес для памяти, куда сохранить массив
    USART2_Receive_settings();


    // 2 послать адрес в ответ на проверку
    const uint32_t addr = bytes_to_uint32_pointer(rx_s);
    const bool allowed = address_allowed(addr);
    if (allowed) {
        Set_tx_s(rx_s[0], rx_s[1], rx_s[2], rx_s[3], rx_s[4]); // отвечает форматированным адресом
    }
    else {
        Set_tx_s(0xFF, 0xFF, 0xFF, 0xFF, rx_s[4]); // чужой адрес в ответ: H7 пришлёт fail, страница не стирается
    }
    USART2_Send_settings();
    // const uint32_t addr_raw = bytes_to_uint32_pointer(&rx_s[1]);


    // 3 если ок, то записываем в память..
    USART2_Receive_settings();

    int bug = 0;
    if (allowed && rx_s[0] == (uint8_t)response::ok) {

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

        // Проверка записанных данных - прямое сравнение с памятью
        volatile uint32_t* flash_ptr = (volatile uint32_t*)addr;
        for (uint32_t i = 0; i < (bin_data_32_length / 4); ++i) {
            if (bin_data_32[i] != flash_ptr[i]) {
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

void Jump_to_app(const uint32_t addr) {

    // 2. Деинициализация HAL (сбрасывает SysTick и другие настройки)
    HAL_DeInit();

    // 3. Сброс тактирования в дефолтное состояние
    HAL_RCC_DeInit();

    // 4. Отключаем все прерывания
    // __disable_irq();

    // 5. Отключаем SysTick (критично!)
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // 6. Очищаем все pending прерывания
    for (uint8_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;  // Disable all interrupts
        NVIC->ICPR[i] = 0xFFFFFFFF;  // Clear all pending interrupts
    }

    // 7. Установка нового вектора прерываний
    SCB->VTOR = addr;

    // 8. Установка указателя стека из основной прошивки
    __set_MSP(*(__IO uint32_t*)addr);

    // 9. Барьеры памяти для гарантии выполнения всех операций
    __DSB();
    __ISB();

    // 10. Получаем адрес Reset_Handler основной прошивки и прыгаем
    void (*JumpToApplication)(void);
    uint32_t JumpAddress;
    JumpAddress = *(__IO uint32_t*)(addr + 4);
    JumpToApplication = (void (*)(void))JumpAddress;
    JumpToApplication();

    // Этот код никогда не должен выполниться
    while (1);
}

void jump_to_adress() {
    responder_1();
    USART2_Send_settings(); // ответить  (ChipN_32, 0x88, 0x88, 0x88, 0x88)

    // получение адреса
    USART2_Receive_settings();
    responder_0();
    USART2_Send_settings(); // ответить  (полученный адрес)

    // если ок, то прыгаем
    USART2_Receive_settings();
    if (rx_s[0] == (uint8_t)response::ok) {
        ADRESS_FOR_JUMP = bytes_to_uint32_pointer(tx_s);
        flag_for_jump = 2;
    }
}

void USART2_IRQ() {
    if (LL_USART_IsActiveFlag_RXNE_RXFNE(USART2)) { // здесь прерывание, если адрес UART совпал
        communication();
        LL_USART_RequestEnterMuteMode(USART2); // ставит USART в MuteMode и сбрасывает флаг RWU.
    }
}

void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
    tx_s[0] = a;
    tx_s[1] = b;
    tx_s[2] = c;
    tx_s[3] = d;
    tx_s[4] = e;
}

void responder_0() {
    (uint32_t&)tx_s = bytes_to_uint32_pointer(rx_s);
}

void responder_1() {
    (uint32_t&)tx_s[1] = bytes_to_uint32_pointer(&rx_s[1]);
    tx_s[0] = (uint8_t)ChipN_32;
}

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
    tx_s[0] = (uint8_t)ChipN_32;
    tx_s[1] = (uint8_t)r;
    tx_s[2] = (uint8_t)r;
    tx_s[3] = (uint8_t)r;
    tx_s[4] = (uint8_t)r;
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    for (int i = 0; i < tx_settings_length; i++) {
        LL_USART_TransmitData9(USART2, tx_s[i]);
        while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    }
    while (!LL_USART_IsActiveFlag_TC(USART2)) {}
    LL_USART_DisableDirectionTx(USART2);
    LL_USART_ClearFlag_TC(USART2);
}

void USART2_Receive_settings_after_number() {
    while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
    rx_s[0] = LL_USART_ReceiveData9(USART2);
    for (int i = 0; i < rx_settings_length; i++) {
        while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
        rx_s[i] = LL_USART_ReceiveData9(USART2);
    }
    LL_USART_RequestRxDataFlush(USART2);
}

void USART2_Receive_settings() {
    while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
    for (int i = 0; i < rx_settings_length; i++) {
        while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
        rx_s[i] = LL_USART_ReceiveData9(USART2);
    }
    LL_USART_RequestRxDataFlush(USART2);
}


void Erase(uint32_t Page, uint32_t Banks) {
    HAL_FLASH_Unlock();
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
    HAL_FLASH_Lock();
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
        Set_tx_s((uint8_t)response::fail, 0x11, 0x11, 0x11, 0x11);
        USART2_Send_settings();
        return;
    }

    // Проверяем корректность параметров
    if (size == 0 || Data == NULL) {
        Set_tx_s((uint8_t)response::fail, 0x22, 0x22, 0x22, 0x22);
        USART2_Send_settings();
        return;
    }

    // Используем критическую секцию для безопасности вызова из прерывания
    uint32_t primask = __get_PRIMASK();
    // __disable_irq();

    HAL_FLASH_Unlock();

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

        // Вычисляем сколько байт осталось записать
        uint32_t remaining_bytes = size - (i * 8);

        // Формируем 64-битное значение из двух 32-битных
        // Little Endian: младшие биты идут первыми
        if (remaining_bytes >= 8) {
            // Полный doubleword - два uint32_t
            current_data = ((uint64_t)Data[idx]) | (((uint64_t)Data[idx + 1]) << 32);
        }
        else if (remaining_bytes >= 4) {
            // Есть хотя бы один полный uint32_t
            current_data = ((uint64_t)Data[idx]) | 0xFFFFFFFF00000000ULL;

            if (remaining_bytes > 4) {
                // Есть еще частичный второй uint32_t
                uint32_t partial = Data[idx + 1];
                uint32_t valid_bytes = remaining_bytes - 4;
                uint32_t mask = (1UL << (valid_bytes * 8)) - 1;
                partial &= mask;
                current_data = ((uint64_t)Data[idx]) | (((uint64_t)partial) << 32);
            }
        }
        else {
            // Меньше 4 байт - только часть первого uint32_t
            uint32_t partial = Data[idx];
            uint32_t mask = (1UL << (remaining_bytes * 8)) - 1;
            partial &= mask;
            current_data = (uint64_t)partial;
            // Заполняем остаток в текущем doubleword значением 0xFF
            for (uint32_t b = remaining_bytes; b < 8; b++) {
                current_data |= ((uint64_t)0xFF) << (b * 8);
            }
        }

        // Выполняем запись
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_address, current_data);

        // Барьер памяти после записи
        __DMB();

        FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }

    // Барьер памяти перед завершением
    __DMB();
    HAL_FLASH_Lock();

    __set_PRIMASK(primask);
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


static inline void uint32_to_bytes_pointer(uint32_t value, uint8_t* bytes) {
    *((uint32_t*)bytes) = value; // требуется выравнивание !
}

static inline uint32_t bytes_to_uint32_pointer(const uint8_t* bytes) {
    return *((uint32_t*)bytes); // требуется выравнивание !
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
