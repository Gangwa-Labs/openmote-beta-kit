# OpenMote library reference

Two classes: `OpenMote` for the board itself, `OpenMoteIR` for infrared.
Pin numbers alone live in `OpenMotePins.h` if that's all you need.

```cpp
#include <OpenMote.h>     // board
#include <OpenMoteIR.h>   // infrared (optional)

OpenMote mote;
```

---

## Setup

### `bool begin(bool initSensors = true)`
Configures every pin and brings up the I2C sensors. Returns `true` if everything
came up; on `false`, check `imuAvailable()` / `magAvailable()` — buttons, LEDs,
rumble and battery work regardless.

Pass `false` to skip sensor init and leave the I2C bus alone.

### `void update()`
**Call this once at the top of every `loop()`.** Debouncing and the rumble timer
both live here. Without it `buttonPressed()` never fires and a timed rumble never
stops.

---

## Buttons

Twelve buttons, active low, debounced by 15 ms. Identify them with the `BTN_*`
constants: `BTN_UP`, `BTN_DOWN`, `BTN_LEFT`, `BTN_RIGHT`, `BTN_A`, `BTN_B`,
`BTN_1`, `BTN_2`, `BTN_PLUS`, `BTN_MINUS`, `BTN_HOME`, `BTN_POWER`.

| Method | Returns |
|--------|---------|
| `buttonPressed(b)` | `true` for one `update()` when the button goes down |
| `buttonReleased(b)` | `true` for one `update()` when it comes back up |
| `buttonDown(b)` | `true` the whole time it is held |
| `buttonHeldFor(b, ms)` | `true` while held for at least `ms` |
| `buttonHoldTime(b)` | milliseconds held so far, or 0 |
| `anyButtonDown()` | `true` if anything is down |
| `buttonMask()` | bitmask, bit *N* = button *N* |
| `OpenMote::buttonName(b)` | `"A"`, `"Up"`, `"Home"`… (static) |
| `OpenMote::buttonPin(b)` | the GPIO number (static) |
| `setDebounceTime(ms)` | change the debounce window |

**Edge vs level** is the distinction that matters:

```cpp
if (mote.buttonPressed(BTN_A)) fireOnce();     // once per press
if (mote.buttonDown(BTN_A))    keepFiring();   // every loop while held
```

---

## LEDs

Numbered **1–4** to match the silkscreen. Out-of-range values are ignored.

| Method | Effect |
|--------|--------|
| `setLED(n, on)` | one LED on or off |
| `toggleLED(n)` | flip one |
| `getLED(n)` | what it was last set to |
| `setLEDs(mask)` | all four at once; bit0 = LED1 … bit3 = LED4 |
| `ledBar(count)` | light the first `count` (0–4) as a bar graph |
| `allLEDsOff()` | |

The LEDs are plain GPIO with series resistors — on/off only. For brightness
control, drive `OM_PIN_LED1`…`OM_PIN_LED4` with your own LEDC channels (note
that channels 0 and 1 are taken by rumble and IR).

---

## Rumble

| Method | Effect |
|--------|--------|
| `rumble(ms, strength = 255)` | buzz for `ms` then stop automatically |
| `rumbleOn(strength = 255)` | run until told otherwise |
| `rumbleOff()` | stop now |
| `isRumbling()` | |

`strength` is a 0–255 PWM duty at 20 kHz (above hearing, so no whine). Below
about 60 the motor may not overcome its own friction.

`rumble()` returns immediately — `update()` is what stops it.

---

## Battery

| Method | Returns |
|--------|---------|
| `batteryVolts()` | volts (a healthy 1S LiPo is 3.3–4.2 V) |
| `batteryPercent()` | 0–100, from a LiPo discharge curve |
| `batteryValid()` | `false` if no successful read has happened yet |
| `setBatteryCalibration(m)` | override the divider multiplier |

**Two caveats, both worth reading once:**

*Wi-Fi blocks the ADC.* The sense pin is GPIO14 = ADC2_CH3, and the Wi-Fi driver
owns ADC2 while the radio is up. Reads fail, and these functions hand back the
last good value instead. Take a reading **before `WiFi.begin()`** so there is
always something sane cached — battery voltage moves slowly enough that a
minute-old value is fine.

*Calibrate it.* The default multiplier (5.2553) comes from the schematic, not
your board, and the ESP32-S3 ADC is nonlinear at the low end. Measure the pack
with a multimeter once:

```cpp
// meter says 3.95 V, library says 4.05 V:
mote.setBatteryCalibration(5.2553f * (3.95f / 4.05f));
```

There is no GPIO connected to the charger, so firmware **cannot** tell whether
the board is charging. D11 is the only charge indicator.

---

## Motion sensors

```cpp
struct IMUData { float ax, ay, az;   // g
                 float gx, gy, gz;   // degrees/second
                 bool  valid; };

struct MagData { float mx, my, mz;   // microtesla
                 bool  valid; };
```

