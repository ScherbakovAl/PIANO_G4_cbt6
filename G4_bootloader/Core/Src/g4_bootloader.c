#include "g4_bootloader.h"

void g4_bootloader() {


    Erase(20, FLASH_BANK_1);


    // ВАЖНО: Адрес для DOUBLEWORD должен быть выровнен по 8 байт!
    uint32_t address = 0x0800A000; // 0x08009FF0 - смотреть с этого // с адреса 0x0804 0000 - bank2
    
    // uint64_t data = 0xFAFBECEDABAC8888;

    uint64_t data[4] = {
        0x1122334455667788,  // В памяти: 0x88 0x77 0x66 0x55 0x44 0x33 0x22 0x11
        0x8877665544332211,  // В памяти: 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
        0xAABBCCDDFEFE0011,  // В памяти: 0x11 0x00 0xFF 0xEE 0xDD 0xCC 0xBB 0xAA
        0x11FFEEDDCCBBAA00   // В памяти: 0x00 0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x11
    };

    // uint32_t data[4] = {
    // 0x12345678,  // В памяти: 0x88 0x77 0x66 0x55 0x44 0x33 0x22 0x11
    // 0x87654321,  // В памяти: 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88
    // 0xABCDEF11,  // В памяти: 0x11 0x00 0xFF 0xEE 0xDD 0xCC 0xBB 0xAA
    // 0x22FEDCBA   // В памяти: 0x00 0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x11
    // };

    Flash(address, data[0], 1);
    address += 8;  // Увеличиваем адрес на 8 байт (размер DOUBLEWORD)

    Flash(address, data[1], 1);
    address += 8;  // Увеличиваем адрес на 8 байт (размер DOUBLEWORD)

    Flash(address, data[2], 1);

    while (1) {}
}

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

void Flash(uint32_t Address, uint64_t Data, uint32_t size) {
    // ВАЖНО: Проверяем, что адрес выровнен по 8 байт для DOUBLEWORD
    if (Address % 8 != 0) {
        // Ошибка выравнивания! Адрес должен быть кратен 8
        return;
    }
    Unlock();
    // Выполняем запись с проверкой статуса
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, Data);
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    // Можно проверить статус и обработать ошибку:
    // if (status != HAL_OK) {
    //     // Обработка ошибки записи
    //     uint32_t error = HAL_FLASH_GetError();
    // }
    Lock();
}

void ReadFromFlash(uint32_t Address, uint8_t* pData, uint32_t Size) {

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

void ReaIf() {

      // if ((*(volatile uint32_t*)(Flash_Address + 0x640)) != key_to_change_memory) {
      // 	SaveToMemory();
      // }
      // else {
      // for (int i = 0; i < sizeCHART_BUFFER; ++i) {
      // 	compsCHART_0[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)));
      // 	compsCHART_1[i] = *(volatile uint32_t*)(Flash_Address + (i * sizeof(uint32_t)) + 0x320);
      // }
}

// пометочки..
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

