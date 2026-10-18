
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

// <<<<<<< Updated upstream
    volatile uint32_t chipAdress = 0; // 1-13-on, 14-26-off
// =======
    // #define CHAddr 3
    
    // #if CHAddr > 13
    // #define CHipForNoteOFF
    // #endif
    
    // const int chipAdress = 3; // 1-13-on, 14-26-off
// >>>>>>> Stashed changes
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

    uint32_t tim[7][2] = { }; // метки: ON — G↓ и R↓, OFF — G↑ и R↑

    // OFF: метка спада на R (подъём демпфера), подъём уже отправлен, последний опрос уровня
    uint32_t t_rdown[7] = { };
    uint8_t lifted[7] = { };
    uint8_t r_late[7] = { }; // R↓ поймана перевзводом (клавиша нажата во время сброса): момент неизвестен
    uint32_t tim_poll[7] = { };

    // Метки пишет таймер: выход компаратора заведён на вход захвата (TISEL), без EXTI и прерываний.
    // Все таймеры — PSC 0, тик 1/136 МГц ≈ 7.35 нс. Интервал — внутри одной клавиши, т.е. одного таймера.
    // TIM3 16-битный: метку клавиши 2 достраивает до 32 бит take() по TIM2 — её надо забрать за 2^16 тиков (482 мкс).
    struct cap_ch {
        TIM_TypeDef* tim;
        uint32_t ch; // LL_TIM_CHANNEL_CHx — он же бит CCxE
        uint32_t n;  // 1..4: CCxIF = 1 << n, CCxOF = 1 << (n + 8), CCRx = (&CCR1)[n - 1]
    };
    const cap_ch cap[7] = {
        { TIM2, LL_TIM_CHANNEL_CH4, 4 }, // COMP1
        { TIM5, LL_TIM_CHANNEL_CH2, 2 }, // COMP2
        { TIM3, LL_TIM_CHANNEL_CH1, 1 }, // COMP3
        { TIM2, LL_TIM_CHANNEL_CH3, 3 }, // COMP4
        { TIM2, LL_TIM_CHANNEL_CH1, 1 }, // COMP5
        { TIM2, LL_TIM_CHANNEL_CH2, 2 }, // COMP6
        { TIM5, LL_TIM_CHANNEL_CH1, 1 }, // COMP7
    };
    uint16_t k3 = 0; // TIM2 − TIM3 (младшие 16 бит), замер при старте

    // Взвод канала без ожидания на месте: arm() пишет ЦАП и уходит, шаги делает arm_busy() из основного цикла.
    // arm_step: 0 — свободен; 1 — ЦАП в краю, ждём SETTLE; 2 — ЦАП на пороге, ждём SETTLE.
    uint8_t arm_step[7] = { };
    uint32_t arm_t[7] = { };     // момент последней записи в ЦАП (TIM2)
    int arm_dac[7] = { };
    uint8_t arm_rise[7] = { };
    uint8_t arm_check[7] = { };  // по окончании: сигнал уже за порогом — событие «опоздало»
    uint8_t arm_probe[7] = { };  // не взвод, а проба уровня: захват не включается, ответ в probe_res
    uint8_t probe_res[7] = { };
    uint8_t late[7] = { };       // опоздавшее событие ждёт take(); метка — момент проверки (t_late)
    uint32_t t_late[7] = { };
    int comp[7][3] = { };
    int comp_calibration[7] = { };
    int cc_fl[7] = { }; // флаг. Проверять состояние данного компаратора при калибровке или нет

    struct default_comp {
        int green_on = 2200;
        int red_on = 2000;
        int green_off = 1800;
        int red_off = 3400;
        int grey_off = 1600;
    };
    struct default_comp def_comp;

    int8_t f[7] = { 0 }; // состояние автомата клавиши; только основной цикл
    volatile int fl = 1;
    volatile int flag_Service_command = 1;
    volatile int need_reset = 0; // сброс автоматов просят из прерывания USART2, делает основной цикл
    volatile int muted = 0;      // mute с H7: автоматы стоят, захват выключен

    enum class command {
        sync_timer = 1,
        cal,
        read_comp_value,
        set_comp_value,
        read_comp_setting,
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
        none = 0,
        boot1 = 1,
        piano, // main
        boot2
    };

    struct conv16to8x2 {
        uint8_t a = 0;
        uint8_t b = 0;
    };

    struct nt {
        uint8_t note = 0;
        uint32_t timelegth = 0;
    };

    // очередь событий на шину (вместо std::deque): пишет и читает только основной цикл
    const uint8_t ev_size = 16; // делитель 256
    nt ev_buf[ev_size];
    uint8_t ev_w = 0;
    uint8_t ev_r = 0;

    // поле времени на шине (24 бита): бит 23 — подъём демпфера, младшие 23 — интервал в тиках таймера
    const uint32_t LEN_MAX = 0x7FFFFE;   // насыщение поля времени (≈61.7 мс при тике 7.35 нс), раньше старшие биты молча отрезались
    const uint32_t LEN_NONE = 0x7FFFFF;  // note-off без замера скорости
    const uint32_t LIFT_FLAG = 0x800000; // подъём демпфера; младшие биты — интервал R↓→G↓ (0 = не замерен)
