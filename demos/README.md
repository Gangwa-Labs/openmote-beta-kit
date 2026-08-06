# OpenMote demos

Four complete, flashable applications. Unlike the small teaching sketches in the
[library's `examples/`](https://github.com/Gangwa-Labs/OpenMote/tree/main/examples),
each of these is a full PlatformIO project you can build and use as-is.

| Demo | What it does | Build with | Uses the library |
|---|---|---|---|
| [`ir_learn/`](ir_learn/) | Learning universal remote — capture IR codes and bind them to buttons | PlatformIO | ✅ yes |
| [`airmouse/`](airmouse/) | Absolute-pointing USB HID air mouse | PlatformIO | ❌ predates it |
| [`tv_remote/`](tv_remote/) | BLE HID remote for Fire TV, plus IR power | PlatformIO | ❌ predates it |
| [`usb_microphone/`](usb_microphone/) | Enumerates as a USB audio capture device | **`idf.py`** | ❌ not Arduino |

Each is independent — build one without touching the others:

```bash
cd demos/ir_learn
pio run -t upload
pio device monitor
```

`usb_microphone` is the exception: it is pure ESP-IDF and **cannot be built with
PlatformIO** (see its README for why). It has no `platformio.ini` on purpose.

> **Clone the whole repo.** Every demo's `platformio.ini` points at the shared
> board definition in [`../../boards`](../boards/) via `boards_dir`, and
> `ir_learn` additionally pulls the library from the repo root. Copying a single
> demo folder somewhere else will break those relative paths.

---

## ir_learn — start here

The newest demo and the only one written against the OpenMote library, so it is
also the best worked example of `OpenMote` + `OpenMoteIR` in a real application.

Hold **A + B** for two seconds, point a remote at the board, then press the
button you want that code bound to. Mappings persist in NVS. Full walkthrough in
its [README](ir_learn/README.md).

## airmouse

Absolute pointing — where you aim the controller is where the cursor goes.
6-DOF fusion (accelerometer + gyroscope) through a Mahony filter: gravity pins
pitch and roll, and yaw is integrated from the gyro.

This is the **magnetometer-free** build, which is the one that matters — the
MMC5603NJ is not in the final hardware. Without a magnetic reference, yaw has
nothing to correct its drift, so **HOME re-centres the aim** whenever it
wanders. Pitch and roll never drift, because gravity is absolute.

Enumerates as a USB HID pointer alongside a CDC serial port for live tuning.

## tv_remote

BLE HID keyboard aimed at Fire TV — D-pad navigates, A selects, B goes back,
+/− control volume — plus an IR blast for TV power. LED 2 slow-blinks while
Bluetooth is disconnected.

Note it is pinned to the **Arduino-ESP32 2.x** platform (`espressif32@6.9.0`).
That is deliberate: the 3.x core ships its own NimBLE host, which collides with
NimBLE-Arduino 1.4.3 and produces multiple-definition link errors inside
ESP32-BLE-Keyboard. Leave the platform line alone.

It uses **IRremoteESP8266** rather than the kit's IR class, which is the right
call for long or unusual protocols.

## usb_microphone

The odd one out: **pure ESP-IDF C, not an Arduino sketch**, so it cannot use the
OpenMote library at all. The PDM MEMS microphone feeds a ring buffer and the
`espressif/usb_device_uac` component owns the entire USB side.

Plug it in and it appears as *Microphone (usb uac)* — no driver needed.

**Build it with `idf.py`, not PlatformIO.** PlatformIO's ESP-IDF builder trips
over this project's managed components and registers two conflicting compile
actions for `usb_descriptors.c`. That is a builder limitation rather than a
problem with the firmware, and it affects the original project this was copied
from too. There is no `platformio.ini` here so that nobody loses an hour to a
build that cannot succeed — see [its README](usb_microphone/README.md) for the
`idf.py` steps.

It also drives **GPIO21 (the amplifier's SD_MODE) low** at startup to hold the
speaker amp in shutdown — an enabled amp with no I2S clocks just burns current
and hisses.

---

## Why three demos don't use the library

`airmouse`, `tv_remote` and `usb_microphone` all predate the OpenMote library or
sit outside Arduino entirely. They talk to the hardware directly through their
own `pin_definitions.h`.

They are shipped **as-is on purpose**: each is known-good on real boards, and
rewriting proven firmware to route through a new library is a good way to
introduce bugs into the one thing you were confident about. Their pin numbers
agree with [`OpenMotePins.h`](https://github.com/Gangwa-Labs/OpenMote/blob/main/src/OpenMotePins.h) — they just spell them
differently.

For anything new, use the library. `ir_learn` shows what that looks like.
