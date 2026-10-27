/*
 * midi_keyboard.cpp
 *
 *  Created on: Mar 6, 2025
 *      Author: sche
 */

#include "piano_g4.hpp"
#include "stm32g4xx.h"

// static const uint32_t ADRESS_CHIP_NUMBER_older = 0x08003800;
static const uint32_t ADRESS_CHIP_NUMBER = 0x08004800; // здесь храним номер чипа

// main
void g4() {
    Read_uint32(ADRESS_CHIP_NUMBER, &chipAdress, 4);


    // FlashPageInfo_t info = GetPageAndBank(ADRESS_CHIP_NUMBER, 4);
    // Erase(info.start_page, info.bank); // очистить номер чипа
    // Flash_uint32(ADRESS_CHIP_NUMBER, &chipAdress, 4);


    static const uint8_t note = chipAdress * 7;
    tx_settings[0] = chipAdress;
    rx_settings[0] = chipAdress;

    reg_init();
    cap_init();

    GPIOB->BSRR = 0x80; // pB7 // tim interrupter // чтобы включился счёт таймера с H7

    // TIM init. TIM2 — первым: TIM5 и TIM3 отстают от него на пару тиков, и «TIM2->CNT − метка» в окнах
    // автоматов не бывает отрицательной (иначе — огромное беззнаковое и мгновенный таймаут)
    LL_TIM_EnableCounter(TIM2); // метки клавиш 0, 3, 4, 5 и «сейчас» для окон и пауз
    LL_TIM_EnableCounter(TIM5); // метки клавиш 1, 6
    LL_TIM_EnableCounter(TIM3); // метка клавиши 2 (16 бит)
    const uint32_t c2 = TIM2->CNT; // сначала TIM2: замер k3 не больше истинного — take() не уйдёт в «отрицательный» возраст
    const uint32_t c3 = TIM3->CNT;
    k3 = c2 - c3;
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
// TIM2, TIM3, TIM5 - presc = 0 - 7.35ns период

    if (chipAdress <= 13) { // ship ON
        windowReTRigger1 = 55000 * TICK_US; // = 55ms но не выше 61ms - ограничение канала связи
        windowReTRigger2 = 50000 * TICK_US; // = 50ms
    }
    reset_all(); // взвод: ON ждёт G↓, OFF — R↓ (подъём демпфера)

    USART2_Send(compN_, dot_, 0);

    if (chipAdress > 13) {
        while (1) {
            // off
            if (need_reset) {
                reset_all();
            }
            if (!muted) {
                for (uint8_t i = 0; i < 7; ++i) {
                    off_step(i, note + i);
                }
            }
            check();
            service();
        }
    }
    else {
        while (1) {
            // on
            if (need_reset) {
                reset_all();
            }
            if (!muted) {
                for (uint8_t i = 0; i < 7; ++i) {
                    on_step(i, note + i);
                }
            }
            check();
            service();
        }
    }
}

void check() {
    if (ev_r != ev_w && fl) {
        const nt& x = ev_buf[ev_r % ev_size];
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
        ++ev_r;
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
            reset_all();
            service_flag = 0;
            USART2_Send(compN_, 0, (int)subcommand::stop_calibration);
        }
    }
}

void resetData() { // из прерывания USART2: только глушим, автоматы и очередь сбросит основной цикл (reset_all)
    for (uint8_t i = 0; i < 7; ++i) {
        cap_off(i);
    }
    need_reset = 1;
}

// основной цикл: очередь пуста, все автоматы — в исходное состояние
void reset_all() {
    need_reset = 0;
    ev_r = ev_w;
    __disable_irq();
    if (!fl && !LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_1)) { // событие ждёт слота на шине — тоже устарело
        LL_TIM_DisableIT_CC3(TIM8);
        fl = 1;
    }
    __enable_irq();
    for (int i = 0; i < 7; ++i) {
        comp_idle(i);
    }
}

