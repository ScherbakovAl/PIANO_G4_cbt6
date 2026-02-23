/*
 * midi_keyboard.cpp
 *
 *  Created on: Mar 6, 2025
 *      Author: sche
 */

#include "piano_g4.hpp"
#include "stm32g4xx.h"
#include "deque"

std::deque<nt> dequeOn;

// static const uint32_t ADRESS_CHIP_NUMBER_older = 0x08003800;
static const uint32_t ADRESS_CHIP_NUMBER = 0x08004800; // здесь храним номер чипа

using JumpFuncPtr = void(*)(const int& a, const uint32_t& t); // подмена функции jump_func на jump_on или jump_off
JumpFuncPtr jump_func = nullptr;

// main
void g4() {
    Read_uint32(ADRESS_CHIP_NUMBER, &chipAdress, 4);


    // FlashPageInfo_t info = GetPageAndBank(ADRESS_CHIP_NUMBER, 4);
    // Erase(info.start_page, info.bank); // очистить номер чипа
    // Flash_uint32(ADRESS_CHIP_NUMBER, &chipAdress, 4);


    static const uint8_t note = chipAdress * 7;
    tx_settings[0] = chipAdress;
    rx_settings[0] = chipAdress;


    if (chipAdress > 13) { // выбор функции для прыжка для on или off
        jump_func = jump_off;
    }
    else {
        jump_func = jump_on;
    }

    reg_init();

    GPIOB->BSRR = 0x80; // pB7 // tim interrupter // чтобы включился счёт таймера с H7

    // TIM init
    LL_TIM_EnableCounter(TIM2);               // основной счётчик (100nS)
    LL_TIM_EnableCounter(TIM5);               // доп счётчик (p100uS)
    LL_TIM_SetAutoReload(TIM8, allChipCount); // PWM - tim clk
    LL_TIM_OC_SetCompareCH3(TIM8, chipAdress);
    LL_TIM_EnableCounter(TIM8); // PWM - tim clk

    // UART init
    LL_USART_DisableDirectionTx(USART2); // один или несколько MCU должны быть подтянуты к pull_UP ! // надо ли? проверить..
    LL_USART_Disable(USART2);            // // надо ли? проверить..
    LL_USART_ConfigNodeAddress(USART2, LL_USART_ADDRESS_DETECT_7B, chipAdress);
    LL_USART_Enable(USART2);
    LL_USART_DisableDirectionTx(USART2);   // один или несколько MCU должны быть подтянуты к pull_UP !
    LL_USART_ClearFlag_TC(USART2);         // чтобы при включении не срабатывало прерывание
    LL_USART_EnableIT_RXNE_RXFNE(USART2);  // включение прерывания по приему данных
    LL_USART_EnableMuteMode(USART2);       // включение MuteMode
    LL_USART_RequestEnterMuteMode(USART2); // ставит USART в MuteMode и сбрасывает флаг RWU.
    LL_USART_EnableDMAReq_TX(USART2);      // подсоединяет DMA к UART
    LL_USART_EnableIT_TC(USART2);          // прерывание окончания передачи

    // DMA TX
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t) & (USART2->TDR));
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)tx_data);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, tx_dataLength);

    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);
    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_1);
    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_2);
    LL_DAC_Enable(DAC4, LL_DAC_CHANNEL_1);
    LL_DAC_Enable(DAC4, LL_DAC_CHANNEL_2);
    LL_DAC_Enable(DAC2, LL_DAC_CHANNEL_1);


    for (int i = 0; i < 7; ++i) { // установки по-умолчанию
        if (chipAdress > 13) {
            comp[i][0] = def_comp.green_off;
            comp[i][1] = def_comp.red_off;
            comp[i][2] = def_comp.grey_off;
        }
        else {
            comp[i][0] = def_comp.green_on;
            comp[i][1] = def_comp.red_on;

        }
        comp_calibration[i] = 0;
        comp_set_data(i, comp[i][0]);
        LL_COMP_Enable(comp_def(i));
    }
    //---------------------------------

