
/*
 * midi_keyboard.h
 *
 *  Created on: Mar 7, 2025
 *      Author: sche
 */

#pragma once

#include "main.h"
#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {

    using uint = unsigned int;
    using cuint = const uint;

    volatile uint32_t chipAdress = 0; // 1-13-on, 14-26-off
    const int allChipCount = 27; // до этого значения считает таймер

    // !!!  ***  !!!   подтяжки линий (в UART нужен UP для Tx у одного из g4)+(UP для линии прерываний)   !!!  ***  !!!

    // data
    uint8_t tx_data[4] = { }; // TODO uint8_t->uint32_t ??? !!! ->  45: LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, <<<(uint32_t)>>> tx_data); <<<< !!!!
    const int tx_dataLength = sizeof(tx_data);

    // settings
    uint8_t tx_settings[5] = { };
    const int tx_settings_length = sizeof(tx_settings);
    uint8_t rx_settings[5] = { };
    const int rx_settings_length = sizeof(rx_settings);
    uint8_t compN_ = 0;
    uint8_t dot_ = 0;
    uint8_t a_ = 0;
    uint8_t b_ = 0;

    uint32_t windowReTRigger1 = 0;
    uint32_t windowReTRigger2 = 0;

    volatile uint32_t tim[7][2] = { };
    uint32_t tim_off[7][2] = { };
    int comp[7][3] = { };
    int comp_calibration[7] = { };
    int cc_fl[7] = { }; // флаг. Проверять состояние данного компаратора при калибровке или нет
    int def_on[2] = { 2200, 2000 };
    int def_off[3] = { 1800, 3400, 1600 }; // [0]-1я точка, затем 2я. [2] - точка переключения направления срабатывания компаратора
    struct default_comp { // TODO
        int green_on = 2200;
        int red_on = 2000;
        int green_off = 1800;
        int red_off = 3400;
        int grey_off = 1600;
    };
    volatile int8_t f[7] = { 0 };
    volatile int fl = 1;
    volatile int flag_Service_command = 1;

    enum class command {
        sync_timer = 1,
        cal,
        read_comp_value,
        set_comp_value,
        read_comp_setting, // TODO реализовать
        all_calib,
        reset_to_bootloader,
        mute,
        unmute,
        echo = 11
    };

    enum class subcommand {
        start_calibration = 1,
        read_calibration,
        stop_calibration,
        working,
        nothing,
        exit
    };

    enum class chip_states {
        bootloader = 1,
        piano // main
    };

    struct conv16to8x2 {
        uint8_t a = 0;
        uint8_t b = 0;
    };

    struct nt {
        uint8_t n = 0;
        uint32_t timelegth = 0;
    };

    struct abcd {
        uint8_t a = 0;
        uint8_t b = 0;
        uint8_t c = 0;
    };

    subcommand service_commands = subcommand::working;
    int service_flag = 0;

    void check();
    void service();
    void resetData();
    void communication();
    void calibration();
    void all_calibration();
    void mute();
    void unmute();
    void USART2_Send(const uint8_t& compN, const uint8_t& dot, const int& value);
    void USART2_Receive();
    void TIM8_CC_IRQ();
    void USART2_IRQ();
    uint32_t timeLength(const uint& t1, const uint& t2);
    void pause(const uint& p);
    int convert8x2to16(const uint8_t& a, const uint8_t& b);
    conv16to8x2 convert16to8x2(const int& a);
    abcd conv32to8(const nt& x);
    void setCOMP_extVoltage_Up(const int& compN);
    void setCOMP_extVoltage_Down(const int& compN);
    void set_ALL_COMP_extVoltage_Up();
    void set_ALL_COMP_extVoltage_Down();
    void comp_ext_enable(const int& compN);
    void comp_ext_disable(const int& compN);
    void comp_set_data(const int& compN, const int& data);
    uint32_t comp_read_data(const int& compN);
    void COMP1_2_3_IRQ();
    void COMP4_5_6_IRQ();
    void COMP7_IRQ();
    // void iffing(const int& a);
    COMP_TypeDef* comp_def(const int& i);
    void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size);
    void jump_off(const int& a);
    void jump_on(const int& a);

}
#endif // extern "C"

// GPIOA->BSRR = 0x200; // pA9
// GPIOA->BSRR = 0x2000000; // pA9
// GPIOA->BSRR = 0x400; // pA10
// GPIOA->BSRR = 0x4000000; // pA10
// GPIOA->BSRR = 0x800; // pA11
// GPIOA->BSRR = 0x8000000; // pA11
// GPIOA->BSRR = 0x1000; // pA12
// GPIOA->BSRR = 0x10000000; // pA12
// GPIOA->BSRR = 0x8000; // pA15
// GPIOA->BSRR = 0x80000000; // pA15
// GPIOB->BSRR = 0x8; // pB3
// GPIOB->BSRR = 0x80000; // pB3
// GPIOB->BSRR = 0x10; // pB4
// GPIOB->BSRR = 0x100000; // pB4
// GPIOB->BSRR = 0x20; // pB5
// GPIOB->BSRR = 0x200000; // pB5
// GPIOB->BSRR = 0x80; // pB7
// GPIOB->BSRR = 0x800000; // pB7
// GPIOB->BSRR = 0x1000; // pB12
// GPIOB->BSRR = 0x10000000; // pB12
// GPIOB->BSRR = 0x8000; // pB15
// GPIOB->BSRR = 0x80000000; // pB15
// GPIOC->BSRR = 0x40; // pC6 led
// GPIOC->BSRR = 0x400000; // pC6 led
// GPIOA->BSRR = 0x100; // pA8 // tim interrupter
// GPIOA->BSRR = 0x1000000; // pA8 // tim interrupter
