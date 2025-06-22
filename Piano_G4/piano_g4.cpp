/*
 * midi_keyboard.cpp
 *
 *  Created on: Mar 6, 2025
 *      Author: sche
 */

#include "piano_g4.hpp"

#include "deque"
std::deque<notes> dequeOn;

// main
void g4() {
    { // init
        GPIOA->BSRR = 0x100; // pA8 // tim interrupter // чтобы включился счёт таймера с H7
        // TIM init
        LL_TIM_EnableCounter(TIM2);               // основной счётчик (100nS)
        LL_TIM_EnableCounter(TIM5);               // доп счётчик (100uS)
        LL_TIM_SetAutoReload(TIM8, allChipCount); // TIM8 для связи
        LL_TIM_OC_SetCompareCH3(TIM8, chipAdress);
        //	LL_TIM_EnableIT_UPDATE(TIM8); // это для теста
        LL_TIM_EnableCounter(TIM8); // TIM8 для связи
        //---------------------------------

        // UART init
        LL_USART_DisableDirectionTx(USART1); // один или несколько MCU должны быть подтянуты к pull_UP ! // надо ли? проверить..
        LL_USART_Disable(USART1);            // // надо ли? проверить..
        LL_USART_ConfigNodeAddress(USART1, LL_USART_ADDRESS_DETECT_7B, chipAdress);
        LL_USART_Enable(USART1);
        LL_USART_DisableDirectionTx(USART1);   // один или несколько MCU должны быть подтянуты к pull_UP !
        LL_USART_ClearFlag_TC(USART1);         // чтобы при включении не срабатывало прерывание
        LL_USART_EnableIT_RXNE_RXFNE(USART1);  // включение прерывания по приему данных
        LL_USART_EnableMuteMode(USART1);       // включение MuteMode
        LL_USART_RequestEnterMuteMode(USART1); // ставит USART в MuteMode и сбрасывает флаг RWU.
        LL_USART_EnableDMAReq_TX(USART1);      // подсоединяет DMA к UART
        LL_USART_EnableIT_TC(USART1);          // прерывание окончания передачи
        //---------------------------------

        // DMA TX
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
        LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t) & (USART1->TDR));
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)tx_data);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, tx_dataLength);
        //---------------------------------

        // DAC COMP
        LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, comp[0][0]);
        LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_2, comp[1][0]);
        LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_1, comp[2][0]);
        LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, comp[3][0]);
        LL_DAC_ConvertData12RightAligned(DAC4, LL_DAC_CHANNEL_1, comp[4][0]);
        LL_DAC_ConvertData12RightAligned(DAC4, LL_DAC_CHANNEL_2, comp[5][0]);
        LL_DAC_ConvertData12RightAligned(DAC2, LL_DAC_CHANNEL_1, comp[6][0]);

        LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
        LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);
        LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_1);
        LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_2);
        LL_DAC_Enable(DAC4, LL_DAC_CHANNEL_1);
        LL_DAC_Enable(DAC4, LL_DAC_CHANNEL_2);
        LL_DAC_Enable(DAC2, LL_DAC_CHANNEL_1);

        LL_COMP_Enable(COMP1);
        LL_COMP_Enable(COMP2);
        LL_COMP_Enable(COMP3);
        LL_COMP_Enable(COMP4);
        LL_COMP_Enable(COMP5);
        LL_COMP_Enable(COMP6);
        LL_COMP_Enable(COMP7);
        //---------------------------------

        // SPI
        //	LL_SPI_Enable(SPI1); // pA5-sck pA7-mosi
        //---------------------------------
    }


	// TODO
	// TODO
	// TODO
	// TODO
	// TODO
	// TODO

	// TODO
	// TODO	GPIO ANALOG MODE !!!!!!!!!!!! test and using !!!
	// TODO

	// TODO
	// TODO
	// TODO
	// TODO
	// TODO
	// TODO

    while (1) {
        for (int i = 0; i < 7; ++i) {

            if (f[i] != 0) {
                if (f[i] == 1) {
                    tim_off[i][0] = TIM5->CNT;
                    comp_set_data(i, comp[i][1]);
                    f[i] = 2;
                    comp_ext_enable(i);
                }
                if (f[i] == 2 && TIM5->CNT - tim_off[i][0] > 300) {
                    comp_ext_disable(i);
                    comp_set_data(i, comp[i][0]);
                    f[i] = 0;
                    comp_ext_enable(i);
                }
                if (f[i] == 3) {
                    tim_off[i][1] = TIM5->CNT;
                    f[i] = 4;
                    notes n;
                    n.set(i, tim[i][0], tim[i][1]);
                    dequeOn.push_back(n);
                }
                if (f[i] == 4 && TIM5->CNT - tim_off[i][1] > 500) {
                    comp_set_data(i, comp[i][0]);
                    f[i] = 0;
                    comp_ext_enable(i);
                }
            }
        }
        if (!dequeOn.empty() && fl) {
            auto x = dequeOn.front();
            tx_data[0] = x.n;
            tx_data[1] = x.a;
            tx_data[2] = x.b;
            LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
            LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, tx_dataLength);
            LL_USART_ClearFlag_TC(USART1);
            LL_TIM_ClearFlag_CC3(TIM8);
            LL_TIM_EnableIT_CC3(TIM8);
            dequeOn.pop_front();
            fl = 0;
        }
    }
}