// можно для неполных сделать точку срабатывания высоко, выше 1000 и не будет компаратор срабатывать..

// 136Mhz - speed cheep
// TIM2 - presc = 12    - 100ns период
// TIM5 - presc = 13599 - 100us период

    if (chipAdress > 13) {
        // set_ALL_COMP_extVoltage_Up(); // для ship OFF
        windowReTRigger1 = 10 * 60; // = 60ms // TIM5 (p100us) // TODO провериь бОльшие значения
        windowReTRigger2 = 10 * 60; // = 60ms // TIM5 (p100us)
    }
    else { // ship ON
        windowReTRigger1 = 10 * 30; // = 30ms // TIM5 (p100us)
        windowReTRigger2 = 10 * 50; // = 50ms // TIM5 (p100us)
    }

    USART2_Send(compN_, dot_, 0);

    if (chipAdress > 13) {
        while (1) {
            // off
            for (uint8_t i = 0; i < 7; ++i) {
                if (f[i] != 0) { // for note off
                    if (f[i] == 1) {
                        setCOMP_extVoltage_Up(i);
                        comp_set_data(i, comp[i][0]);
                        f[i] = 2;
                        comp_ext_enable(i);
                    }
                    if (f[i] == 3) {
                        tim_off[i][0] = TIM5->CNT;
                        comp_set_data(i, comp[i][1]);
                        f[i] = 4;
                        comp_ext_enable(i);

                    }
                     // TODO видимо эту    vvvvv    секцию надо удалить      vvvvv
                    // if (f[i] == 4 && TIM5->CNT - tim_off[i][0] > windowReTRigger1) {
                    //     comp_ext_disable(i);
                    //     comp_set_data(i, comp[i][2]); // ?
                    //     setCOMP_extVoltage_Down(i); // ?
                    //     f[i] = 0; // ?
                    //     comp_ext_enable(i);
                    // }
                    if (f[i] == 5) {
                        tim_off[i][1] = TIM5->CNT;
                        f[i] = 6;
                        nt n{ note + i, timeLength(tim[i][0], tim[i][1]) };
                        dequeOn.push_back(n);
                    }
                    if (f[i] == 6 && TIM5->CNT - tim_off[i][1] > windowReTRigger2) {
                        comp_set_data(i, comp[i][2]);
                        setCOMP_extVoltage_Down(i);
                        f[i] = 0;
                        comp_ext_enable(i);
                    }
                }
            }
            check();
            service();
        }
    }
    else {
        while (1) {
            // on
            for (uint8_t i = 0; i < 7; ++i) {
                if (f[i] != 0) { // for note on
                    if (f[i] == 1) {
                        tim_off[i][0] = TIM5->CNT; // TIM5 (p100us)
                        comp_set_data(i, comp[i][1]);
                        f[i] = 2;
                        comp_ext_enable(i);
                    }
                    if (f[i] == 2 && TIM5->CNT - tim_off[i][0] > windowReTRigger1) {
                        comp_ext_disable(i);
                        comp_set_data(i, comp[i][0]);
                        f[i] = 0;
                        comp_ext_enable(i);
                    }
                    if (f[i] == 3) {
                        tim_off[i][1] = TIM5->CNT;
                        f[i] = 4;
                        nt n{ note + i, timeLength(tim[i][0], tim[i][1]) };
                        dequeOn.push_back(n);
                    }
                    if (f[i] == 4 && TIM5->CNT - tim_off[i][1] > windowReTRigger2) {
                        comp_set_data(i, comp[i][0]);
                        f[i] = 0;
                        comp_ext_enable(i);
                    }
                }
            }
            check();
            service();
        }
    }
}

