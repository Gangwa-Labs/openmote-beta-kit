# OpenMote Gen 3.3 — Pin Definitions

Every GPIO on the board. The machine-readable version of this table is
[`src/OpenMotePins.h`](../src/OpenMotePins.h) — if the two ever disagree, the
header is what the code actually uses.

**MCU:** ESP32-S3-WROOM-1-N16R8 — 16 MB flash, 8 MB **Octal** PSRAM.

---

## By GPIO number

The ESP32-S3 has no GPIO 22–25; they are omitted rather than missing.

| GPIO | Signal | Function | ADC | Notes |
|-----:|--------|----------|-----|-------|
| 0 | POWER_BUTT | Power / Boot button (S1) | — | ⚠ strapping pin — held low at reset = download mode |
| 1 | IRM | IR receiver output (IRM-56384) | ADC1_CH0 | **active low** |
| 2 | RIGHT_BUTT | D-pad Right | ADC1_CH1 | |
| 3 | LED4 | Player LED 4 (D4) | ADC1_CH2 | ⚠ strapping pin (JTAG source select) |
| 4 | +BUTT | Plus button | ADC1_CH3 | |
| 5 | LRCLK | I²S word select, to amp | ADC1_CH4 | |
| 6 | BCLK | I²S bit clock, to amp | ADC1_CH5 | |
| 7 | DIN | I²S data, to amp | ADC1_CH6 | |
| 8 | 2_BUTT | Button 2 | ADC1_CH7 | |
| 9 | LED2 | Player LED 2 (D2) | ADC1_CH8 | |
| 10 | LED1 | Player LED 1 (D1) | ADC1_CH9 | |
| 11 | MIC_DATA | PDM microphone data | ADC2_CH0 | |
| 12 | MIC_CLK | PDM microphone clock | ADC2_CH1 | |
| 13 | −BUTT | Minus button | ADC2_CH2 | |
| 14 | BATT_LEVEL | Battery voltage tap | ADC2_CH3 | ⚠ ADC2 — unusable while Wi-Fi is on |
| 15 | SCL | I²C clock | ADC2_CH4 | shared: IMU, J15 |
| 16 | SDA | I²C data | ADC2_CH5 | shared: IMU, J15 |
| 17 | HOME_BUTT | Home button | ADC2_CH6 | |
| 18 | 1_BUTT | Button 1 | ADC2_CH7 | |
| 19 | USB_D− | Native USB D− (J5) | ADC2_CH8 | do not repurpose |
| 20 | USB_D+ | Native USB D+ (J5) | ADC2_CH9 | do not repurpose |
| 21 | SD_MODE | Audio amp shutdown / mode | — | analog mode pin — see below |
| 26–32 | — | **SPI flash** | — | 🚫 reserved by the module |
| 33–37 | — | **Octal PSRAM** | — | 🚫 reserved on the N16R8 |
| 38 | A_BUTT | A button | — | |
| 39 | RMBL | Rumble motor gate (Q1) | — | ⚠ MTCK — pulled up in download mode, see below |
| 40 | LEFT_BUTT | D-pad Left | — | MTDO — also pulled up in download mode (harmless) |
| 41 | IR40 | IR LED transmit (D6 via Q2) | — | MTDI |
| 42 | B_BUTT | B button (trigger) | — | MTMS |
| 43 | UP_BUTT | D-pad Up | — | ⚠ default U0TXD |
| 44 | DOWN_BUTT | D-pad Down | — | ⚠ default U0RXD |
| 45 | IO_Breakout1 | Expansion / debug pad | — | ⚠ strapping pin (VDD_SPI) |
| 46 | LED3 | Player LED 3 (D3) | — | ⚠ strapping pin (ROM message) |
| 47 | INT2 | IMU interrupt 2 | — | |
| 48 | INT1 | IMU interrupt 1 | — | |

---

## By function

### I²C bus
Shared by the IMU and the J15 expansion connector.
Pull-ups are on the board already.

| Signal | GPIO |
|--------|-----:|
| SCL | 15 |
| SDA | 16 |

| Device | Address | ID register | Expected |
|--------|--------:|-------------|----------|
| LSM6DS3TR-C IMU | `0x6A` | `WHO_AM_I` (0x0F) | `0x6A` |

### Buttons
All twelve are active **low** and need `INPUT_PULLUP`.

| Button | GPIO | | Button | GPIO |
|--------|-----:|-|--------|-----:|
| Up | 43 | | A | 38 |
| Down | 44 | | B (trigger) | 42 |
| Left | 40 | | 1 | 18 |
| Right | 2 | | 2 | 8 |
| Plus (+) | 4 | | Home | 17 |
| Minus (−) | 13 | | Power | 0 |

