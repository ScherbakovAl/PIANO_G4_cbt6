#pragma once

#include "stm32g4xx_hal.h"

// #include "stm32g4xx_ll_rcc.h"
// #include "stm32g4xx_ll_bus.h"
// #include "stm32g4xx_ll_crs.h"
// #include "stm32g4xx_ll_system.h"
// #include "stm32g4xx_ll_exti.h"
// #include "stm32g4xx_ll_cortex.h"
// #include "stm32g4xx_ll_utils.h"
// #include "stm32g4xx_ll_pwr.h"
// #include "stm32g4xx_ll_dma.h"
// #include "stm32g4xx_ll_usart.h"
// #include "stm32g4xx_ll_gpio.h"


#ifdef __cplusplus
extern "C" {
#endif

// Структура для хранения информации о странице и банке
typedef struct {
    uint32_t start_page;    // Начальная страница
    uint32_t end_page;      // Конечная страница (если диапазон охватывает несколько страниц)
    uint32_t bank;          // Банк памяти (FLASH_BANK_1 или FLASH_BANK_2)
    uint8_t  is_valid;      // Флаг валидности (1 - валидно, 0 - адрес вне диапазона Flash)
} FlashPageInfo_t;

void Unlock();
void Lock();
void Erase(uint32_t Page, uint32_t Banks);
void Flash(uint32_t Address, uint64_t* Data, uint32_t size);
void Read(uint32_t Address, uint8_t* pData, uint32_t Size);
FlashPageInfo_t GetPageAndBank(uint32_t Address, uint32_t Size);
// void ReadIf();

#ifdef __cplusplus
}
#endif