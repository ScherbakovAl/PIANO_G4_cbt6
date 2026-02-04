/*
 * midi_keyboard.cpp
 *
 *  Created on: Mar 6, 2025
 *      Author: sche
 */

#include "piano_g4.hpp"

#include "deque"
std::deque<nt> dequeOn;

static const uint32_t ADRESS_CHIP_NUMBER = 0x08003800; // здесь храним номер чипа

// main
void g4() {
    Read_uint32(ADRESS_CHIP_NUMBER, &chipAdress, 4);
    static const uint8_t note = chipAdress * 7;
    tx_settings[0] = chipAdress;
    rx_settings[0] = chipAdress;

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
            comp[i][0] = def_off[0];
            comp[i][1] = def_off[1];
        }
        else {
            comp[i][0] = def_on[0];
            comp[i][1] = def_on[1];

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
        windowReTRigger1 = 10 * 60; // = 40ms // TIM5 (p100us) // TODO провериь бОльшие значения
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
        nt x = dequeOn.front();
        abcd a{ conv32to8(x) };
        tx_data[0] = x.n; // TODO add "& 0xFF" ?
        tx_data[1] = a.a;
        tx_data[2] = a.b;
        tx_data[3] = a.c;
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
            USART2_Send(compN_, 0, subcommand::stop_calibration);
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

    if (rx_settings[0] == command::sync_timer) {
        TIM8->CNT = 0;  // PWM - tim clk
        USART2_Send(compN_, dot_, 0);
        resetData();
    }

    if (rx_settings[0] == command::cal) {
        calibration();
        resetData();
    }

    if (rx_settings[0] == command::read_comp_value) {
        USART2_Send(compN_, dot_, comp[compN_][dot_]);
    }

    if (rx_settings[0] == command::set_comp_value) {
        comp[compN_][dot_] = convert8x2to16(a_, b_);
        USART2_Send(compN_, dot_, comp[compN_][dot_]);
        comp_refresh(compN_);
        f[compN_] = 0;
    }

    if (rx_settings[0] == command::all_calib) {
        all_calibration();
    }

    if (rx_settings[0] == command::reset_to_bootloader) {
        LL_mDelay(20);
        NVIC_SystemReset();
    }

    if (rx_settings[0] == command::echo) {
        USART2_Send(0, 0, state::piano);
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
    if (b_ == subcommand::start_calibration) {
        for (int i = 0; i < 7; ++i) {
            f[i] = 0;
            comp_ext_disable(i);
        }
        service_commands = subcommand::working;
        service_flag = 1;
        USART2_Send(compN_, 0, subcommand::start_calibration);
    }
    else if (b_ == subcommand::read_calibration) {
        pause(4);
        USART2_Send(compN_, 0, subcommand::read_calibration);
        for (int i = 0; i < 7; ++i) {
            pause(6);
            USART2_Send(i, 0, comp_calibration[i]);
        }
        flag_Service_command = 1;
    }
    else if (b_ == subcommand::stop_calibration) {
        service_commands = subcommand::exit;
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

abcd conv32to8(const nt& x) {
    abcd a;
    uint32_t u = x.timelegth;
    a.a = (u & 0xff << 16) >> 16;
    a.b = (u & 0xff << 8) >> 8;
    a.c = u & 0xff;
    return a;
}

// COMPs controls
void setCOMP_extVoltage_Up(const int& compN) {
    switch (compN) {
    case 0:
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_21);
        break;
    case 1:
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_22);
        break;
    case 2:
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_29);
        break;
    case 3:
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_30);
        break;
    case 4:
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_31);
        break;
    case 5:
        LL_EXTI_DisableFallingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_EnableRisingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_32);
        break;
    case 6:
        LL_EXTI_DisableFallingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_EnableRisingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_33);
        break;
    }
}

void setCOMP_extVoltage_Down(const int& compN) {
    switch (compN) {
    case 0:
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_21);
        break;
    case 1:
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_22);
        break;
    case 2:
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_29);
        break;
    case 3:
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_30);
        break;
    case 4:
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_31);
        break;
    case 5:
        LL_EXTI_DisableRisingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_EnableFallingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_32);
        break;
    case 6:
        LL_EXTI_DisableRisingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_EnableFallingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_33);
        break;
    }
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
    switch (compN) {
    case 0:
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_21);
        break;
    case 1:
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_22);
        break;
    case 2:
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29);
        break;
    case 3:
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_30);
        break;
    case 4:
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_31);
        break;
    case 5:
        LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_32);
        break;
    case 6:
        LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_33);
        break;
    }
}

void comp_ext_disable(const int& compN) {
    switch (compN) {
    case 0:
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_21);
        break;
    case 1:
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_22);
        break;
    case 2:
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_29);
        break;
    case 3:
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_30);
        break;
    case 4:
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_31);
        break;
    case 5:
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_32);
        break;
    case 6:
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_33);
        break;
    }
}

void comp_set_data(const int& compN, const int& data) {
    switch (compN) {
    case 0:
        LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, data);
        break;
    case 1:
        LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_2, data);
        break;
    case 2:
        LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_1, data);
        break;
    case 3:
        LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, data);
        break;
    case 4:
        LL_DAC_ConvertData12RightAligned(DAC4, LL_DAC_CHANNEL_1, data);
        break;
    case 5:
        LL_DAC_ConvertData12RightAligned(DAC4, LL_DAC_CHANNEL_2, data);
        break;
    case 6:
        LL_DAC_ConvertData12RightAligned(DAC2, LL_DAC_CHANNEL_1, data);
        break;
    }
}

COMP_TypeDef* comp_def(const int& i) {
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
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_21)) {
        comp_ext_disable(0);
        iffing(0);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_22)) {
        comp_ext_disable(1);
        iffing(1);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_29)) {
        comp_ext_disable(2);
        iffing(2);
    }
}

void COMP4_5_6_IRQ(void) { // line 30, 31, 32
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_30)) {
        comp_ext_disable(3);
        iffing(3);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_31)) {
        comp_ext_disable(4);
        iffing(4);
    }
    if (LL_EXTI_IsActiveFlag_32_63(LL_EXTI_LINE_32)) {
        comp_ext_disable(5);
        iffing(5);
    }
}

void COMP7_IRQ(void) { // line 33
    if (LL_EXTI_IsActiveFlag_32_63(LL_EXTI_LINE_33)) {
        comp_ext_disable(6);
        iffing(6);
    }
}

void iffing(const int& a) {
    if (chipAdress > 13) { // off
        if (f[a] == 0) {
            f[a] = 1;
        }
        if (f[a] == 2) {
            f[a] = 3;
            tim[a][0] = TIM2->CNT;
        }
        if (f[a] == 4) {
            f[a] = 5;
            tim[a][1] = TIM2->CNT;
        }
    }
    else { // on
        if (f[a] == 0) {
            f[a] = 1;
            tim[a][0] = TIM2->CNT;
        }
        if (f[a] == 2) {
            f[a] = 3;
            tim[a][1] = TIM2->CNT;
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
