#pragma once

#include "stm32g4xx_hal.h"

// #include "stm32g4xx_ll_rcc.h"
// #include "stm32g4xx_ll_bus.h"
// #include "stm32g4xx_ll_crs.h"
// #include "stm32g4xx_ll_system.h"
// #include "stm32g4xx_ll_exti.h"
// #include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
// #include "stm32g4xx_ll_pwr.h"
// #include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_usart.h"
// #include "stm32g4xx_ll_gpio.h"






enum command {
    reset = 10,
    jump_to_piano_g4,
    set_number,
    data_from_H7_to_array_g4,
    data_from_array_g4_to_H7,
    copy_array_to_flash,
    echo
};

enum response {
    ok = 0xCB,
    fail = 0x99
};

// Структура для хранения информации о странице и банке
typedef struct {
    uint32_t start_page;    // Начальная страница
    uint32_t end_page;      // Конечная страница (если диапазон охватывает несколько страниц)
    uint32_t bank;          // Банк памяти (FLASH_BANK_1 или FLASH_BANK_2)
    uint8_t  is_valid;      // Флаг валидности (1 - валидно, 0 - адрес вне диапазона Flash)
} FlashPageInfo_t;

void communication();
void USART2_Send_settings();
void USART2_Send_responce(response);
void USART2_Receive_settings();
void USART2_Receive_settings2();
void Unlock();
void Lock();
void Erase(uint32_t Page, uint32_t Banks);
void Flash(uint32_t Address, uint64_t* Data, uint32_t size);
void Read(uint32_t Address, uint8_t* pData, uint32_t Size);
void Flash_uint32(uint32_t Address, volatile uint32_t* Data, uint32_t size);
void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size);
void Reset();
void Jump_to_app();
// void Set_N(uint32_t n);
void From_H7_to_array_g4();
void From_array_g4_to_H7();
void Copy_array_to_flash();
void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
FlashPageInfo_t GetPageAndBank(uint32_t Address, uint32_t Size);
// void ReadIf();


#ifdef __cplusplus
extern "C" {
#endif

    void USART2_IRQ();
    void g4_bootloader();

#ifdef __cplusplus
}
#endif
