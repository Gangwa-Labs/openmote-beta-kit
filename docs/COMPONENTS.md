# OpenMote Gen 3.3 — Component List

Every part on the board, taken from the production BOM
(`PCB Files/OpenMoteGen3.3/OpenMoteGen3.3.csv`).

---

## Active components

| Ref | Part | Package | Function |
|-----|------|---------|----------|
| U8 | **ESP32-S3-WROOM-1-N16R8** | Module | Main MCU: dual-core Xtensa LX7, Wi-Fi b/g/n, Bluetooth 5 LE, 16 MB flash, 8 MB Octal PSRAM, native USB |
| U5 | **LSM6DS3TR-C** | LGA-14 | 6-axis IMU (3-axis accel + 3-axis gyro), I²C `0x6A` |
| U2 | **MAX98357AETE+T** | TQFN-16 | I²S class-D mono audio amplifier, ~3.2 W into 4 Ω |
| U1 | **LMD4030T261-OA1** | MEMS | PDM MEMS microphone, ~−26 dBFS, ~58 dB(A) SNR |
| U4 | **IRM-56384** | MOLD-3pin | 38 kHz IR receiver module, active-low output |
| U3 | **XC6210B332MR** | SOT-23-5 | 3.3 V LDO regulator (Torex) |
| U7 | **ME4054** | SOT-23-5 | Single-cell Li-ion / LiPo linear charger |
| U6 | **BRCL3160MF** | SOT-23-6 | Battery load switch / power path |
| Q1 | **SI2302** | SOT-23 | N-channel MOSFET, low threshold — drives the rumble motor |
| Q2 | **MMBT2222A** | SOT-23 | NPN transistor — drives the IR LED |

## Optoelectronics

| Ref | Qty | Part | Function |
|-----|----:|------|----------|
| D1–D4 | 4 | LED 0603 | Player LEDs 1–4 |
| D11 | 1 | LED 0603 | Charge status indicator (hardware only, no GPIO) |
| D6 | 1 | IR LED, 40° | IR transmitter, through-hole, 5 mm horizontal |
| D7, D8 | 2 | 1N5819 | Schottky diodes, SOD-323 (power path) |

## Switches

| Ref | Qty | Part | Function |
|-----|----:|------|----------|
| SW10–SW13 | 4 | YZA-048 4.8 mm | Tactile switches |
| S1 | 1 | KEY-SMD 4P 6×6 mm | "Sync" / Power / Boot button |

> The board carries twelve button inputs in total; the remaining actuators are
> part of the D-pad and shell assembly rather than discrete BOM lines.

## Connectors

| Ref | Part | Function |
|-----|------|----------|
| J5 | TYPE-C-31-M-12 | USB-C — data + charging (5 V VBUS) |
| J11 | JST-SH 2-pin | LiPo battery |
| J15 | JST-SH 4-pin | I²C expansion (Qwiic-style: 3V3 / GND / SDA / SCL) |
| J6 / J7 | pads | Rumble motor leads |
| JP1 | 3-pole solder jumper | Biases the amplifier SD_MODE pin |

## Resistors (0402 unless noted)

| Ref | Qty | Value | Purpose |
|-----|----:|-------|---------|
| R19 | 1 | 200 kΩ | Battery divider, high side |
| R25, R26 | 2 | 47 kΩ | R26 = battery divider low side |
| R17 | 1 | 680 kΩ | Series into the amp SD_MODE pin |
| R10, R11 | 2 | 36 Ω (1206) | IR LED current limit |
| R8, R9 | 2 | 5.1 kΩ | USB-C CD pulldowns |
| R7 | 1 | 220 Ω | |
| R6, R28 | 2 | 100 Ω | |
| R2–R5, R18, R22, R23 | 7 | 1 kΩ | LED current limiting and general |
| R1, R15, R16, R20, R24, R27, R29 | 7 | 10 kΩ | Pull-ups / pulldowns — **R20 is the rumble gate pulldown** |

## Capacitors (0402)

| Ref | Qty | Value |
|-----|----:|-------|
| C16, C18, C25, C29, C30, C35, C36 | 7 | 10 µF |
| C2, C19–C21, C24, C26, C28, C31, C32, C34, C37 | 11 | 1.0 µF |
| C1, C3, C17, C22, C23, C27, C33, C38 | 8 | 100 nF |

---

## Key specifications

### ESP32-S3-WROOM-1-N16R8
- Dual-core Xtensa LX7 at 240 MHz, 320 KB internal SRAM.
- 16 MB QIO flash, 8 MB **Octal (OPI)** PSRAM — must be enabled in the build
  config or `ps_malloc()` returns null.
- Wi-Fi 802.11 b/g/n, Bluetooth 5 LE, onboard antenna.
- Native USB on GPIO19/20 (USB CDC, TinyUSB, and USB-JTAG debugging).

### LSM6DS3TR-C IMU
- Accelerometer ±2 / ±4 / ±8 / ±16 g; gyroscope ±125 … ±2000 dps.
- Output data rate up to 6.66 kHz. The library defaults to 416 Hz.
- Two interrupt lines wired out (GPIO48, GPIO47) for wake-on-motion.

### MAX98357A amplifier
- 8 kHz–96 kHz sample rates, 16/24/32-bit I²S.
- One speaker on the board, so the output mode is the (L+R)/2 mono mix.
- `SD` is an analog mode pin, not a digital enable — see
  [PINOUT.md](PINOUT.md#-sd_mode-gpio21-is-not-a-plain-digital-enable).

### Battery monitoring
- Divider R19 = 200 kΩ / R26 = 47 kΩ → ratio 47/247 = **0.1903**.
- `V_batt = V_adc × 5.2553`. At 4.2 V the ADC sees about 0.80 V.
- 12-bit ADC, 11 dB attenuation. **Calibrate against a multimeter** — the
  ESP32-S3 ADC is noticeably nonlinear at the bottom of its range.
- There is no GPIO connected to the charger, so firmware cannot tell whether the
  board is charging. D11 is the only charge indicator.

---

## Datasheets

- [ESP32-S3-WROOM-1](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [LSM6DS3TR-C](https://www.st.com/resource/en/datasheet/lsm6ds3tr-c.pdf)
- [MAX98357A](http://datasheets.maximintegrated.com/en/ds/MAX98357A-MAX98357B.pdf)
- [XC6210 LDO](https://www.torexsemi.com/file/xc6210/XC6210.pdf)
- [MMBT2222A](https://assets.nexperia.com/documents/data-sheet/MMBT2222A.pdf)
