# OpenMote board definition for the Arduino IDE

Adds **OpenMote Gen 3.3** to the Arduino IDE's board list, with every setting
already correct. Without it you select *ESP32S3 Dev Module* and set eight menu
items by hand — and two of those, if you get them wrong, produce a board that
does not boot or a `ps_malloc()` that silently returns null.

This is not a full platform. It reuses the core, compiler and upload tools from
**esp32 by Espressif Systems**, and supplies only this board's defaults and pin
variant. Install the esp32 core first.

---

## Install

1. Install **esp32 by Espressif Systems** through Boards Manager (3.x).
2. Copy the `openmote` folder from here into your sketchbook's `hardware`
   folder, creating it if it does not exist:

   - Windows — `Documents\Arduino\hardware\openmote`
   - macOS — `~/Documents/Arduino/hardware/openmote`
   - Linux — `~/Arduino/hardware/openmote`

3. Restart the Arduino IDE.
4. **Tools → Board → OpenMote → OpenMote Gen 3.3**

The depth matters. When it is right you have:

```
Documents/Arduino/hardware/openmote/esp32/boards.txt
```

not `hardware/boards.txt` and not `hardware/openmote/boards.txt`. If the board
does not appear in the menu, this is almost always why.

## What it pre-selects

**Every menu the ESP32S3 Dev Module offers is still there and still
changeable** — all seventeen of them. The board definition only changes which
option each one *starts* on:

| Menu | Pre-selected |
|---|---|
| Flash Size | 16MB (128Mb) |
| Flash Mode | QIO 80MHz — `memory_type` resolves to `qio_opi` |
| PSRAM | OPI PSRAM (Octal) |
| USB CDC On Boot | Enabled |
| USB Mode | Hardware CDC and JTAG |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| CPU Frequency | 240 MHz |

Nothing is hidden or removed. If a default turns out to be wrong for what you
are doing, change it in Tools exactly as you would on the Dev Module.

`boards.txt` is **generated** from the core's own `esp32s3` entry by
[`gen_boards.py`](gen_boards.py), so no property can be lost in transcription.
Regenerate it against a newer core with:

```
python hardware/gen_boards.py \
  <Arduino15>/packages/esp32/hardware/esp32/<version>/boards.txt \
  hardware/openmote/esp32/boards.txt
```

The flash/PSRAM pair is the one that matters most. The N16R8 module is 16 MB
**quad** flash plus 8 MB **octal** PSRAM, so the only correct combination is
`qio` + `opi`. In the generic Dev Module recipe it is easy to read "OPI PSRAM"
and then also pick "OPI 80MHz" for the flash mode, which yields `opi_opi` and a
board that will not boot. Here that combination is not offered at all.

## The one menu you will need to change

**USB Mode** — switch to *USB-OTG (TinyUSB)* for HID sketches such as the
AirMouse example, which will not compile otherwise. Set it back to
*Hardware CDC and JTAG* for everything else.

## Never set build.extra_flags in boards.txt

Worth stating plainly, because it cost real debugging time. In the esp32
`platform.txt`, `build.extra_flags` is a **composite**:

```
build.extra_flags=-DARDUINO_HOST_OS="..." -DESP32=ESP32 \
  -DCORE_DEBUG_LEVEL={build.code_debug} {build.defines} \
  {build.extra_flags.{build.mcu}} ...
```

and `build.extra_flags.esp32s3` is what supplies `-DARDUINO_USB_MODE` and
`-DARDUINO_USB_CDC_ON_BOOT`. Overriding `build.extra_flags` from `boards.txt`
replaces the whole composite and silently drops those defines.

The failure mode is nasty: **the sketch still compiles**. But with
`ARDUINO_USB_CDC_ON_BOOT` gone, `Serial` falls back to UART0 — which on this
board is GPIO43/44, the D-pad — so nothing ever reaches the USB port and the
Serial Monitor stays blank forever.

Board-specific defines belong in **`build.defines`**, which the composite
includes. The core's own `esp32s3` entry sets `build.extra_flags` nowhere.

## Verified

Built against **esp32 core 3.3.7** with `arduino-cli`. All ten library examples
compile against `openmote:esp32:openmote_gen3`, including the AirMouse example
under `USBMode=default`.

## Maintenance

`platform.txt` here carries **four recipes copied from the esp32 core**, and
that is the part that can rot.

The reason: when a board borrows a core from another platform, the Arduino build
system still expands `{runtime.platform.path}` to *this* folder rather than the
esp32 one. Every esp32 recipe that uses it to find a helper file — the partition
CSVs, `gen_esp32part`, `boot_app0.bin` — therefore looks inside this package and
fails. `{build.core.path}` does point into the referenced platform, so each
override is the upstream recipe with `{runtime.platform.path}` replaced by
`{build.core.path}/../..` and nothing else changed.

The affected recipes are:

- `tools.gen_esp32part.cmd` (and `.windows`)
- `recipe.hooks.prebuild.3.pattern` (and `.windows`)
- `recipe.hooks.objcopy.postobjcopy.3.pattern_args`
- `recipe.hooks.savehex.postsavehex.4.pattern` (and `.windows`)
- `tools.esptool_py.upload.pattern_args`

**When bumping the supported esp32 core version**, diff those lines against the
new core's `platform.txt` and re-apply the substitution. If upstream changes a
flag and the copy here does not follow, builds keep succeeding while quietly
using stale options — which is worse than breaking.

## A note on the port name

The IDE will still show the port as **"ESP32 Family Device"**. That is not
something a board definition can fix: the ESP32-S3's native USB reports
`303A:1001`, which every ESP32-S3 board on the market also reports, so the IDE
genuinely cannot tell them apart. Changing it would mean running USB in
TinyUSB mode with a PID allocated from Espressif, at the cost of the hardware
USB-JTAG path.