void communication() {
    USART1_Receive();

    if (rx_settings[0] == command::sync_timer) {
        TIM8->CNT = 0;
        USART1_Send(compN_, dot_, 0);
    }

    if (rx_settings[0] == command::cal) {
        calibration();
    }

    if (rx_settings[0] == command::read_comp_value) {
        USART1_Send(compN_, dot_, comp[compN_][dot_]);
    }

    if (rx_settings[0] == command::set_comp_value) {
        comp[compN_][dot_] = convert8x2to16(a_, b_);
        USART1_Send(compN_, dot_, comp[compN_][dot_]);
    }
    // if (rx_settings[0] == command::set_div_value) { // TODO
    //     получить значени div и применить его
    //     divisible = ...;
    //     сконвертировать и отправить обратно это значение для проверки
    //     USART1_Send(a, b, cd);
    // }
}

// UART Send-Recive
void USART1_Send(uint8_t compN, uint8_t dot, int value) {
    LL_USART_EnableDirectionTx(USART1); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    tx_settings[1] = { compN };
    tx_settings[2] = { dot };
    conv16to8x2 c;
    c = convert16to8x2(value);
    tx_settings[3] = c.a;
    tx_settings[4] = c.b;
    while (!LL_USART_IsActiveFlag_TXE(USART1)) {}
    for (int i = 0; i < tx_settings_length; i++) {
        LL_USART_TransmitData9(USART1, tx_settings[i]);
        while (!LL_USART_IsActiveFlag_TXE(USART1)) {}
    }
    while (!LL_USART_IsActiveFlag_TC(USART1)) {}
    LL_USART_DisableDirectionTx(USART1);
    LL_USART_ClearFlag_TC(USART1);
}

void USART1_Receive() {
    while (!LL_USART_IsActiveFlag_RXNE(USART1)) {}
    rx_settings[0] = LL_USART_ReceiveData9(USART1);
    for (int i = 0; i < rx_settings_length; i++) {
        while (!LL_USART_IsActiveFlag_RXNE(USART1)) {}
        rx_settings[i] = LL_USART_ReceiveData9(USART1);
    }
    compN_ = rx_settings[1];
    dot_ = rx_settings[2];
    a_ = rx_settings[3];
    b_ = rx_settings[4];
    LL_USART_RequestRxDataFlush(USART1);
    LL_USART_RequestEnterMuteMode(USART1);
}
//---------------------------------

void calibration() {
    comp_ext_disable(compN_);
    comp_set_data(compN_, 0);
    f[compN_] = 0;
    for (int a = 0; a < 4095; ++a) {
        comp_set_data(compN_, a);
        if (LL_COMP_ReadOutputLevel(comp_def(compN_)) == LL_COMP_OUTPUT_LEVEL_LOW) {
            USART1_Send(compN_, dot_, a);
            break;
        }
        if (a > 4090) {
            USART1_Send(compN_, dot_, 0);
            break;
        }
    }
    f[compN_] = 0;
    comp_ext_enable(compN_);
}
//---------------------------------

// TIM IQR Handler
void TIM8_UP_IRQ() { // для теста (начало счёта таймера)  // for test
    LL_TIM_ClearFlag_UPDATE(TIM8);
}

void TIM8_CC_IRQ() { // это прерывание срабатывает от TIM8 по номеру контроллера
    GPIOA->BSRR = 0x1000000; // pA8 // tim interrupter
    LL_TIM_ClearFlag_CC3(TIM8);
    LL_USART_EnableDirectionTx(USART1); // отправлять только при ВЫКЛюченных остальных слэйв-трансляторах.. иначе замыкание
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
}
//---------------------------------

// UART IQR Handler
void USART1_IRQ() {
    if (LL_USART_IsActiveFlag_TC(USART1)) {  // здесь прерывание, если завершена передача данных
        LL_USART_DisableDirectionTx(USART1); // один или несколько должны быть подтянуты к pull_UP !
        LL_USART_ClearFlag_TC(USART1);       // идёт вслед за DisableDirectionTx, иначе прерывается ещё раз
        fl = 1;
        LL_TIM_DisableIT_CC3(TIM8);
        GPIOA->BSRR = 0x100; // pA8 // tim interrupter
    }
    if (LL_USART_IsActiveFlag_RXNE_RXFNE(USART1)) { // здесь прерывание, если адрес UART совпал
        GPIOA->BSRR = 0x1000000; // pA8 // tim interrupter
        communication();
        LL_USART_RequestRxDataFlush(USART1);   // сбрасывает RXNE флаг (или uint8_t data = LL_USART_ReceiveData8(USART1); сбрасывает)
        LL_USART_RequestEnterMuteMode(USART1); // ставит USART в MuteMode и сбрасывает флаг RWU.
        GPIOA->BSRR = 0x100; // pA8 // tim interrupter
    }
}
//---------------------------------

