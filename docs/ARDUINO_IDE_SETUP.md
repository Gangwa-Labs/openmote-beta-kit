# Programming the OpenMote from the Arduino IDE

There is **no custom board package to install**. The OpenMote is an
ESP32-S3-WROOM-1-N16R8 module, and the stock **ESP32S3 Dev Module** entry drives
it perfectly once a handful of settings are right. Reasons for doing it this way
are at the [bottom of this page](#why-not-a-custom-board-package).

Total setup time: about five minutes.

---

## 1. Install the ESP32 board support

1. **Arduino IDE → File → Preferences**
2. In *Additional boards manager URLs*, add:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **Tools → Board → Boards Manager**, search for `esp32`, and install
   **esp32 by Espressif Systems**. Version **3.0.0 or newer** is recommended;
   the library also compiles against the 2.x series.

## 2. Install the OpenMote library

The library lives in its own repository:
**[github.com/Gangwa-Labs/OpenMote](https://github.com/Gangwa-Labs/OpenMote)**.
It is not part of this kit — this repo carries the documentation, demos and
board definitions that go around it.

Either:

- **Code → Download ZIP** on that repo, then **Sketch → Include Library →
  Add .ZIP Library…** and select the downloaded file; or
- clone it into your Arduino `libraries/` directory:
  - Windows — `Documents\Arduino\libraries\OpenMote`
  - macOS — `~/Documents/Arduino/libraries/OpenMote`
  - Linux — `~/Arduino/libraries/OpenMote`

Restart the IDE. The examples appear under
**File → Examples → OpenMote**.

## 3. Select the board and settings

**Tools → Board → esp32 → ESP32S3 Dev Module**, then match this table.
The four marked ⭐ are the ones that will break your board if they are wrong.

| Setting | Value | Why |
|---------|-------|-----|
| Board | **ESP32S3 Dev Module** | |
| ⭐ **PSRAM** | **OPI PSRAM** | The N16R8 has *Octal* PSRAM. Set to "Disabled" or "QSPI" it will not be detected; `ps_malloc()` returns null. |
| ⭐ **Flash Size** | **16MB (128Mb)** | Anything smaller and large sketches or filesystems overrun the end of flash. |
| ⭐ **USB CDC On Boot** | **Enabled** | `Serial` output goes over the USB-C cable. Without this you get no serial monitor at all, because the UART pins are the D-pad. |
| ⭐ **USB Mode** | **Hardware CDC and JTAG** | Native USB. Also what makes the board reset itself for uploads. |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** | Plenty of room for a sketch plus a filesystem. |
| ⭐ **Flash Mode** | **QIO 80MHz** | Not cosmetic — see the warning below. Do **not** pick "OPI 80MHz". |
| CPU Frequency | **240MHz (WiFi)** | |
| Upload Speed | **921600** | Drop to 460800 or 115200 if uploads fail partway. |
| Arduino Runs On | Core 1 | |
| Events Run On | Core 1 | |
| Core Debug Level | None | Raise to "Info" when chasing a Wi-Fi or USB problem. |
| Erase All Flash Before Sketch Upload | Disabled | Enable once if the board is behaving strangely after a bad flash. |
| JTAG Adapter | Integrated USB JTAG | Leave Disabled unless you are actually debugging. |
| USB Firmware MSC On Boot | Disabled | |
| USB DFU On Boot | Disabled | |

> ### ⚠ Do not select "OPI 80MHz" as the Flash Mode
>
> It looks like the matching choice once you have set PSRAM to OPI. It is not.
>
> The Arduino core derives its memory configuration from **both** menus:
> `memory_type = {Flash Mode} _ {PSRAM type}`. The OpenMote's N16R8 module is
> 16 MB **quad** flash plus 8 MB **octal** PSRAM, so the correct combination is
> `qio` + `opi` = **`qio_opi`** — which is what "QIO 80MHz" + "OPI PSRAM" gives.
>
> Selecting "OPI 80MHz" produces `opi_opi`, telling the bootloader the *flash* is
> octal as well. The board will not boot. The "OPI" in the Flash Mode menu refers
> to the flash chip; the "OPI" you want is the one in the PSRAM menu.

> **Using an OpenMote for USB HID** (keyboard, mouse, gamepad — see the air mouse
> firmware)? Change **USB Mode** to **USB-OTG (TinyUSB)**. You lose the serial
> monitor over USB while that is selected.

## 4. Upload

1. Plug the board into USB-C.
2. **Tools → Port** — pick the port that appears. On Windows it is a COM port; on
   macOS `/dev/cu.usbmodem…`; on Linux `/dev/ttyACM0`.
3. Open **File → Examples → OpenMote → 01_HelloOpenMote** and hit Upload.
4. Open the Serial Monitor at **115200 baud**.

If the upload hangs or the port never appears, see
[FLASHING.md](FLASHING.md) — you can put the board into download mode by hand.

---

## Quick reference: the settings that matter

```
Board:              ESP32S3 Dev Module
PSRAM:              OPI PSRAM                      <-- must be OPI
Flash Size:         16MB (128Mb)                   <-- must be 16MB
USB CDC On Boot:    Enabled                        <-- or no serial output
USB Mode:           Hardware CDC and JTAG
Partition Scheme:   16M Flash (3MB APP/9.9MB FATFS)
Flash Mode:         QIO 80MHz
Upload Speed:       921600
```

---

## Why not a custom board package?

A proper `boards.txt` package — the kind where "OpenMote Gen 3.3" shows up in the
board list — is genuinely nice, but it costs more than it gives back right now:

- It has to be **hosted**. Boards Manager installs from a
  `package_openmote_index.json` served over HTTPS, pointing at a versioned,
  SHA-256-checksummed release archive. That is a release process, not a file.
- It has to be **re-tested on every esp32 core release**, because a board package
  inherits the core's menu structure and platform recipes.
- The alternative — telling people to hand-copy a `boards.txt` fragment and a
  variant folder into their installed core — breaks silently every time the IDE
  updates the core, and produces confusing bug reports.

The settings table above gets a beta board building and flashing in five minutes
with nothing to install and nothing to maintain. The pin names live in
`OpenMotePins.h`, which is where they are useful anyway.

**If you do want a real board entry later**, the pieces are:

1. `hardware/openmote/esp32/boards.txt` — an entry cloned from the core's
   `esp32s3` section with the defaults baked in (`build.flash_size=16MB`,
   `build.memory_type=qio_opi`, `build.psram_type=opi`, `build.extra_flags`
   carrying `-DARDUINO_USB_CDC_ON_BOOT=1`).
2. `hardware/openmote/esp32/variants/openmote_gen3/pins_arduino.h` — the same
   numbers as `OpenMotePins.h`, plus the `LED_BUILTIN`, `SDA` and `SCL`
   definitions the Arduino API expects.
3. `platform.txt` inherited from the esp32 core via `core=esp32:esp32`.
4. A `package_openmote_index.json` and a tagged GitHub release for Boards Manager.

The [PlatformIO board definition](../boards/openmote_gen3.json) in this repo
already encodes step 1's settings, so it is a good starting point for the
`boards.txt` values.
