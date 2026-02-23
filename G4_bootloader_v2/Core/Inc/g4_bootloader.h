#pragma once

#include "stm32g4xx_hal.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_usart.h"

#ifdef __cplusplus
extern "C" {

    void USART2_IRQ();
    void g4_bootloader();

    enum class command_bootloader {
        echo = 11,
        reset_bootloader, // 200ms delay
        data_from_H7_to_array_g4,
        copy_array_to_flash_g4,
        jump_to_piano_g4, // прыжок по зашитому в g4 адресу
        jump_g4_to_adress // прыжок по указанному адресу
    };

    enum class chip_states {
        bootloader = 1,
        piano
    };

    enum class response {
        ok = 0x0C,
        fail = 0xDD
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
    void USART2_Receive_settings_after_number();
    void USART2_Receive_settings();
    void Erase(uint32_t Page, uint32_t Banks);
    void Flash_uint32(uint32_t Address, volatile uint32_t* Data, uint32_t size);
    void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size);
    void Reset();
    void Jump_to_app(const uint32_t addr);
    void jump_to_adress();
    void From_H7_to_array_g4();
    void Copy_array_to_flash();
    void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
    void responder_0();
    void responder_1();
    FlashPageInfo_t GetPageAndBank(uint32_t Address, uint32_t Size);
    static inline void uint32_to_bytes_pointer(uint32_t value, uint8_t* bytes);
    static inline uint32_t bytes_to_uint32_pointer(const uint8_t* bytes);

}
#endif