void check() {
    if (!dequeOn.empty() && fl) {
        const nt& x = dequeOn.front();
        uint32_t u = x.timelegth;
        tx_data[0] = x.note;
        tx_data[1] = (u & 0xff << 16) >> 16;
        tx_data[2] = (u & 0xff << 8) >> 8;
        tx_data[3] = u & 0xff;
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, tx_dataLength);
        LL_USART_ClearFlag_TC(USART2);
        LL_TIM_ClearFlag_CC3(TIM8); // PWM - tim clk
        LL_TIM_EnableIT_CC3(TIM8);  // PWM - tim clk
        dequeOn.pop_front();
        fl = 0;
    }
}

void service() {
    while (service_flag) {
        if (service_commands == subcommand::working && flag_Service_command) {
            for (uint8_t i = 0; i < 7; ++i) {
                cc_fl[i] = 1;
            }
            for (int a = 0; a < 4092; ++a) {
                for (uint8_t i = 0; i < 7; ++i) {
                    if (cc_fl[i]) {
                        comp_set_data(i, a);
                        if (LL_COMP_ReadOutputLevel(comp_def(i)) == LL_COMP_OUTPUT_LEVEL_LOW) {
                            comp_calibration[i] = a;
                            cc_fl[i] = 0;
                        }
                        if (a > 4090) {
                            comp_calibration[i] = 0;
                        }
                    }
                }
            }
            flag_Service_command = 0;
        }
        if (service_commands == subcommand::exit) {
            if (!dequeOn.empty()) dequeOn.clear();
            for (int i = 0; i < 7; ++i) {
                f[i] = 0;
                comp_set_data(i, comp[i][0]);
                comp_ext_enable(i);
            }
            service_flag = 0;
            USART2_Send(compN_, 0, (int)subcommand::stop_calibration);
        }
    }
}

void resetData() {
    // if (!dequeOn.empty()) dequeOn.clear(); // TODO нельзя это очищать из прерывания!!!
    for (uint8_t i = 0; i < 7; ++i) {
        f[i] = 0;
        comp_ext_disable(i);
        comp_set_data(i, comp[i][0]);
        comp_ext_enable(i);
    }
}

void comp_refresh(uint8_t& n) {
    comp_ext_disable(n);
    comp_set_data(n, comp[n][0]);
    comp_ext_enable(n);
}

void communication() {
    USART2_Receive();

    if (rx_settings[0] == (uint8_t)command::sync_timer) {
        TIM8->CNT = 0;  // PWM - tim clk
        resetData();
        USART2_Send(compN_, dot_, 0);
    }

    if (rx_settings[0] == (uint8_t)command::cal) {
        calibration();
        resetData();
    }

    if (rx_settings[0] == (uint8_t)command::read_comp_value) {
        USART2_Send(compN_, dot_, comp[compN_][dot_]);
    }

    if (rx_settings[0] == (uint8_t)command::set_comp_value) {
        comp[compN_][dot_] = convert8x2to16(a_, b_);
        USART2_Send(compN_, dot_, comp[compN_][dot_]);
        f[compN_] = 0;
    }

    if (rx_settings[0] == (uint8_t)command::read_comp_setting) {
        pause(4);
        USART2_Send(compN_, dot_, comp_read_data(compN_));
    }

    if (rx_settings[0] == (uint8_t)command::all_calib) {
        all_calibration();
    }

    if (rx_settings[0] == (uint8_t)command::reset_to_bootloader) {
        LL_mDelay(20);
        NVIC_SystemReset();
    }

    if (rx_settings[0] == (uint8_t)command::mute) {
        mute();
        USART2_Send(0, 0, (int)chip_states::piano);
    }

    if (rx_settings[0] == (uint8_t)command::unmute) {
        unmute();
        USART2_Send(0, 0, (int)chip_states::piano);
    }

    if (rx_settings[0] == (uint8_t)command::echo) {
        USART2_Send(0, 0, (int)chip_states::piano);
    }
}

