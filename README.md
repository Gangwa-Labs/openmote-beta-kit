# OpenMote Beta Kit

Everything you need to program an **OpenMote Gen 3.3** board: a library for the
hardware, the full pin map and component list, and board definitions for the
Arduino IDE and PlatformIO.

The OpenMote is an ESP32-S3 handheld controller — twelve buttons, four player
LEDs, a rumble motor, a 6-axis IMU, a speaker, a microphone, infrared in and
out, and a LiPo battery.

---

## Quick start

**Arduino IDE** — five minutes, nothing custom to install.

1. Add the ESP32 boards URL and install **esp32 by Espressif Systems**.
2. **Sketch → Include Library → Add .ZIP Library…** and select this kit.
3. **Tools → Board → ESP32S3 Dev Module**, then set:
   ```
   PSRAM:            OPI PSRAM          <-- must be OPI
   Flash Size:       16MB (128Mb)       <-- must be 16MB
   USB CDC On Boot:  Enabled            <-- or you get no serial output
   USB Mode:         Hardware CDC and JTAG
   Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
   ```
4. **File → Examples → OpenMote → 01_HelloOpenMote**, upload, open the Serial
   Monitor at 115200.

Full settings table and the reasoning: **[docs/ARDUINO_IDE_SETUP.md](docs/ARDUINO_IDE_SETUP.md)**

**PlatformIO** — copy `templates/platformio-project/` and press Build. It carries
its own board definition, so nothing needs to be installed globally.
See **[docs/PLATFORMIO_SETUP.md](docs/PLATFORMIO_SETUP.md)**.

---

## Hello world

```cpp
#include <OpenMote.h>

OpenMote mote;

void setup() {
    Serial.begin(115200);
    mote.begin();
}

void loop() {
    mote.update();                          // required, every loop

    if (mote.buttonPressed(BTN_A)) {
        mote.setLED(1, true);
        mote.rumble(200);                   // buzz for 200 ms
    }
    if (mote.buttonReleased(BTN_A)) {
        mote.setLED(1, false);
    }

    IMUData imu = mote.readIMU();
    if (imu.valid) {
        Serial.printf("%.2f %.2f %.2f g\n", imu.ax, imu.ay, imu.az);
    }
}
```

---

## What the library covers

| | |
|---|---|
| **Buttons** | All 12, debounced, with press/release edges and hold timing |
| **LEDs** | The 4 player LEDs, individually or as a bar graph |
| **Rumble** | PWM strength control and non-blocking timed buzzes |
| **Battery** | Voltage and percentage, with the ADC2/Wi-Fi conflict handled |
| **IMU** | LSM6DS3TR-C accelerometer + gyroscope, pitch/roll, temperature |
| **Infrared** | NEC send and receive, plus raw timing capture and replay |
| **I2C** | Bus scanner and register access for the J15 expansion port |

**Audio is not included.** The speaker and microphone need ESP-IDF I2S drivers
whose API shifts between IDF versions, and wrapping them would make this library
fragile. All the pin definitions are here, and the demo firmware projects have
working implementations to copy.

---

## Documentation

| | |
|---|---|
| **[PINOUT.md](docs/PINOUT.md)** | Every GPIO, what it does, and the four that will bite you |
| **[COMPONENTS.md](docs/COMPONENTS.md)** | Full BOM with LCSC part numbers and key specs |
| **[API.md](docs/API.md)** | Complete library reference |
| **[ARDUINO_IDE_SETUP.md](docs/ARDUINO_IDE_SETUP.md)** | Board settings, and why there's no custom board package |
| **[PLATFORMIO_SETUP.md](docs/PLATFORMIO_SETUP.md)** | Using the `openmote_gen3` board definition |
| **[FLASHING.md](docs/FLASHING.md)** | Manual download mode, the Windows upload hang, recovery |
| **[TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)** | Symptom-first fixes |

---

## Examples

Under **File → Examples → OpenMote** once installed.

| | |
|---|---|
| `01_HelloOpenMote` | Start here — what's connected, and button events |
| `02_ButtonsAndLEDs` | Edge vs level vs hold detection; LED helpers |
| `03_RumbleAndBattery` | Haptics and power monitoring |
| `04_Motion` | Accelerometer, gyroscope, tilt angles, gyro bias calibration |
| `05_InfraredRemote` | Learn a code from any remote, then replay it |
| `06_BoardSelfTest` | Bring-up test for a new board — **run this first** |

---

## Repository layout

```
openmote-beta-kit/
├── src/                       the library
│   ├── OpenMote.h/.cpp        buttons, LEDs, rumble, battery, sensors
│   ├── OpenMoteIR.h/.cpp      infrared send/receive
│   └── OpenMotePins.h         pin map — usable on its own
├── examples/                  six worked examples
├── docs/                      pinout, components, setup, troubleshooting
├── boards/
│   └── openmote_gen3.json     PlatformIO board definition
└── templates/
    └── platformio-project/    ready-to-build starter project
```

---

## Four things that catch people out

**GPIO 26–37 do not exist for you.** Flash uses 26–32 and the Octal PSRAM uses
33–37. Using them hangs the board. `OM_PIN_IS_RESERVED(pin)` will tell you.

**The serial console must be USB CDC.** The UART0 pins are wired to D-pad Up and
Down, so a UART log gets corrupted by button presses.

**Battery reads fail while Wi-Fi is on.** The sense pin is on ADC2, which the
radio takes priority over. Read it before `WiFi.begin()`; the library caches the
last good value.

**The rumble motor buzzes while you flash.** Hardware quirk of Gen 3.3 — GPIO39
is force-pulled-up in download mode. No firmware is running, so nothing can stop
it. It's harmless, and fixed in Gen 3.4.

---

## Hardware summary

| | |
|---|---|
| MCU | ESP32-S3-WROOM-1-N16R8 — 16 MB flash, 8 MB Octal PSRAM |
| Wireless | Wi-Fi 802.11 b/g/n, Bluetooth 5 LE |
| Input | 12 buttons (D-pad, A/B, 1/2, +/−, Home, Power) |
| Output | 4 player LEDs, rumble motor, speaker |
| Sensors | LSM6DS3TR-C 6-axis IMU (accelerometer + gyroscope) |
| Audio | MAX98357A I2S amp, PDM MEMS microphone |
| Infrared | 38 kHz receiver, 40° transmit LED |
| Power | USB-C charging, 1S LiPo, voltage monitoring |
| Expansion | J15 4-pin I2C (Qwiic-style) |

---

## License

MIT — see [LICENSE](LICENSE).
