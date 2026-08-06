# Flashing an OpenMote

Normally you just press Upload. This page is for when that doesn't work.

---

## The normal way

Plug in USB-C, select the port, upload. The board resets itself into download
mode, takes the firmware, and reboots.

## Entering download mode by hand

The ESP32-S3 enters download mode when **GPIO0 is held low at reset**. On the
OpenMote, GPIO0 is the **Power / Sync button (S1)**.

1. Plug the USB-C cable in.
2. Remove the battery cover.
3. **Hold down the Power button** on the top of the controller.
4. Tap the **Reset button** on the back of the controller.
5. Release the Power button after a second or two.

The board is now sitting in the ROM bootloader waiting for firmware. It will not
run your sketch until it is reset again. A new serial port usually enumerates at
this point — often a *different* port number from the normal one, so re-check
your port selection.

## Things that look like flashing problems but aren't

**The rumble motor buzzes while flashing.** Expected on Gen 3.3. GPIO39 is MTCK,
which the chip force-pulls-up in download mode; that partially turns on the
motor's MOSFET. No firmware is running, so nothing can prevent it.

It stops as soon as the board boots — so if you entered download mode by hand,
it will keep buzzing after the upload finishes, because the board is still
sitting in the bootloader. **Tap the Reset button on the back and it stops.**

Fixed in Gen 3.4 by changing R20 from 10 kΩ to 1 kΩ.

**No serial output after a successful upload.** Almost always
**USB CDC On Boot** set to Disabled (Arduino IDE), or a missing
`-DARDUINO_USB_CDC_ON_BOOT=1` (PlatformIO — the supplied board file has it).
The board's UART pins are wired to the D-pad, so USB CDC is the only console.

**Port disappears after upload.** Normal on native USB: the chip re-enumerates
when it reboots. Wait a second or two and reopen the monitor. If you are using
`USB Mode: USB-OTG (TinyUSB)` for HID work, the CDC port won't come back at all
until you flash something that isn't a HID sketch.