// Calibrations
void calibration() {
    comp_ext_disable(compN_);
    comp_set_data(compN_, 0);
    f[compN_] = 0;
    for (int a = 0; a < 4092; ++a) {
        comp_set_data(compN_, a);
        if (LL_COMP_ReadOutputLevel(comp_def(compN_)) == LL_COMP_OUTPUT_LEVEL_LOW) {
            USART2_Send(compN_, dot_, a);
            break;
        }
        if (a > 4090) {
            USART2_Send(compN_, dot_, 0);
            break;
        }
    }
    f[compN_] = 0;
    comp_set_data(compN_, comp[compN_][0]);
    comp_ext_enable(compN_);
}

void all_calibration() {
    if (b_ == (uint8_t)subcommand::start_calibration) {
        for (int i = 0; i < 7; ++i) {
            f[i] = 0;
            comp_ext_disable(i);
        }
        service_commands = subcommand::working;
        service_flag = 1;
        USART2_Send(compN_, 0, (int)subcommand::start_calibration);
    }
    else if (b_ == (uint8_t)subcommand::read_calibration) {
        pause(4);
        USART2_Send(compN_, 0, (int)subcommand::read_calibration);
        for (int i = 0; i < 7; ++i) {
            pause(6);
            USART2_Send(i, 0, comp_calibration[i]);
        }
        flag_Service_command = 1;
    }
    else if (b_ == (uint8_t)subcommand::stop_calibration) {
        service_commands = subcommand::exit;
    }
}

void mute() {
    for (int a = 0; a < 7; ++a) {
        comp_ext_disable(a);
    }
}

void unmute() {
    for (int a = 0; a < 7; ++a) {
        comp_ext_enable(a);
    }
}

// UART Send-Recive
void USART2_Send(const uint8_t& compN, const uint8_t& dot, const int& value) {
    LL_USART_EnableDirectionTx(USART2); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    conv16to8x2 c;
    c = convert16to8x2(value);
    tx_settings[1] = { compN };
    tx_settings[2] = { dot };
    tx_settings[3] = c.a;
    tx_settings[4] = c.b;
    pause(2);
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    for (int i = 0; i < tx_settings_length; i++) {
        LL_USART_TransmitData9(USART2, tx_settings[i]);
        while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    }
    while (!LL_USART_IsActiveFlag_TC(USART2)) {}
    LL_USART_DisableDirectionTx(USART2);
    LL_USART_ClearFlag_TC(USART2);
}

void USART2_Receive() {
    while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
    rx_settings[0] = LL_USART_ReceiveData9(USART2);
    for (int i = 0; i < rx_settings_length; i++) {
        while (!LL_USART_IsActiveFlag_RXNE(USART2)) {}
        rx_settings[i] = LL_USART_ReceiveData9(USART2);
    }
    compN_ = rx_settings[1];
    dot_ = rx_settings[2];
    a_ = rx_settings[3];
    b_ = rx_settings[4];
    LL_USART_RequestRxDataFlush(USART2);
    LL_USART_RequestEnterMuteMode(USART2);
}

// TIM8 IRQ Handler
void TIM8_CC_IRQ() { // это прерывание срабатывает от TIM8 по номеру контроллера
    GPIOB->BSRR = 0x800000; //pB7 // tim interrupter
    LL_TIM_ClearFlag_CC3(TIM8);  // PWM - tim clk
    LL_USART_EnableDirectionTx(USART2); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
}

// UART IRQ Handler
void USART2_IRQ() {
    if (LL_USART_IsActiveFlag_TC(USART2)) {  // здесь прерывание, если завершена передача данных
        LL_USART_DisableDirectionTx(USART2); // один или несколько должны быть подтянуты к pull_UP !
        LL_USART_ClearFlag_TC(USART2);       // идёт вслед за DisableDirectionTx, иначе прерывается ещё раз
        LL_TIM_DisableIT_CC3(TIM8);  // PWM - tim clk
        fl = 1;
        GPIOB->BSRR = 0x80; //pB7 // tim interrupter
    }
    else if (LL_USART_IsActiveFlag_RXNE_RXFNE(USART2)) { // здесь прерывание, если адрес UART совпал
        GPIOB->BSRR = 0x800000; //pB7 // tim interrupter
        communication();
        LL_USART_RequestRxDataFlush(USART2);   // сбрасывает RXNE флаг (или uint8_t data = LL_USART_ReceiveData8(USART2); сбрасывает)
        LL_USART_RequestEnterMuteMode(USART2); // ставит USART в MuteMode и сбрасывает флаг RWU.
        GPIOB->BSRR = 0x80; //pB7 // tim interrupter
    }
}

