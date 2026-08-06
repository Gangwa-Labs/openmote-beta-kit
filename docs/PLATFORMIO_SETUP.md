# Programming the OpenMote from PlatformIO

PlatformIO gets a **real board definition** —
[`boards/openmote_gen3.json`](../boards/openmote_gen3.json) — so flash size,
Octal PSRAM and USB CDC are all correct by default. You write
`board = openmote_gen3` and stop thinking about it.

---

## Fastest path: copy the template

[`templates/platformio-project/`](../templates/platformio-project/) is a
complete, working project. Copy it anywhere, open the folder in VS Code with the
PlatformIO extension installed, and press Build.

```
templates/platformio-project/
├── platformio.ini
├── partitions.csv
├── boards/
│   └── openmote_gen3.json     <- the board definition, project-local
└── src/
    └── main.cpp
```

Because the board file sits in the project's own `boards/` directory, nothing
needs to be installed globally and the project stays self-contained.

## Or add the board to an existing project

Copy `boards/openmote_gen3.json` into **either**:

- `<your-project>/boards/` — project-local, travels with the repo; or
- `~/.platformio/boards/` (`%USERPROFILE%\.platformio\boards\` on Windows) —
  available to every project on your machine.

Then in `platformio.ini`:

```ini
[env:openmote]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.38/platform-espressif32.zip
board = openmote_gen3
framework = arduino

monitor_speed = 115200
monitor_filters = esp32_exception_decoder

board_build.partitions = partitions.csv
board_build.filesystem = littlefs

build_flags = -std=gnu++17
build_unflags = -std=gnu++11

lib_deps = https://github.com/Gangwa-Labs/OpenMote.git
```

### About that platform line

`pioarduino` is a community fork of the espressif32 platform that tracks current
Arduino-ESP32 (3.x, on ESP-IDF 5.x). The official `platform = espressif32` in the
PlatformIO registry lags well behind. Pinning the exact release means a checkout
still builds the same way in a year.

If you would rather use the registry version, `platform = espressif32` works too
— the board file is compatible with both.

## What the board definition sets for you

| Setting | Value |
|---------|-------|
| MCU / variant | `esp32s3` |
| Flash | 16 MB, QIO, 80 MHz |
| PSRAM | Octal (`memory_type: qio_opi`, `psram_type: opi`) |
| Defines | `BOARD_HAS_PSRAM`, `ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`, `ARDUINO_OPENMOTE_GEN3` |
| Partitions | `default_16MB.csv` unless you override it |
| Upload | esptool at 460800 baud |
| Debug | built-in USB-JTAG |

`ARDUINO_OPENMOTE_GEN3` is defined for you, so library code can do:

```cpp
#ifdef ARDUINO_OPENMOTE_GEN3
    // OpenMote-specific path
#endif
```

## Partition table

The template ships a 16 MB layout with two 3 MB OTA slots and a ~10 MB
filesystem:

```
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xE000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x300000,
app1,     app,  ota_1,   0x310000, 0x300000,
spiffs,   data, spiffs,  0x610000, 0x9F0000,
```

The data partition must be **named `spiffs`** even when it holds a LittleFS
image — both the Arduino core and PlatformIO look it up by that label. Naming it
`littlefs` produces a "non-matching type/subtype" warning and a filesystem your
sketch cannot mount.

Don't need OTA? Delete `app1` and `otadata` and give the space to the app or the
filesystem.

## Common commands

```bash
pio run                 # build
pio run -t upload       # build and flash
pio device monitor      # serial monitor
pio run -t uploadfs     # build and flash the LittleFS image from data/
pio run -t clean        # clean
```

If `pio run -t upload` hangs, see [FLASHING.md](FLASHING.md) — there is a known
issue with USB-JTAG auto-reset on Windows and a reliable workaround.
