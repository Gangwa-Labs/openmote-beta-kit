# Troubleshooting

Run **File → Examples → OpenMote → 06_BoardSelfTest** first. It checks
everything the library covers and prints a pass/fail summary you can paste into
a bug report.

---

## Nothing on the serial monitor

| Check | |
|-------|--|
| Baud rate | 115200 |
| Arduino IDE | **USB CDC On Boot → Enabled** |
| PlatformIO | `-DARDUINO_USB_CDC_ON_BOOT=1` (the supplied board file has it) |
| Timing | Add `delay(1000)` after `Serial.begin()` — native USB takes a moment to enumerate |
| USB Mode | If it is `USB-OTG (TinyUSB)`, there is no CDC port by design |

The OpenMote's UART0 pins are wired to the D-pad, so USB CDC is the only console
you get. This is not optional configuration.

## Garbage characters, or phantom button presses in the log

You are logging over UART0, which is GPIO43/44 — the same pins as D-pad Up and
Down. Switch to USB CDC.

## Upload hangs at "Connecting…"

See [FLASHING.md](FLASHING.md). Short version: hold the **Power button**, plug in
USB, release, then upload with `--before no_reset`.

## PSRAM not detected / `ps_malloc()` returns null

The N16R8 has **Octal** PSRAM, not Quad.

- Arduino IDE: **PSRAM → OPI PSRAM**
- PlatformIO: `memory_type: qio_opi` — already set in the supplied board file

Verify at runtime:

```cpp
Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());   // expect ~8388608
```

## Sketch is too big / filesystem is truncated

**Flash Size → 16MB (128Mb)** in the Arduino IDE, and a 16 MB partition scheme.
The default 4 MB scheme silently wastes three quarters of the chip.

## The board crashes when I use a particular pin

GPIO **26–37** are consumed internally by flash and Octal PSRAM. They are not
available, and using them will hang or reset the board.

```cpp
if (OM_PIN_IS_RESERVED(pin)) { /* don't */ }
```

## `buttonPressed()` never returns true

You are not calling `mote.update()` in `loop()`. All debouncing and edge
detection happens there.

Also check you want `buttonPressed()` (fires once per press) and not
`buttonDown()` (true the whole time it's held).

## A button reads as pressed constantly

- Is it the **Power button**? GPIO0 is a strapping pin — if it is held at reset,
  the board sits in download mode and never runs your sketch at all.
- Otherwise, probe the GPIO from the table in [PINOUT.md](PINOUT.md). A solder
  bridge under the switch will hold the line low.

## The rumble motor buzzes while flashing

Expected on Gen 3.3, and not fixable in firmware. GPIO39 is MTCK, which the
ESP32-S3 force-pulls-up in download mode; against the 10 kΩ gate pulldown (R20)
that puts ~0.6 V on the SI2302's gate, past its ~0.45 V threshold. No firmware is
running at that point, so nothing can hold the pin low.

Gen 3.4 changes R20 to 1 kΩ, which drops the gate to ~0.07 V.

## No audio from the speaker

The amplifier's `SD` pin is an **analog** mode pin, not a digital enable, and it
is driven through a 680 kΩ resistor (R17) plus the JP1 solder jumper:

| Voltage on SD | Result |
|---------------|--------|
| < 0.16 V | shutdown |
| 0.16 – 0.77 V | on, (L+R)/2 mono |
| 0.77 – 1.4 V | on, right only |
| > 1.4 V | on, left only |

Drive `OM_PIN_AMP_SD` (GPIO21) **HIGH** before streaming, and give it time — the
node settles slowly through 680 kΩ. If it still won't play, check how JP1 is set
on your board and measure the SD pin voltage with a meter while GPIO21 is high.

## Magnetometer or IMU not found

Run `mote.i2cScan()`. You should see `0x30` (magnetometer) and `0x6A` (IMU).

- Nothing at all → the bus is dead. Check the pull-ups and that nothing plugged
  into J15 is shorting SDA/SCL.
- One missing → likely a solder joint on that chip. The MMC5603NJ is a 0.8 mm
  WLP-4; it is the usual suspect.

## Compass heading is wrong or drifts

Calibrate it — example 04 has a routine. Uncalibrated, the speaker magnet and the
battery dominate the reading.

Then check you are using `readMag()` (rotated into the IMU's frame) rather than
`readMagRaw()` (the chip's own frame) for anything involving both sensors.

## Battery reads 0 V, or a wildly wrong voltage

**0 V or a stale value while Wi-Fi is running** is expected: GPIO14 is on ADC2,
which the Wi-Fi driver takes over. Read the battery before `WiFi.begin()`.
`batteryValid()` tells you whether the cached value is real.

**A consistently wrong voltage** means the divider needs calibrating — see
[API.md](API.md#battery).

## IR won't receive

- The receiver is on the **front edge** of the board. Point the remote at it.
- Only NEC is decoded. `available()` still fires for other protocols; check
  `isNEC()` and use `printRaw()` to see the timing.
- Are you calling `ir.resume()` after every frame? Without it the buffer stays
  frozen and nothing new is captured.
- Fluorescent lights and sunlight both add noise. Try a darker room.

## IR won't transmit

- Range is a few metres, line of sight. Aim the front of the board at the target.
- IR is invisible — check the LED with a phone camera. Most phone front cameras
  show it as a faint purple flicker.
- Software-timed transmission jitters under heavy Wi-Fi load. If timing is
  critical, use IRremoteESP8266 (RMT-based) with `OM_PIN_IR_TX`.

## Library won't compile

- **`OpenMote.h: No such file`** — the library isn't installed. See
  [ARDUINO_IDE_SETUP.md](ARDUINO_IDE_SETUP.md#2-install-the-openmote-library).
- **`ledcAttach` / `ledcSetup` errors** — the library handles both esp32 core 2.x
  and 3.x. If you see these, your core install is likely mid-upgrade; reinstall
  it from Boards Manager.
- **Errors mentioning `esp32` architecture** — this library is ESP32-only. Make
  sure an ESP32-S3 board is selected before compiling.

---

## Reporting a problem

Include:

1. The **06_BoardSelfTest** output.
2. Arduino IDE + esp32 core version, or `platformio.ini`.
3. Your board settings (the Tools menu list).
4. What you expected and what happened.