// (100ns)
void pause(const uint& p) {
    TIM2->CNT = 0;
    while (TIM2->CNT < p) {}
}

uint32_t timeLength(const uint& t1, const uint& t2) {
    uint32_t tOut;
    if (t1 > t2) {
        tOut = (4'294'967'295 - t1) + t2 + 1;
    }
    else {
        tOut = t2 - t1;
    }
    return tOut;
}

int convert8x2to16(const uint8_t& a, const uint8_t& b) {
    return a << 8 | b;
}

conv16to8x2 convert16to8x2(const int& a) { // TODO AI - Проблема: Неправильный порядок операций. Должно быть r.a = (a & (0xff << 8)) >> 8; или r.a = (a >> 8) & 0xff;
    conv16to8x2 r;
    r.a = (a & 0xff << 8) >> 8;
    r.b = a & 0xff;
    return r;
}

// COMPs controls
void setCOMP_extVoltage_Up(const int& compN) {
    const exti_prN& tmp = reg_flags[compN];
    CLEAR_BIT(*tmp.ftsrX, tmp.line); // LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_21);
    SET_BIT(*tmp.rtsrX, tmp.line); // LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_21);
    WRITE_REG(*tmp.prX, tmp.line); // LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
    CLEAR_BIT(*tmp.emrX, tmp.line); // LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_21);
}

void setCOMP_extVoltage_Down(const int& compN) {
    const exti_prN& tmp = reg_flags[compN];
    CLEAR_BIT(*tmp.rtsrX, tmp.line); // LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_21);
    SET_BIT(*tmp.ftsrX, tmp.line); // LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_21);
    WRITE_REG(*tmp.prX, tmp.line); // LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
    CLEAR_BIT(*tmp.emrX, tmp.line); // LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_21);
}

void set_ALL_COMP_extVoltage_Up() {
    for (int i = 0; i < 7; ++i) {
        setCOMP_extVoltage_Up(i);
    }
}

void set_ALL_COMP_extVoltage_Down() {
    for (int i = 0; i < 7; ++i) {
        setCOMP_extVoltage_Down(i);
    }
}

void comp_ext_enable(const int& compN) {
    const exti_prN& tmp = reg_flags[compN];
    SET_BIT(*tmp.imrX, tmp.line); // LL_EXTI_EnableIT_X_XX(LL_EXTI_LINE_XX);
}

void comp_ext_disable(const int& compN) {
    const exti_prN& tmp = reg_flags[compN];
    WRITE_REG(*tmp.prX, tmp.line); // LL_EXTI_ClearFlag_X_XX(LL_EXTI_LINE_XX);
    CLEAR_BIT(*tmp.imrX, tmp.line); // LL_EXTI_DisableIT_X_XX(LL_EXTI_LINE_XX);
}

void comp_set_data(const int& compN, const int& data) {
    MODIFY_REG(*reg_flags[compN].pregSet, DAC_DHR12R1_DACC1DHR, data); // LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, data);
}

uint32_t comp_read_data(const int& compN) {
    return READ_BIT(*reg_flags[compN].pregRead, DAC_DOR1_DACC1DOR); // data = LL_DAC_RetrieveOutputData(DAC1, LL_DAC_CHANNEL_1);
}

COMP_TypeDef* comp_def(const int& i) { // TODO ?
    switch (i) {
    case 0:
        return COMP1;
        break;
    case 1:
        return COMP2;
        break;
    case 2:
        return COMP3;
        break;
    case 3:
        return COMP4;
        break;
    case 4:
        return COMP5;
        break;
    case 5:
        return COMP6;
        break;
    case 6:
        return COMP7;
        break;
    }
}

// COMPs IQR Handlers
void COMP1_2_3_IRQ() { // line 21, 22, 29
    const uint32_t t = TIM2->CNT;
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_21)) {
        comp_ext_disable(0);
        jump_func(0, t);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_22)) {
        comp_ext_disable(1);
        jump_func(1, t);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_29)) {
        comp_ext_disable(2);
        jump_func(2, t);
    }
}

