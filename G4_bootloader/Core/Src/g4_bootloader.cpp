#include "g4_bootloader.h"

uint8_t ChipN[1] = { 1 }; // <<<<< номер чипа указать здесь (который запишется в FLASH в первый раз)
volatile uint8_t ChipNumber[1] = {}; // <<<<< номер чипа - использовать в коде
const int ADRESS_CHIP_NUMBER = 0x08009800; // Page 19, BANK_1 // здесь храним номер чипа


// уточнить здесь адрес, где будет находится прошивка. Выяснить страницу и банк
#define MAIN_FIRMWARE 0x08010000 // здесь основная прошивка // TODO 

// bin_data
uint8_t bin_data[2048] = {}; // массив для получения прошивки
const int bin_data_length = sizeof(bin_data);

// settings
uint8_t tx_settings[5] = { };
const int tx_settings_length = sizeof(tx_settings);
uint8_t rx_settings[5] = { };
const int rx_settings_length = sizeof(rx_settings);
uint8_t t1 = 0;
uint8_t t2 = 0;
uint8_t t3 = 0;
uint8_t t4 = 0;

void g4_bootloader() {

    // считываем номер чипа из памяти (или если надо записываем)
    // Erase(19, FLASH_BANK_1); // очистить номер чипа, если надо обновить номер
    Read(ADRESS_CHIP_NUMBER, const_cast<uint8_t*>(ChipNumber), 1u);
    if (ChipNumber[0] == 0xFF) {
        Erase(19, FLASH_BANK_1); // очистить номер чипа
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

    tx_settings[0] = ChipNumber[0];
    rx_settings[0] = ChipNumber[0];




    while (1) {}
}

void USART2_IRQ() {
    if (LL_USART_IsActiveFlag_RXNE_RXFNE(USART2)) { // здесь прерывание, если адрес UART совпал
        communication();
        LL_USART_RequestRxDataFlush(USART2);   // сбрасывает RXNE флаг
        LL_USART_RequestEnterMuteMode(USART2); // ставит USART в MuteMode и сбрасывает флаг RWU.
    }
}

void communication() {
    USART2_Receive_settings();
    const uint8_t rx_command = rx_settings[0];
    if (rx_command == command::reset) {
        Reset();
    }

    if (rx_command == command::jump_to_piano_g4) {
        Jump_to_app();
    }

}

// UART Send-Recive
void USART2_Send_settings(const uint8_t* compN, const uint8_t* dot, const int* value) {
    LL_USART_EnableDirectionTx(USART2); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    // conv16to8x2 c;
    // c = convert16to8x2(value);
    // tx_settings[1] = { compN };
    // tx_settings[2] = { dot };
    // tx_settings[3] = c.a;
    // tx_settings[4] = c.b;
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    for (int i = 0; i < tx_settings_length; i++) {
        LL_USART_TransmitData9(USART2, tx_settings[i]);
        while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    }
    while (!LL_USART_IsActiveFlag_TC(USART2)) {}
    LL_USART_DisableDirectionTx(USART2);
    LL_USART_ClearFlag_TC(USART2);
}

void USART2_Receive_settings() {
    while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
    rx_settings[0] = LL_USART_ReceiveData9(USART2);
    for (int i = 0; i < rx_settings_length; i++) {
        while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
        rx_settings[i] = LL_USART_ReceiveData9(USART2);
    }
    t1 = rx_settings[1];
    t2 = rx_settings[2];
    t3 = rx_settings[3];
    t4 = rx_settings[4];
    LL_USART_RequestRxDataFlush(USART2);
    LL_USART_RequestEnterMuteMode(USART2);
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

    // КРИТИЧНО: Необходимо дождаться завершения операции стирания!
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    // КРИТИЧНО: Очистить биты управления стиранием (PER и PNB)
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));

    // Очистить флаги ошибок, если они есть
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
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

    // Вычисляем количество DOUBLEWORD для записи
    // Каждое DOUBLEWORD = 8 байт
    uint32_t doublewords_count = (size + 7) / 8; // Округление вверх

    // Записываем данные в цикле
    for (uint32_t i = 0; i < doublewords_count; i++) {
        uint32_t current_address = Address + (i * 8);
        uint64_t current_data;

        // Если это последний элемент и size не кратен 8, обрабатываем остаток
        if (i == doublewords_count - 1 && (size % 8) != 0) {
            current_data = 0xFFFFFFFFFFFFFFFF; // Заполняем FF (стертое состояние Flash)
            uint8_t* src = (uint8_t*)&Data[i];
            uint8_t* dst = (uint8_t*)&current_data;
            uint32_t remaining_bytes = size % 8;
            for (uint32_t j = 0; j < remaining_bytes; j++) {
                dst[j] = src[j];
            }
        }
        else {
            current_data = Data[i];
        }

        // Выполняем запись с проверкой статуса
        HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_address, current_data);
        FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

        // Проверяем статус и обрабатываем ошибку:
        if (status != HAL_OK) {
            // Обработка ошибки записи
            // uint32_t error = HAL_FLASH_GetError();
            Lock();
            return;
        }
    }

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

void Reset() {
    // LL_mDelay(20); // ?
    NVIC_SystemReset();
}

void Jump_to_app() {
    void (*JumpToApplication)(void);
    uint32_t JumpAddress;
    JumpAddress = *(__IO uint32_t*) (MAIN_FIRMWARE + 4); // здесь должно быть " + 4 ", а в скрипте компоновщика " - 8 " !!
    JumpToApplication = (void*)JumpAddress;
    JumpToApplication();
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