void notes::set(uint8_t nn, uint32_t t1, uint32_t t2) {
    uint32_t tOut;
    if (t1 > t2) {
        tOut = 4'294'967'295 - t1 + t2 + 1;
    }
    else {
        tOut = t2 - t1;
    }
    cuint midi_speed = divisible / tOut;
    cuint midi_hi = midi_speed / maxMidi;
    cuint midi_lo = midi_speed - midi_hi * maxMidi;

    n = (uint8_t)note + nn;
    a = (uint8_t)midi_hi;
    b = (uint8_t)midi_lo;
}
//---------------------------------

// (100ns)
void pause(int p) {
    TIM2->CNT = 0;
    while (TIM2->CNT < p) {
    }
}

int convert8x2to16(uint8_t a, uint8_t b) {
    return a << 8 | b;
}

conv16to8x2 convert16to8x2(int a) {
    conv16to8x2 r;
    r.a = (a & 0xff << 8) >> 8;
    r.b = a & 0xff;
    return r;
}

// void conv32to8() {} // реализовать
// void conv8to32() {} // реализовать
//---------------------------------

void setCOMP_extVoltage_Up(int compN) {
    switch (compN) {
    case 0:
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_21);
        break;
    case 1:
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_22);
        break;
    case 2:
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_29);
        break;
    case 3:
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_30);
        break;
    case 4:
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_31);
        break;
    case 5:
        LL_EXTI_EnableRisingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableFallingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_32);
        break;
    case 6:
        LL_EXTI_EnableRisingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableFallingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_33);
        break;
    }
}

void setCOMP_extVoltage_Down(int compN) {
    switch (compN) {
    case 0:
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_21);
        break;
    case 1:
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_22);
        break;
    case 2:
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_29);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_29);
        break;
    case 3:
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_30);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_30);
        break;
    case 4:
        LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_31);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableEvent_0_31(LL_EXTI_LINE_31);
        break;
    case 5:
        LL_EXTI_EnableFallingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableRisingTrig_32_63(LL_EXTI_LINE_32);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_32);
        break;
    case 6:
        LL_EXTI_EnableFallingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableRisingTrig_32_63(LL_EXTI_LINE_33);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableEvent_32_63(LL_EXTI_LINE_33);
        break;
    }
}

void comp_ext_enable(int compN) {
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

void comp_ext_disable(int compN) {
    switch (compN) {
    case 0:
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_21);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
        break;
    case 1:
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_22);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_22);
        break;
    case 2:
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_29);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);
        break;
    case 3:
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_30);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_30);
        break;
    case 4:
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_31);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_31);
        break;
    case 5:
        LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_32);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32);
        break;
    case 6:
        LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_33);
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_33);
        break;
    }
}

void comp_set_data(int compN, int data) {
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

// COMP IQR Handler
void COMP1_2_3_IRQ() { // line 21, 22, 29
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_21)) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_21);
        iffing(0);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_22)) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_22);
        iffing(1);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_29)) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_29);
        iffing(2);
    }
}

void COMP4_5_6_IRQ(void) { // line 30, 31, 32
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_30)) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_30);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_30);
        iffing(3);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_31)) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_31);
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_31);
        iffing(4);
    }
    if (LL_EXTI_IsActiveFlag_32_63(LL_EXTI_LINE_32)) {
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32);
        LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_32);
        iffing(5);
    }
}

void COMP7_IRQ(void) { // line 33
    if (LL_EXTI_IsActiveFlag_32_63(LL_EXTI_LINE_33)) {
        LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_33);
        LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_33);
        iffing(6);
    }
}

void iffing(int a) {
    if (f[a] == 0) {
        f[a] = 1;
        tim[a][0] = TIM2->CNT;
    }
    if (f[a] == 2) {
        f[a] = 3;
        tim[a][1] = TIM2->CNT;
    }
}
//---------------------------------

// SPI Send-Recive
void SPI1_SendData(uint16_t data) {
    while (!LL_SPI_IsActiveFlag_TXE(SPI1)) {};
    LL_SPI_TransmitData16(SPI1, data);
    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {};
}
//---------------------------------

void debugg() { // DEBUG
    if (TIM2->CNT > 1000000) {
        TIM2->CNT = 0;
        LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_6);
    }
}

COMP_TypeDef* comp_def(const int i) {
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