void COMP4_5_6_IRQ(void) { // line 30, 31, 32
    const uint32_t t = TIM2->CNT;
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_30)) {
        comp_ext_disable(3);
        jump_func(3, t);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_31)) {
        comp_ext_disable(4);
        jump_func(4, t);
    }
    if (LL_EXTI_IsActiveFlag_32_63(LL_EXTI_LINE_32)) {
        comp_ext_disable(5);
        jump_func(5, t);
    }
}

void COMP7_IRQ(void) { // line 33
    const uint32_t t = TIM2->CNT;
    if (LL_EXTI_IsActiveFlag_32_63(LL_EXTI_LINE_33)) {
        comp_ext_disable(6);
        jump_func(6, t);
    }
}

void jump_off(const int& a, const uint32_t& t) {
    if (f[a] == 0) {
        f[a] = 1;
    }
    if (f[a] == 2) {
        f[a] = 3;
        tim[a][0] = t;
    }
    if (f[a] == 4) {
        f[a] = 5;
        tim[a][1] = t;
    }
}

void jump_on(const int& a, const uint32_t& t) {
    if (f[a] == 0) {
        f[a] = 1;
        tim[a][0] = t;
    }
    if (f[a] == 2) {
        f[a] = 3;
        tim[a][1] = t;
    }
}