#define SEND_LIFT 1                       // 1 — только вместе с send_damper_lift = true в H7
    const uint32_t TICK_US = 136;                 // тиков TIM2/TIM3/TIM5 в микросекунде (PSC 0)
    const uint32_t TICK_OLD = 14;                 // тиков в прежнем тике TIM2 (PSC 13, 102.9 нс) — H7 делит на столько же
    const uint32_t SETTLE = 3 * TICK_US;          // установление ЦАП + компаратора после смены порога
    const uint32_t OFF_POLL = 500 * TICK_US;      // 0.5 мс: опрос уровня, пока клавиша между R и G
    const uint32_t OFF_LIFT_WAIT = 15000 * TICK_US; // 15 мс: ниже R, но не до G — подъём без скорости

    struct dac_reg {
        __IO uint32_t* pregSet = nullptr;
        __IO uint32_t* pregRead = nullptr;
    };

    // Структура для хранения информации о странице и банке
    // typedef struct {
    //     uint32_t start_page;    // Начальная страница
    //     uint32_t end_page;      // Конечная страница (если диапазон охватывает несколько страниц)
    //     uint32_t bank;          // Банк памяти (FLASH_BANK_1 или FLASH_BANK_2)
    //     uint8_t  is_valid;      // Флаг валидности (1 - валидно, 0 - адрес вне диапазона Flash)
    // } FlashPageInfo_t;

    // enum class response {
    //     ok = 0x0C,
    //     fail = 0xDD
    // };

    dac_reg dac_regs[7];

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
    void comp_set_data(const int& compN, const int& data);
    uint32_t comp_read_data(const int& compN);
    COMP_TypeDef* comp_def(const int& i);
    void Read_uint32(uint32_t Address, volatile uint32_t* pData, uint32_t Size);
    void reg_init();
    void cap_init();
    void cap_set(const int i, const bool on, const bool rising);
    void cap_off(const int i);
    uint8_t take(const int i, uint32_t& t);
    void ev_push(const uint8_t note, const uint32_t len);
    uint32_t sat(const uint32_t t);
    void reset_all();
    void comp_idle(const int i);
    void on_step(const uint8_t i, const uint8_t n);
    void off_step(const uint8_t i, const uint8_t n);
    void arm(const int i, const int dac, const bool rising, const int8_t wait, const bool preset, const bool check);
    void probe(const int i, const int dac, const bool rising, const int8_t wait);
    bool arm_busy(const int i);
    bool past(const int i, const bool rising);
    // FlashPageInfo_t GetPageAndBank(uint32_t Address, uint32_t Size);
    // void Erase(uint32_t Page, uint32_t Banks);
    // void Flash_uint32(uint32_t Address, volatile uint32_t* Data, uint32_t size);
    // void Set_tx_s(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);
    // void USART2_Send_settings();

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
