# Air Mouse

Turns the OpenMote into an **absolute-pointing USB mouse**: where you aim the
controller is where the cursor goes, as if an invisible line connected the two.

Enumerates as a USB HID pointer alongside a CDC serial port, so you can tune it
live while it is being used as a mouse.

---

## Controls

| Button | Action |
|---|---|
| **A** | Left click — hold while moving to drag |
| **B** | Right click |
| **HOME** | Recentre — point at the middle of the screen and tap |
| **+ / −** | Sensitivity (narrower / wider field of view) |
| **1 / 2** | Scroll up / down |
| **POWER** | Pause / resume pointing |
| **D-pad Down** | Re-zero the gyro (hold the board still) |
| **D-pad Left/Right** | Invert X / Y axis |

Serial console (115200) accepts the same tuning live: `c` recentre, `r` reset
filter, `g` gyro cal, `+`/`−` field of view, `x`/`y` invert axis, `s` swap axes,
`d` toggle the debug stream.

## How it points

6-DOF fusion through a Mahony filter. The board's aim axis (**+Y**, the top
edge) is rotated into the world frame; its azimuth maps to screen X and its
elevation to screen Y across a tunable field of view, sent as an absolute HID
report. Because the aim axis *is* the pointing axis, twisting your wrist never
moves the cursor.

## No magnetometer — what that means for you

This is the **magnetometer-free** build, and it is the one that matters: the
MMC5603NJ is not in the final hardware.

- **Pitch and roll never drift.** Gravity is an absolute reference and the
  accelerometer measures it directly.
- **Yaw drifts.** With no magnetic reference there is nothing to correct
  integrated gyro error, so the aim slowly wanders sideways over tens of
  seconds. That is physics, not a tuning problem.

The fix is **HOME**: point at the centre of the screen and tap. Re-zeroing takes
under a second and becomes muscle memory quickly. Zeroing the gyro bias (D-pad
Down, held still) makes the drift much slower to begin with.

## Build

```bash
pio run -t upload
pio device monitor
```

Requires the whole repo checked out — `platformio.ini` refers to the shared
board definition in `../../boards`.

## Notes

**This demo predates the OpenMote library** and talks to the IMU directly
through its own `include/pin_definitions.h`. It is shipped as-is because it is
known-good on real hardware; rewriting proven firmware to route through a new
library is a good way to introduce bugs. The pin numbers agree with
[`OpenMotePins.h`](https://github.com/Gangwa-Labs/OpenMote/blob/main/src/OpenMotePins.h) — they are just spelled
differently.

**Calibration is stored in NVS** and survives power cycles and re-flashes of the
same firmware.

**If the cursor moves the wrong way:** press `x` if left/right is reversed, `y`
if up/down is reversed, or `s` if moving sideways moves the cursor vertically.
