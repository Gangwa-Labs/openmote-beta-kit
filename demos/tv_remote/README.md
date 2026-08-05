# BLE TV Remote

A Bluetooth HID remote aimed at Fire TV, plus an IR blast for TV power — so one
controller handles both the streaming box and the television.

---

## Controls

| Button | Action | Link |
|---|---|---|
| **D-pad** | Navigation (arrow keys) | BLE |
| **A** | Select / OK (Enter) | BLE |
| **B** | Back (Escape) | BLE |
| **HOME** | Home menu | BLE |
| **+ / −** | Volume up / down | BLE |
| **POWER** | TV power — replays a stored NEC code at 38 kHz | IR |

LED 2 slow-blinks while Bluetooth is disconnected.

## Pairing

Flash it, then look for the OpenMote in your TV or streaming box's Bluetooth
settings and pair as you would any keyboard. The pairing is bonded and stored,
so it reconnects on its own afterwards.

## Build

```bash
pio run -t upload
pio device monitor
```

Requires the whole repo checked out — `platformio.ini` refers to the shared
board definition in `../../boards`.

First build downloads three libraries: ESP32-BLE-Keyboard, NimBLE-Arduino and
IRremoteESP8266.

## ⚠ Do not change the platform line

```ini
platform = espressif32@6.9.0
```

This demo is deliberately pinned to the **Arduino-ESP32 2.x** platform, unlike
the other Arduino demos here which use pioarduino / Arduino 3.x.

The 3.x core ships its own NimBLE host. That collides with NimBLE-Arduino 1.4.3,
which ESP32-BLE-Keyboard depends on, and the build dies with multiple-definition
link errors. Leave it alone.

Two other settings are load-bearing and also worth leaving alone:

- `upload_speed = 115200` — native USB CDC does not survive esptool's default
  jump to 460800 ("No serial data received").
- `upload_flags = --no-stub` — the ESP32-S3's USB-Serial/JTAG drops the
  connection right after esptool's stub loader starts. Flashing through the ROM
  loader is slower but reliable.

## Changing the TV power code

The stored NEC code is a constant in `src/main.cpp`. To capture your own TV's
code, flash the [`ir_learn`](../ir_learn/) demo first, learn the power button,
read the code off the serial log, and paste it in here.

## Notes

**This demo predates the OpenMote library** and uses its own
`include/pin_definitions.h`. It is shipped as-is because it is known-good on
real hardware.

It also uses **IRremoteESP8266** rather than the kit's `OpenMoteIR` class. That
is the right call here: IRremoteESP8266 is RMT-based, so its timing is immune to
Wi-Fi and BLE stack activity, and it understands far more protocols. Reach for
it whenever IR is doing real work rather than a demo.