void reg_init() {
    for (int i = 0; i < 7; ++i) {
        switch (i) {
        case 0:
            reg_flags[i].line = LL_EXTI_LINE_21;
            reg_flags[i].prX = &EXTI->PR1; // LL_EXTI_ClearFlag_X_XX(LL_EXTI_LINE_XX)
            reg_flags[i].imrX = &EXTI->IMR1; // LL_EXTI_ .. En ... Dis ...ableIT_X_XX(LL_EXTI_LINE_XX);
            reg_flags[i].emrX = &EXTI->EMR1; // LL_EXTI_ .. En ... Dis ...ableEvent_0_31(LL_EXTI_LINE_21);
            reg_flags[i].ftsrX = &EXTI->FTSR1; // LL_EXTI_ .. En ... Dis ...ableFallingTrig_0_31(LL_EXTI_LINE_21);
            reg_flags[i].rtsrX = &EXTI->RTSR1; // LL_EXTI_ .. En ... Dis ...ableRisingTrig_0_31(LL_EXTI_LINE_21);
            reg_flags[i].pregSet = __DAC_PTR_REG_OFFSET(DAC1->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            reg_flags[i].pregRead = __DAC_PTR_REG_OFFSET(DAC1->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 1:
            reg_flags[i].line = LL_EXTI_LINE_22;
            reg_flags[i].prX = &EXTI->PR1;
            reg_flags[i].imrX = &EXTI->IMR1;
            reg_flags[i].emrX = &EXTI->EMR1;
            reg_flags[i].ftsrX = &EXTI->FTSR1;
            reg_flags[i].rtsrX = &EXTI->RTSR1;
            reg_flags[i].pregSet = __DAC_PTR_REG_OFFSET(DAC1->DHR12R1, (LL_DAC_CHANNEL_2 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            reg_flags[i].pregRead = __DAC_PTR_REG_OFFSET(DAC1->DOR1, (LL_DAC_CHANNEL_2 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 2:
            reg_flags[i].line = LL_EXTI_LINE_29;
            reg_flags[i].prX = &EXTI->PR1;
            reg_flags[i].imrX = &EXTI->IMR1;
            reg_flags[i].emrX = &EXTI->EMR1;
            reg_flags[i].ftsrX = &EXTI->FTSR1;
            reg_flags[i].rtsrX = &EXTI->RTSR1;
            reg_flags[i].pregSet = __DAC_PTR_REG_OFFSET(DAC3->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            reg_flags[i].pregRead = __DAC_PTR_REG_OFFSET(DAC3->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 3:
            reg_flags[i].line = LL_EXTI_LINE_30;
            reg_flags[i].prX = &EXTI->PR1;
            reg_flags[i].imrX = &EXTI->IMR1;
            reg_flags[i].emrX = &EXTI->EMR1;
            reg_flags[i].ftsrX = &EXTI->FTSR1;
            reg_flags[i].rtsrX = &EXTI->RTSR1;
            reg_flags[i].pregSet = __DAC_PTR_REG_OFFSET(DAC3->DHR12R1, (LL_DAC_CHANNEL_2 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            reg_flags[i].pregRead = __DAC_PTR_REG_OFFSET(DAC3->DOR1, (LL_DAC_CHANNEL_2 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 4:
            reg_flags[i].line = LL_EXTI_LINE_31;
            reg_flags[i].prX = &EXTI->PR1;
            reg_flags[i].imrX = &EXTI->IMR1;
            reg_flags[i].emrX = &EXTI->EMR1;
            reg_flags[i].ftsrX = &EXTI->FTSR1;
            reg_flags[i].rtsrX = &EXTI->RTSR1;
            reg_flags[i].pregSet = __DAC_PTR_REG_OFFSET(DAC4->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            reg_flags[i].pregRead = __DAC_PTR_REG_OFFSET(DAC4->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 5:
            reg_flags[i].line = LL_EXTI_LINE_32;
            reg_flags[i].prX = &EXTI->PR2;
            reg_flags[i].imrX = &EXTI->IMR2;
            reg_flags[i].emrX = &EXTI->EMR2;
            reg_flags[i].ftsrX = &EXTI->FTSR2;
            reg_flags[i].rtsrX = &EXTI->RTSR2;
            reg_flags[i].pregSet = __DAC_PTR_REG_OFFSET(DAC4->DHR12R1, (LL_DAC_CHANNEL_2 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            reg_flags[i].pregRead = __DAC_PTR_REG_OFFSET(DAC4->DOR1, (LL_DAC_CHANNEL_2 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 6:
            reg_flags[i].line = LL_EXTI_LINE_33;
            reg_flags[i].prX = &EXTI->PR2;
            reg_flags[i].imrX = &EXTI->IMR2;
            reg_flags[i].emrX = &EXTI->EMR2;
            reg_flags[i].ftsrX = &EXTI->FTSR2;
            reg_flags[i].rtsrX = &EXTI->RTSR2;
            reg_flags[i].pregSet = __DAC_PTR_REG_OFFSET(DAC2->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            reg_flags[i].pregRead = __DAC_PTR_REG_OFFSET(DAC2->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;

        default:
            break;
        }
    }
}

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

/*
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

void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
    tx_settings[0] = a;
    tx_settings[1] = b;
    tx_settings[2] = c;
    tx_settings[3] = d;
    tx_settings[4] = e;
}

void USART2_Send_settings() {
    LL_USART_EnableDirectionTx(USART2); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    for (int i = 0; i < tx_settings_length; i++) {
        LL_USART_TransmitData9(USART2, tx_settings[i]);
        while (!LL_USART_IsActiveFlag_TXE(USART2)) {}
    }
    while (!LL_USART_IsActiveFlag_TC(USART2)) {}
    LL_USART_DisableDirectionTx(USART2);
    LL_USART_ClearFlag_TC(USART2);
}
*/