| Method | Returns |
|--------|---------|
| `imuAvailable()` / `magAvailable()` | did the chip answer during `begin()` |
| `readIMU()` | accelerometer + gyroscope sample |
| `readMag()` | magnetic field, rotated into the IMU's axis frame |
| `readMagRaw()` | magnetic field in the magnetometer's own frame |
| `readTemperature()` | IMU die temperature, °C (±2) |
| `pitch()` / `roll()` | degrees, from the accelerometer |
| `headingDegrees()` | tilt-compensated compass heading, 0–360 |
| `setMagCalibration(x, y, z)` | hard-iron offsets, in raw-frame µT |
| `setAccelRange(r)` | `ACCEL_2G` (default), `_4G`, `_8G`, `_16G` |
| `setGyroRange(r)` | `GYRO_245DPS`, `_500DPS`, `_1000DPS` (default), `_2000DPS` |

Always check `.valid` — a `false` means the I2C read failed, not that the board
is stationary.

**Why two magnetometer functions.** The MMC5603NJ is physically rotated relative
to the IMU on the PCB. `readMag()` swaps X/Y and inverts Z so the two sensors
agree with each other, which is what any fusion algorithm needs. `readMagRaw()`
gives you the chip's own frame — use it for calibration, since that is the frame
`setMagCalibration()` expects.

**Calibrate before trusting the heading.** The speaker magnet, the battery and
any steel nearby all bend the field. Example 04 has a working routine: spin the
board through every orientation, record min and max per axis, and feed
`(min+max)/2` to `setMagCalibration()`.

---

## I2C helpers

For the J15 expansion connector, or any register the library doesn't wrap.

| Method | |
|--------|--|
| `i2cWrite(addr, reg, value)` | |
| `i2cRead(addr, reg, buf, len)` | burst read |
| `i2cDetect(addr)` | does anything ACK at this address |
| `i2cScan(Print& = Serial)` | print every device found; returns the count |

The bus is already running at 400 kHz after `begin()`. Addresses `0x6A` (IMU)
and `0x30` (magnetometer) are taken.

---

## Infrared — `OpenMoteIR`

Separate class, because receiving installs a GPIO interrupt and you should ask
for that explicitly.

```cpp
#include <OpenMoteIR.h>
OpenMoteIR ir;

void setup() { ir.begin(); }

void loop() {
    if (ir.available()) {
        if (ir.isNEC()) Serial.printf("0x%02X 0x%02X\n", ir.address(), ir.command());
        ir.resume();                 // re-arm, or nothing else is captured
    }
}
```

### Receiving

| Method | |
|--------|--|
| `begin(enableReceive = true)` | `false` for transmit only |
| `available()` | a frame has arrived and the air has gone quiet |
| `resume()` | re-arm for the next frame — **required after every one** |
| `isNEC()` | did it decode as valid NEC |
| `address()` / `command()` | the decoded bytes |
| `code()` | raw 32 bits, first bit received in bit 31 |
| `isRepeat()` | an NEC "still holding the button" frame |
| `rawLength()` / `rawBuffer()` | raw mark/space timing, microseconds |
| `printRaw(Print&)` | dump the timing, ready to paste into `sendRaw()` |

The buffer stays frozen between `available()` and `resume()`, so you can take
your time reading it.

### Transmitting

| Method | |
|--------|--|
| `sendNEC(address, command)` | a standard NEC frame |
| `sendNECRaw(code)` | replay exactly what `code()` gave you |
| `sendNECRepeat()` | the NEC repeat frame |
| `sendRaw(durations, count, carrierKHz = 38)` | arbitrary timing, mark first |
| `setDutyCycle(percent)` | carrier duty, default 33 |

### Limits

Transmit timing is generated in software with `delayMicroseconds()`, so a busy
Wi-Fi stack can jitter it. That is fine for NEC, which has generous tolerances.
For air conditioners and other long protocols, use **IRremoteESP8266** instead —
it uses the RMT peripheral and needs nothing from this library beyond the pin
numbers (`OM_PIN_IR_RX` = 1, `OM_PIN_IR_TX` = 41).

---

## Pin constants

Every GPIO is available as `OM_PIN_*` — see
[`src/OpenMotePins.h`](../src/OpenMotePins.h) and [PINOUT.md](PINOUT.md).

```cpp
#include <OpenMotePins.h>     // just the numbers, no driver code

pinMode(OM_PIN_LED1, OUTPUT);
Wire.begin(OM_PIN_SDA, OM_PIN_SCL);
```

`OM_PIN_IS_RESERVED(pin)` is true for GPIO 26–37, which belong to flash and
Octal PSRAM and must not be touched.

---

## Peripherals the library deliberately leaves alone

**Audio** — the I2S speaker output and the PDM microphone. Both need ESP-IDF I2S
drivers whose API has changed between IDF versions; wrapping them would make this
library fragile for no real gain. The pin definitions are all here
(`OM_PIN_I2S_*`, `OM_PIN_MIC_*`, `OM_PIN_AMP_SD`), and the demo firmware projects
have working implementations to copy.

**LEDC channels 0 and 1** are used by rumble and the IR carrier. Pick other
channels for your own PWM.
