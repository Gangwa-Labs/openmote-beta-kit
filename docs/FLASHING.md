# Flashing an OpenMote

Normally you just press Upload. This page is for when that doesn't work — which,
on Windows, it sometimes doesn't.

---

## The normal way

Plug in USB-C, select the port, upload. The board resets itself into download
mode, takes the firmware, and reboots.

## Entering download mode by hand

The ESP32-S3 enters download mode when **GPIO0 is held low at reset**. On the
OpenMote, GPIO0 is the **Power / Sync button (S1)**.

1. Unplug the USB-C cable.
2. **Hold the Power button down.**
3. Plug the cable back in while still holding.
4. Release after a second or two.

The board is now sitting in the ROM bootloader waiting for firmware. It will not
run your sketch until it is reset again. A new serial port usually enumerates at
this point — often a *different* port number from the normal one, so re-check
your port selection.

## The Windows USB-JTAG hang

**Symptom:** `pio run -t upload` (or an Arduino IDE upload) prints
`Connecting....` and never finishes, even though the port exists and the board is
plugged in. Power-cycling doesn't help.

**Cause:** esptool tries to put the board into download mode by toggling the USB
CDC control lines. Through the ESP32-S3's *native* USB-JTAG bridge on Windows,
that reset sequence is unreliable — it can leave the USB stack wedged rather than
resetting the chip.

**Fix:** put the board in download mode manually (above), then tell esptool not
to attempt a reset of its own.

### PlatformIO

Add this to the environment in `platformio.ini`:

```ini
upload_flags =
    --before
    no_reset
```

Then: enter download mode by hand, and run `pio run -t upload`.

### esptool directly

After a normal `pio run`, the artifacts land in `.pio/build/<env>/`:

```bash
esptool --chip esp32s3 --port COM5 --baud 460800 \
  --before no_reset --after hard_reset \
  write_flash -z \
  0x0     .pio/build/openmote/bootloader.bin \
  0x8000  .pio/build/openmote/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/openmote/firmware.bin
```

Note the **bootloader goes at 0x0** on the ESP32-S3 — not 0x1000 as on the
original ESP32.

To flash only the application (faster, and fine if the bootloader and partition
table haven't changed):

```bash
esptool --chip esp32s3 --port COM5 --before no_reset --after hard_reset \
  write_flash -z 0x10000 .pio/build/openmote/firmware.bin
```

### Worth trying: `--before usb_reset`

esptool has a reset mode specifically for native-USB parts:

```
--before usb_reset
```

It may handle the USB-JTAG bridge correctly where the default `default_reset`
does not, and would save the manual button-holding. **This has not been verified
on a Gen 3.3 board yet** — if you try it, please report whether it works.

## Erasing a board that is misbehaving

A full erase clears NVS, Wi-Fi credentials and any filesystem, and often clears
up "it flashed fine but behaves oddly" problems:

```bash
esptool --chip esp32s3 --port COM5 --before no_reset erase_flash
```

Arduino IDE equivalent: **Tools → Erase All Flash Before Sketch Upload →
Enabled** for one upload, then set it back to Disabled.

## Recovering a board that won't boot

A sketch that crashes immediately, or that reconfigures the USB pins, can leave
the board looking dead. It isn't — the ROM bootloader always wins:

1. Hold the Power button, plug in USB, release.
2. Erase the flash (above).
3. Upload a known-good sketch, e.g. `01_HelloOpenMote`.

There is no way to brick the board from software. The bootloader lives in mask
ROM and cannot be overwritten.

## Things that look like flashing problems but aren't

**The rumble motor buzzes while flashing.** Expected on Gen 3.3. GPIO39 is MTCK,
which the chip force-pulls-up in download mode; that partially turns on the
motor's MOSFET. No firmware is running, so nothing can prevent it. It stops as
soon as the board boots. Fixed in Gen 3.4 by changing R20 from 10 kΩ to 1 kΩ.

**No serial output after a successful upload.** Almost always
**USB CDC On Boot** set to Disabled (Arduino IDE), or a missing
`-DARDUINO_USB_CDC_ON_BOOT=1` (PlatformIO — the supplied board file has it).
The board's UART pins are wired to the D-pad, so USB CDC is the only console.

**Port disappears after upload.** Normal on native USB: the chip re-enumerates
when it reboots. Wait a second or two and reopen the monitor. If you are using
`USB Mode: USB-OTG (TinyUSB)` for HID work, the CDC port won't come back at all
until you flash something that isn't a HID sketch.
