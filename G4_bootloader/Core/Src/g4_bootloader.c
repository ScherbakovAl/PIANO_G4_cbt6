#include "g4_bootloader.h"

void g4_bootloader() {

    Unlock();
    Erase();
    // while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {}
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    // FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    Lock();
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR | FLASH_FLAG_PROGERR |
        // FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
        // FLASH_FLAG_SIZERR | FLASH_FLAG_MISERR);

        // Ждем завершения операции
    // while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {__NOP();}
    // FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    // while (FLASH_GetBusyState() != RESET) {}
    // LL_mDelay(100);
    // while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY1)) {}

    Unlock();
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    // FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    int adress = 0x0800A000; // 0x08009FF0 - смотреть с этого // с адреса 0x0804 0000 - bank2
    uint64_t data = 0xFAFBECEDABAC8888;
    for (int i = 0; i < 0xFF; ++i) {
        Flash(adress++, data);
    }
    // FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
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

// Функция чтения
// void ReadFromFlash(uint64_t* buffer, uint32_t size) {
//     uint64_t* addr = (uint64_t*)FLASH_USER_START_ADDR;
//     for (uint32_t i = 0; i < size; i++) {
//         buffer[i] = addr[i];
//     }
// }


// пржок как пример:
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

    while (1) {}
}

void Unlock() {
    HAL_FLASH_Unlock();
}

void Lock() {
    HAL_FLASH_Lock();
}

void Erase() {
    // FLASH_EraseInitTypeDef* pp;
    // pp->TypeErase = FLASH_TYPEERASE_PAGES;
    // pp->Banks = FLASH_BANK_1;
    // pp->Page = 19; // начинать с этой страницы
    // pp->NbPages = 4; // количество стираемых страниц
    // uint32_t* PError = 0;
    // HAL_FLASHEx_Erase(pp, PError);
    // // FLASH_MassErase(FLASH_BANK_2);
    FLASH_PageErase(20, FLASH_BANK_1);
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
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address, Data); // FLASH_TYPEPROGRAM_DOUBLEWORD FLASH_TYPEPROGRAM_FAST FLASH_TYPEPROGRAM_FAST_AND_LAST
}




