### Player LEDs
Active **high**, driven straight from the GPIO through a series resistor.

| LED | GPIO | Silkscreen |
|-----|-----:|------------|
| 1 | 10 | D1 |
| 2 | 9 | D2 |
| 3 | 46 | D3 |
| 4 | 3 | D4 |

### Audio out — MAX98357A
| Signal | GPIO |
|--------|-----:|
| LRCLK | 5 |
| BCLK | 6 |
| DIN | 7 |
| SD_MODE | 21 |

### Audio in — PDM microphone
| Signal | GPIO |
|--------|-----:|
| MIC_DATA | 11 |
| MIC_CLK | 12 |

The L/R strap on the PCB selects the **left** PDM slot.

### Infrared
| Signal | GPIO | Notes |
|--------|-----:|-------|
| Receive | 1 | IRM-56384, 38 kHz, active low |
| Transmit | 41 | 40° IR LED through an MMBT2222A |

### Other
| Signal | GPIO |
|--------|-----:|
| Rumble motor | 39 |
| Battery sense | 14 |
| IMU INT1 / INT2 | 48 / 47 |
| Expansion pad | 45 |

### J15 — expansion connector
JST-SH 4-pin, Qwiic-style: 3V3, GND, SDA, SCL. **Check the silkscreen for the
pin order before you wire anything up** — it is not guaranteed to match the
Qwiic colour convention.

---

## Things that will bite you

### 🚫 GPIO 26–37 are gone
Flash uses 26–32 and the Octal PSRAM uses 33–37. Touching them crashes or hangs
the board. `OM_PIN_IS_RESERVED(pin)` in the header will tell you.

### ⚠ GPIO14 battery sense is on ADC2
ADC2 belongs to the Wi-Fi radio while the radio is up, and reads fail with
`ESP_ERR_TIMEOUT`. `OpenMote::batteryVolts()` handles this by returning the last
successful reading, but the practical advice is: **take a reading before
`WiFi.begin()`**. Battery voltage moves slowly, so a value from a minute ago is
fine. Fixed in Gen 3.4 by moving the tap to ADC1.

### ⚠ GPIO39 rumble twitches during flashing
GPIO39 is MTCK, which the ESP32-S3 force-pulls-up in download mode regardless of
eFuse settings. Against the 10 kΩ gate pulldown (R20) that puts ~0.6 V on the
SI2302 gate — past its ~0.45 V threshold — so **the motor buzzes while you are
uploading firmware.** This is hardware, not software; no firmware is running at
that point. Harmless. Fixed in Gen 3.4 by dropping R20 to 1 kΩ.

### ⚠ GPIO43/44 are the D-pad, not the serial port
These are the default UART0 TX/RX. On the OpenMote they are Up and Down.
Use **USB CDC** for the serial console — pressing D-pad Up will otherwise inject
garbage into a UART0 log. `OpenMote::begin()` claims both pins as inputs.

### ⚠ Strapping pins in use
GPIO0 (Power button), GPIO3 (LED4), GPIO45 (breakout), GPIO46 (LED3). Driving
them at runtime is fine; just don't hold them at the wrong level across a reset.
In particular, holding the Power button while resetting drops the board into
download mode — which is exactly how you recover a bricked board.

### ⚠ SD_MODE (GPIO21) is not a plain digital enable
On the MAX98357A, `SD` is an **analog** pin: the chip measures the voltage and
uses it for both shutdown and output-channel selection.

| Voltage on SD | Result |
|---------------|--------|
| < 0.16 V | shutdown |
| 0.16 – 0.77 V | on, output = (L+R)/2 |
| 0.77 – 1.4 V | on, right channel only |
| > 1.4 V | on, left channel only |

GPIO21 reaches that pin through a **680 kΩ series resistor (R17)** and the
three-pole solder jumper **JP1**, which together bias the node into one of those
bands. Intended behaviour is *HIGH = enabled in mono mix, LOW = shutdown*.

In practice: 680 kΩ pulls very weakly, so the node settles slowly and the exact
band depends on how JP1 is set on your particular board. Treat GPIO21 as
enable/mute only, drive it explicitly before streaming audio, and if you have a
scope, measure the SD pin voltage on your board once to confirm.

---

**Board revision:** Gen 3.3
**Source:** `PCB Files/OpenMoteGen3.3` KiCad schematic + production BOM
