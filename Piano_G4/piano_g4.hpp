
/*
 * midi_keyboard.h
 *
 *  Created on: Mar 7, 2025
 *      Author: sche
 */

#pragma once

#include "main.h"

#ifdef __cplusplus
extern "C" {

    using uint = unsigned int;
    using cuint = const uint;

    const int chipAdress = 3; // для NoteOff что-то придумать надо здесь! 4-5-6-on, 18-19-20-off
    const int allChipCount = 26;
    const uint8_t note = chipAdress * 7 + 25;
    // !!!  ***  !!!   подтяжки линий (в UART нужен UP для Tx у одного из g4)+(UP для линии прерываний)   !!!  ***  !!!

    // data
    uint8_t tx_data[3] = {};
    const int tx_dataLength = sizeof(tx_data);

    // settings
    uint8_t tx_settings[5] = { chipAdress, };
    const int tx_settings_length = sizeof(tx_settings);
    uint8_t rx_settings[5] = { chipAdress, };
    const int rx_settings_length = sizeof(rx_settings);
    uint8_t compN_ = 0;
    uint8_t dot_ = 0;
    uint8_t a_ = 0;
    uint8_t b_ = 0;

    volatile uint32_t tim[7][2] = {};
    uint32_t tim_off[7][2] = {};
    int comp[7][2] = { {3500, 1000}, {3500, 1000}, {3500, 1000}, {3500, 1000}, {3500, 1000}, {3500, 1000}, {3500, 1000} };
    int def[2] = { 3500, 1000 };
    volatile unsigned int f[7] = { 0 };
    int fl = 1;
    uint divisible = 100'000'000; // 8 200 000
    const unsigned int maxMidi = 127;

    enum command {
        sync_timer = 1,
        cal,
        read_comp_value,
        set_comp_value,
        //set_div_value, // реализовать // TODO
        buff_to_flash,         //
        flash_to_buff,         //
        read_comp_value_flash, //
    };

    struct conv16to8x2 {
        uint8_t a = 0;
        uint8_t b = 0;
    };

    class notes {
    public:
        void set(uint8_t nn, uint32_t t1, uint32_t t2);
        uint8_t n = 0;
        uint8_t a = 0;
        uint8_t b = 0;
    };

    void communication();
    void USART2_Send(uint8_t compN, uint8_t dot, int value);
    void USART2_Receive();
    void calibration();
    void TIM8_UP_IRQ();
    void TIM8_CC_IRQ();
    void USART2_IRQ();
    void pause(int p);
    int convert8x2to16(uint8_t a, uint8_t b);
    conv16to8x2 convert16to8x2(int a);
    // void conv32to8(); // реализовать // TODO
    // void conv8to32(); // реализовать // TODO
    void setCOMP_extVoltage_Up(int compN);
    void setCOMP_extVoltage_Down(int compN);
    void comp_ext_enable(int compN);
    void comp_ext_disable(int compN);
    void comp_set_data(int compN, int dot);
    void COMP1_2_3_IRQ();
    void COMP4_5_6_IRQ();
    void COMP7_IRQ();
    void iffing(int a);
    void SPI1_SendData(uint16_t data);
    COMP_TypeDef* comp_def(const int i);
    void debugg();

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
