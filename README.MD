# PIANO_G4_cbt6 — Sensor board firmware / Прошивка плат-датчиков

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20569249.svg)](https://doi.org/10.5281/zenodo.20569249)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: STM32G4](https://img.shields.io/badge/MCU-STM32G474-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32g474cb.html)

> Part of the custom optical digital MIDI piano project. Companion board:
> [**PIANO_H7**](https://github.com/ScherbakovAl/PIANO_H7) (controller board).
>
> Часть проекта самодельного цифрового MIDI-пианино на оптических датчиках.
> Парная плата: [**PIANO_H7**](https://github.com/ScherbakovAl/PIANO_H7) (плата-контроллер).

---

## English

### Overview

**PIANO_G4_cbt6** is the firmware for the **sensor boards** of a self-built
digital MIDI piano with optical key sensing. Each key's hammer flies past
optical sensors; the board measures the **hammer flight time** between sensor
points and reports it to the controller, which converts it into a realistic
note velocity — the principle used in high-end digital pianos.

The instrument is split across two board types:

| Board | MCU | Role |
|-------|-----|------|
| [PIANO_H7](https://github.com/ScherbakovAl/PIANO_H7) | STM32H723VGTx | Master controller: UI, USB-MIDI, manages the sensor bus |
| **PIANO_G4_cbt6** (this repo) | STM32G474 | Distributed sensor readers (comparators + timers) |

The boards were fabricated and assembled in late summer–autumn 2025 and are
used to play on a real instrument.

### What a G4 board does

- **Optical sensing with on-chip comparators:** uses the STM32G4's analog
  comparators (7 per chip) to detect when a hammer crosses each optical
  sensor's threshold.
- **Hammer-flight timing:** high-resolution timers (100 ns / ~100 µs scales)
  measure the time the hammer takes between sensor points, which encodes how
  hard the key was struck.
- **Note ON / OFF detection:** each board carries an address; addresses 1–13
  handle note-on sensors, 14–26 handle note-off (key release) sensors.
- **Multidrop UART link:** reports note timings to the PIANO_H7 controller and
  receives settings/calibration over USART2 using hardware address detection,
  so many boards share one bus.
- **Self-calibration:** per-comparator threshold calibration, driven on demand
  from the controller.
- **Dual bootloader:** carries its own bootloader (two versions present) so the
  controller can update the G4 main firmware over the UART bus.

### Hardware

- MCU: **STM32G474** (CBT6 / 48-pin)
- Sensing: 7 on-chip analog comparators per board on optical sensors
- Link: multidrop UART (USART2, address-detect) to the PIANO_H7 board
- Hardware experiments (impedance test, IBIS) are in
  [`kicad_for_test_impedance/`](kicad_for_test_impedance/) and
  [`ibis model/`](ibis%20model/).

### Repository layout

```
Piano_G4/                  Application firmware (piano_g4.cpp/.hpp)
Core/, Drivers/            STM32 HAL/LL, CMSIS, startup
G4_bootloader/             Bootloader (v1)
G4_bootloader_v2/          Bootloader (v2)
kicad_for_test_impedance/  Impedance test (KiCad)
ibis model/                IBIS models / signal-integrity
cmake/, CMakeLists.txt, CMakePresets.json
```

### Building

The project uses CMake + the ARM GNU toolchain (configured via STM32CubeMX /
the included presets).

```bash
cmake --preset <preset>     # see CMakePresets.json
cmake --build --preset <preset>
```

Flash with your usual ST tool (ST-Link / DFU), or update over the UART bus from
the PIANO_H7 controller.

### License

Released under the [MIT License](LICENSE).

### Citation

If you use or reference this work, please cite it via its DOI (see the badge
above) or the [`CITATION.cff`](CITATION.cff) file.

---

## Русский

### Описание

**PIANO_G4_cbt6** — это прошивка **плат-датчиков** самодельного цифрового
MIDI-пианино с оптическим считыванием нажатий. Молоточек каждой клавиши
пролетает мимо оптических датчиков; плата измеряет **время пролёта молоточка**
между точками сенсоров и передаёт его контроллеру, который вычисляет
реалистичную velocity ноты — принцип, используемый в дорогих цифровых пиано.

Инструмент состоит из двух типов плат:

| Плата | МК | Роль |
|-------|-----|------|
| [PIANO_H7](https://github.com/ScherbakovAl/PIANO_H7) | STM32H723VGTx | Главный контроллер: интерфейс, USB-MIDI, управление шиной датчиков |
| **PIANO_G4_cbt6** (этот репозиторий) | STM32G474 | Распределённые платы-датчики (компараторы + таймеры) |

Платы изготовлены и собраны в конце лета — осенью 2025 года и используются для
игры на реальном инструменте.

### Что делает плата G4

- **Оптическое считывание встроенными компараторами:** использует аналоговые
  компараторы STM32G4 (по 7 на чип) для детектирования момента, когда
  молоточек пересекает порог оптического датчика.
- **Замер времени пролёта молоточка:** таймеры высокого разрешения (масштабы
  100 нс / ~100 мкс) измеряют время прохождения молоточка между точками
  сенсоров — это и кодирует силу удара по клавише.
- **Детектирование note ON / OFF:** каждая плата имеет адрес; адреса 1–13
  обрабатывают датчики note-on, 14–26 — датчики note-off (отпускание клавиши).
- **Многоточечная шина UART:** передаёт тайминги нот контроллеру PIANO_H7 и
  принимает настройки/калибровку по USART2 с аппаратным определением адреса,
  поэтому много плат используют одну шину.
- **Самокалибровка:** калибровка порогов по каждому компаратору, запускается по
  команде контроллера.
- **Двойной бутлоадер:** собственный бутлоадер (присутствуют две версии),
  чтобы контроллер мог обновлять основную прошивку G4 по шине UART.

### Аппаратная часть

- МК: **STM32G474** (CBT6 / 48 выводов)
- Считывание: 7 встроенных аналоговых компараторов на оптических датчиках
- Связь: многоточечный UART (USART2, address-detect) к плате PIANO_H7
- Аппаратные эксперименты (тест импеданса, IBIS) — в
  [`kicad_for_test_impedance/`](kicad_for_test_impedance/) и
  [`ibis model/`](ibis%20model/).

### Структура репозитория

```
Piano_G4/                  Прошивка приложения (piano_g4.cpp/.hpp)
Core/, Drivers/            STM32 HAL/LL, CMSIS, startup
G4_bootloader/             Бутлоадер (v1)
G4_bootloader_v2/          Бутлоадер (v2)
kicad_for_test_impedance/  Тест импеданса (KiCad)
ibis model/                IBIS-модели / целостность сигналов
cmake/, CMakeLists.txt, CMakePresets.json
```

### Сборка

Проект использует CMake + ARM GNU toolchain (настройка через STM32CubeMX /
включённые пресеты).

```bash
cmake --preset <preset>     # см. CMakePresets.json
cmake --build --preset <preset>
```

Прошивка — обычным инструментом ST (ST-Link / DFU) либо обновление по шине UART
с контроллера PIANO_H7.

### Лицензия

Распространяется под [лицензией MIT](LICENSE).

### Цитирование

При использовании или ссылке на эту работу, пожалуйста, цитируйте по DOI
(см. бейдж выше) или через файл [`CITATION.cff`](CITATION.cff).