// исходное состояние автомата: ON — ждём спада на green, OFF — спада на R (подъём демпфера)
void comp_idle(const int i) {
    if (chipAdress > 13) {
        lifted[i] = 0;
        arm(i, comp[i][1], false, 0, true, true); // R уже внизу — опоздавший R↓, r_late
        return;
    }
    arm(i, comp[i][0], false, 0, false, false);
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
        cap_off(compN_);
        need_reset = 1; // новый порог вступит через comp_idle (с верным фронтом)
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
void calibration() { // следом communication() зовёт resetData(): автомат перевзведёт основной цикл
    cap_off(compN_);
    comp_set_data(compN_, 0);
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
    comp_set_data(compN_, comp[compN_][0]);
}

void all_calibration() {
    if (b_ == (uint8_t)subcommand::start_calibration) {
        for (int i = 0; i < 7; ++i) { // основной цикл уйдёт в service(), на выходе — reset_all()
            cap_off(i);
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
    muted = 1;
    for (int a = 0; a < 7; ++a) {
        cap_off(a);
    }
}

void unmute() { // автоматы стояли — взводим заново
    muted = 0;
    need_reset = 1;
}

// UART Send-Recive
void USART2_Send(const uint8_t& compN, const uint8_t& dot, const int& value) {
    // TC ждём сами. Из основного цикла (старт g4(), выход из калибровки) прерывание TC (USART2_IRQ — конец
    // передачи события) успевало сбросить TC раньше ожидания ниже — вечный цикл, основной цикл чипа вставал.
    LL_USART_DisableIT_TC(USART2);
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
    LL_USART_EnableIT_TC(USART2);
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

// p — в прежних тиках TIM2 (≈100 нс)
void pause(const uint& p) { // TIM2 не обнуляем: он же метка времени замеров
    const uint32_t s = TIM2->CNT;
    while (TIM2->CNT - s < p * TICK_OLD) {}
}

uint32_t sat(const uint32_t t) {
    return t > LEN_MAX ? LEN_MAX : t;
}

void ev_push(const uint8_t note, const uint32_t len) { // очередь полна — событие теряется (16 на 7 клавиш)
    if ((uint8_t)(ev_w - ev_r) < ev_size) {
        ev_buf[ev_w % ev_size] = { note, len };
        ++ev_w;
    }
}

// переход TIM2 через 0 учтён сам: беззнаковое вычитание идёт по модулю 2^32.
// Верно, только пока у TIM2 ARR = 0xFFFFFFFF (tim.c, CubeMX) — иначе интервал через 0 будет неверным!
uint32_t timeLength(const uint& t1, const uint& t2) {
    return t2 - t1;
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
void comp_set_data(const int& compN, const int& data) {
    MODIFY_REG(*dac_regs[compN].pregSet, DAC_DHR12R1_DACC1DHR, data); // LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, data);
}

uint32_t comp_read_data(const int& compN) {
    return READ_BIT(*dac_regs[compN].pregRead, DAC_DOR1_DACC1DOR); // data = LL_DAC_RetrieveOutputData(DAC1, LL_DAC_CHANNEL_1);
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

// Захват: выход компаратора → вход таймера (TISEL), метку пишет таймер.
// CubeMX (comp.c, tim.c) включает компараторам EXTI и прерывания, а TIM2/TIM5 даёт PSC 13 и 13599 — здесь переделываем.
void cap_init() {
    NVIC_DisableIRQ(COMP1_2_3_IRQn);
    NVIC_DisableIRQ(COMP4_5_6_IRQn);
    NVIC_DisableIRQ(COMP7_IRQn);
    LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_21 | LL_EXTI_LINE_22 | LL_EXTI_LINE_29 | LL_EXTI_LINE_30 | LL_EXTI_LINE_31);
    LL_EXTI_DisableIT_32_63(LL_EXTI_LINE_32 | LL_EXTI_LINE_33);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21 | LL_EXTI_LINE_22 | LL_EXTI_LINE_29 | LL_EXTI_LINE_30 | LL_EXTI_LINE_31);
    LL_EXTI_ClearFlag_32_63(LL_EXTI_LINE_32 | LL_EXTI_LINE_33);
    NVIC_ClearPendingIRQ(COMP1_2_3_IRQn);
    NVIC_ClearPendingIRQ(COMP4_5_6_IRQn);
    NVIC_ClearPendingIRQ(COMP7_IRQn);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
    LL_TIM_SetAutoReload(TIM3, 0xFFFF);
    LL_TIM_SetRemap(TIM2, LL_TIM_TIM2_TI1_RMP_COMP5 | LL_TIM_TIM2_TI2_RMP_COMP6 | LL_TIM_TIM2_TI3_RMP_COMP4 | LL_TIM_TIM2_TI4_RMP_COMP1);
    LL_TIM_SetRemap(TIM5, LL_TIM_TIM5_TI1_RMP_COMP7 | LL_TIM_TIM5_TI2_RMP_COMP2);
    LL_TIM_SetRemap(TIM3, LL_TIM_TIM3_TI1_RMP_COMP3);
    LL_TIM_SetPrescaler(TIM2, 0);
    LL_TIM_SetPrescaler(TIM3, 0);
    LL_TIM_SetPrescaler(TIM5, 0);
    LL_TIM_GenerateEvent_UPDATE(TIM2); // PSC вступает по событию обновления
    LL_TIM_GenerateEvent_UPDATE(TIM3);
    LL_TIM_GenerateEvent_UPDATE(TIM5);
    for (int i = 0; i < 7; ++i) { // делитель захвата 1, фильтр выключен — по сбросу
        LL_TIM_IC_SetActiveInput(cap[i].tim, cap[i].ch, LL_TIM_ACTIVEINPUT_DIRECTTI);
    }
}

// Захват канала i: on — CCxE, rising — фронт, иначе спад. CCER общий у каналов таймера, а пишут его
// и основной цикл, и USART2_IRQ (cap_off) — чтение-изменение-запись без прерываний.
void cap_set(const int i, const bool on, const bool rising) {
    const uint32_t e = cap[i].ch;
    const uint32_t pm = __get_PRIMASK();
    __disable_irq();
    MODIFY_REG(cap[i].tim->CCER, e | e << 1 | e << 3, (on ? e : 0) | (rising ? 0 : e << 1)); // CCxE, CCxP, CCxNP
    __set_PRIMASK(pm);
}

void cap_off(const int i) {
    const uint32_t pm = __get_PRIMASK();
    __disable_irq();
    CLEAR_BIT(cap[i].tim->CCER, cap[i].ch);
    __set_PRIMASK(pm);
}

// Событие канала i: 0 — нет; 1 — фронт, t — метка таймера; 2 — «опоздавшее» (взвод застал сигнал
// уже за порогом), t — момент проверки по TIM2, ошибка до двух SETTLE и прохода цикла.
uint8_t take(const int i, uint32_t& t) {
    if (late[i]) {
        late[i] = 0;
        t = t_late[i];
        return 2;
    }
    const cap_ch& c = cap[i];
    if (!(c.tim->SR & 1u << c.n)) {
        return 0;
    }
    t = (&c.tim->CCR1)[c.n - 1];    // чтение сбрасывает CCxIF
    c.tim->SR = ~(1u << (c.n + 8)); // CCxOF: до чтения успел ещё фронт — в CCR метка последнего
    if (c.tim == TIM3) {            // 16 бит: возраст метки считаем по TIM2 (k3 — сдвиг TIM3 относительно TIM2)
        const uint32_t now = TIM2->CNT;
        t = now - (uint16_t)(now - k3 - t);
    }
    return 1;
}

// Перевзвод канала i на порог dac (rising — фронт, иначе спад), затем состояние wait. Без ожидания на месте:
// дальше шаги делает arm_busy(). preset — сначала ЦАП в край (выход компаратора «до фронта», гистерезис
// в известном состоянии). Пока ЦАП устанавливается, захват выключен: скачок порога — не фронт.
// check — по окончании проверить уровень: сигнал уже за порогом — событие «опоздавшее» (take() == 2).
void arm(const int i, const int dac, const bool rising, const int8_t wait, const bool preset, const bool check) {
    cap_set(i, false, rising);
    f[i] = wait;
    late[i] = 0;
    arm_dac[i] = dac;
    arm_rise[i] = rising;
    arm_check[i] = check;
    arm_probe[i] = 0;
    comp_set_data(i, preset ? (rising ? 4095 : 0) : dac);
    arm_t[i] = TIM2->CNT;
    arm_step[i] = preset ? 1 : 2;
}

// Проба: сигнал уже за порогом dac в сторону фронта rising? Захват остаётся выключенным,
// ответ — probe_res[i] в состоянии wait; дальше — arm() или comp_idle().
void probe(const int i, const int dac, const bool rising, const int8_t wait) {
    arm(i, dac, rising, wait, true, false);
    arm_probe[i] = 1;
}

// Шаг взвода/пробы; true — ЦАП ещё устанавливается, автомат клавиши ждёт
bool arm_busy(const int i) {
    if (!arm_step[i]) {
        return false;
    }
    const uint32_t now = TIM2->CNT;
    if (now - arm_t[i] < SETTLE) {
        return true;
    }
    if (arm_step[i] == 1) {
        comp_set_data(i, arm_dac[i]);
        arm_t[i] = now;
        arm_step[i] = 2;
        return true;
    }
    arm_step[i] = 0;
    if (arm_probe[i]) {
        probe_res[i] = past(i, arm_rise[i]);
        return false;
    }
    const cap_ch& c = cap[i];
    c.tim->SR = ~(1u << c.n | 1u << (c.n + 8)); // старые CCxIF/CCxOF
    cap_set(i, true, arm_rise[i]);
    // уровень — раньше флага: фронт между ними попадёт во флаг, а не в «опоздавшие»
    if (arm_check[i] && past(i, arm_rise[i]) && !(c.tim->SR & 1u << c.n)) {
        late[i] = 1;
        t_late[i] = TIM2->CNT;
    }
    return false;
}

// Выход компаратора уже за порогом в сторону фронта (rising — выше, иначе ниже)?
bool past(const int i, const bool rising) {
    return (LL_COMP_ReadOutputLevel(comp_def(i)) == LL_COMP_OUTPUT_LEVEL_HIGH) == rising;
}

// Автомат ON-датчика. Сигнал падает при ударе: G = comp[i][0] > R = comp[i][1], оба порога — спадом.
// G↓ (t0) → R↓ (t1): note-on со скоростью t1−t0. Окна — от аппаратных меток, а не от того, когда их увидел цикл.
void on_step(const uint8_t i, const uint8_t n) {
    if (arm_busy(i)) return;
    uint32_t t;
    switch (f[i]) {
    case 0: // ждём G↓
        if (take(i, t)) {
            tim[i][0] = t;
            arm(i, comp[i][1], false, 2, false, true); // R уже пройден за время взвода — note-on «опоздавший»
        }
        break;
    case 2: // ждём R↓; за windowReTRigger1 от G↓ не дошла — снова G
        if (take(i, t)) {
            tim[i][1] = t;
            ev_push(n, sat(timeLength(tim[i][0], t)));
            cap_off(i);
            f[i] = 4;
        }
        else if (TIM2->CNT - tim[i][0] > windowReTRigger1) {
            arm(i, comp[i][0], false, 0, false, false);
        }
        break;
    case 4: // note-on отправлен; windowReTRigger2 от R↓ — снова G
        if (TIM2->CNT - tim[i][1] > windowReTRigger2) {
            arm(i, comp[i][0], false, 0, false, false);
        }
        break;
    }
}

// Автомат OFF-датчика. Сигнал растёт к покою (демпфер на струнах): G = comp[i][0] < R = comp[i][1].
// Подъём демпфера: R↓ → G↓ (скорость — интервал R↓→G↓). Опускание: G↑ (t0) → R↑ (t1) — note-off со скоростью t1−t0.
// Ниже R, но за OFF_LIFT_WAIT не дошла до G — подъём без скорости; вернулась выше R раньше — помеха, молчим
// (так же гасится дребезг посадки демпфера). Пока клавиша между R и G, раз в OFF_POLL — проба второго порога
// (состояния 3 и 7). Проба уводит ЦАП с ожидаемого порога: фронт, пришедший за это время, будет «опоздавшим».
void off_step(const uint8_t i, const uint8_t n) {
    if (arm_busy(i)) return;
    const uint32_t now = TIM2->CNT;
    uint32_t t;
    uint8_t e;
    switch (f[i]) {
    case 0: // ждём R↓: демпфер пошёл со струн
        if (!(e = take(i, t))) break;
        t_rdown[i] = t;
        r_late[i] = e == 2; // R↓ застал взвод (клавиша нажата во время сброса): момент неизвестен
        tim_poll[i] = t;
        arm(i, comp[i][0], false, 2, true, true);
        break;
    case 2: // ждём G↓: клавиша внизу
        if (take(i, t)) {
            if (!lifted[i]) {
                lifted[i] = 1;
                const uint32_t len = sat(timeLength(t_rdown[i], t));
                if (SEND_LIFT) ev_push(n, LIFT_FLAG | (r_late[i] ? 0 : (len ? len : 1)));
            }
            arm(i, comp[i][0], true, 4, true, true);
        }
        else if (now - tim_poll[i] >= OFF_POLL) {
            tim_poll[i] = now;
            probe(i, comp[i][1], true, 3);
        }
        break;
    case 3: // проба из 2: не вернулась ли выше R?
        if (probe_res[i]) {
            if (lifted[i]) ev_push(n, LEN_NONE); // неглубокое нажатие отпущено: демпфер лёг, скорость не замерить
            comp_idle(i);
            break;
        }
        if (!lifted[i] && now - t_rdown[i] > OFF_LIFT_WAIT) {
            lifted[i] = 1;
            if (SEND_LIFT) ev_push(n, LIFT_FLAG);
        }
        arm(i, comp[i][0], false, 2, true, true);
        break;
    case 4: // ждём G↑: клавиша пошла вверх
        if (!take(i, t)) break;
        tim[i][0] = t;
        tim_poll[i] = t;
        arm(i, comp[i][1], true, 6, true, true);
        break;
    case 6: // ждём R↑: демпфер лёг
        if (take(i, t)) {
            ev_push(n, sat(timeLength(tim[i][0], t)));
            comp_idle(i);
        }
        else if (now - tim_poll[i] >= OFF_POLL) {
            tim_poll[i] = now;
            probe(i, comp[i][0], false, 7);
        }
        break;
    case 7: // проба из 6: не вернулась ли ниже G (дребезг у дна, повтор без посадки демпфера)?
        if (probe_res[i]) {
            arm(i, comp[i][0], true, 4, true, true); // снова внизу: t0 даст следующий G↑
        }
        else {
            arm(i, comp[i][1], true, 6, true, true);
        }
        break;
    }
}

void reg_init() {
    for (int i = 0; i < 7; ++i) {
        switch (i) {
        case 0:
            dac_regs[i].pregSet = __DAC_PTR_REG_OFFSET(DAC1->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            dac_regs[i].pregRead = __DAC_PTR_REG_OFFSET(DAC1->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 1:
            dac_regs[i].pregSet = __DAC_PTR_REG_OFFSET(DAC1->DHR12R1, (LL_DAC_CHANNEL_2 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            dac_regs[i].pregRead = __DAC_PTR_REG_OFFSET(DAC1->DOR1, (LL_DAC_CHANNEL_2 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 2:
            dac_regs[i].pregSet = __DAC_PTR_REG_OFFSET(DAC3->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            dac_regs[i].pregRead = __DAC_PTR_REG_OFFSET(DAC3->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 3:
            dac_regs[i].pregSet = __DAC_PTR_REG_OFFSET(DAC3->DHR12R1, (LL_DAC_CHANNEL_2 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            dac_regs[i].pregRead = __DAC_PTR_REG_OFFSET(DAC3->DOR1, (LL_DAC_CHANNEL_2 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 4:
            dac_regs[i].pregSet = __DAC_PTR_REG_OFFSET(DAC4->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            dac_regs[i].pregRead = __DAC_PTR_REG_OFFSET(DAC4->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 5:
            dac_regs[i].pregSet = __DAC_PTR_REG_OFFSET(DAC4->DHR12R1, (LL_DAC_CHANNEL_2 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            dac_regs[i].pregRead = __DAC_PTR_REG_OFFSET(DAC4->DOR1, (LL_DAC_CHANNEL_2 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
            break;
        case 6:
            dac_regs[i].pregSet = __DAC_PTR_REG_OFFSET(DAC2->DHR12R1, (LL_DAC_CHANNEL_1 >> DAC_REG_DHR12RX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DHR_REGOFFSET_MASK_POSBIT0);
            dac_regs[i].pregRead = __DAC_PTR_REG_OFFSET(DAC2->DOR1, (LL_DAC_CHANNEL_1 >> DAC_REG_DORX_REGOFFSET_BITOFFSET_POS) & DAC_REG_DORX_REGOFFSET_MASK_POSBIT0);
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
