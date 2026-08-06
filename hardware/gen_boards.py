"""
Generate the OpenMote boards.txt from the esp32 core's own esp32s3 entry.

Rules:
  - Every esp32s3 property and EVERY menu option is carried over verbatim.
  - Only the board identity + tool references are rewritten.
  - Defaults are set by reordering each menu so the OpenMote-correct option is
    listed first (Arduino treats the first option in a menu as the default),
    and by setting the matching base build.* property as a belt-and-braces.
  - build.extra_flags is NEVER emitted: in platform.txt it is a composite that
    carries -DARDUINO_USB_MODE / -DARDUINO_USB_CDC_ON_BOOT / -DCORE_DEBUG_LEVEL.
"""
import re, sys, collections

CORE = sys.argv[1]
OUT = sys.argv[2]
SRC = "esp32s3"
DST = "openmote_gen3"

# option that must be FIRST (= the default) for each menu
DEFAULTS = {
    "PSRAM": "opi",
    "FlashSize": "16M",
    "FlashMode": "qio",
    "CDCOnBoot": "cdc",
    "USBMode": "hwcdc",
    "PartitionScheme": "app3M_fat9M_16MB",
    "UploadSpeed": "921600",
    "CPUFreq": "240",
    "DebugLevel": "none",
    "EraseFlash": "none",
}

# base properties we override outright
OVERRIDE = {
    "name": "OpenMote Gen 3.3",
    "build.core": "esp32:esp32",
    "build.variant": DST,
    "build.board": "OPENMOTE_GEN3",
    "bootloader.tool": "esp32:esptool_py",
    "bootloader.tool.default": "esp32:esptool_py",
    "upload.tool": "esp32:esptool_py",
    "upload.tool.default": "esp32:esptool_py",
    "upload.tool.network": "esp32:esp_ota",
    "upload.maximum_size": "3145728",
    # OpenMote hardware defaults, mirroring the pre-selected menu options
    "build.flash_size": "16MB",
    "build.psram_type": "opi",
    "build.cdc_on_boot": "1",
    "build.partitions": "app3M_fat9M_16MB",
    "build.defines": "-DBOARD_HAS_PSRAM",
}

HEADER = """# ---------------------------------------------------------------------------
# OpenMote Gen 3.3 - Arduino board definition
#
# GENERATED from the esp32 core's own esp32s3 entry (3.3.7). Every property and
# every menu option the Dev Module offers is carried over unchanged, so nothing
# is hidden and nothing is unreachable.
#
# The only difference is which option each menu STARTS on. Arduino treats the
# first option listed in a menu as its default, so the menus below are ordered
# to pre-select the settings this board needs:
#
#   PSRAM            OPI PSRAM                          (N16R8 has Octal PSRAM)
#   Flash Size       16MB (128Mb)
#   Flash Mode       QIO 80MHz                          (NOT OPI - see below)
#   USB CDC On Boot  Enabled                            (UART0 is the D-pad)
#   USB Mode         Hardware CDC and JTAG
#   Partition Scheme 16M Flash (3MB APP/9.9MB FATFS)
#
# All of them remain changeable in Tools, exactly as with the Dev Module.
#
# Flash Mode + PSRAM combine into memory_type = {boot}_{psram_type}. The N16R8
# is 16 MB QUAD flash plus 8 MB OCTAL PSRAM, so qio + opi = qio_opi is correct.
# Choosing "OPI 80MHz" gives opi_opi and the board will not boot.
#
# build.extra_flags is deliberately absent. In the esp32 platform.txt it is a
# composite carrying -DARDUINO_USB_MODE, -DARDUINO_USB_CDC_ON_BOOT and
# -DCORE_DEBUG_LEVEL, assembled from build.extra_flags.{build.mcu}. Setting it
# from boards.txt silently drops all of those: the sketch still compiles, but
# Serial falls back to UART0 (GPIO43/44 = D-pad Up/Down) and USB serial goes
# dead. Board-specific defines go in build.defines instead.
#
# Regenerate with scratchpad/gen_boards.py against a new core's boards.txt.
# ---------------------------------------------------------------------------

"""

lines = open(CORE, encoding="utf-8").read().splitlines()

menu_decls = [l for l in lines if l.startswith("menu.")]
board = [l for l in lines if l.startswith(SRC + ".")]

base, menus = [], collections.OrderedDict()
for l in board:
    body = l[len(SRC) + 1:]
    m = re.match(r"menu\.([^.]+)\.([^.=]+)", body)
    if m:
        menus.setdefault(m.group(1), collections.OrderedDict()).setdefault(m.group(2), []).append(body)
    else:
        base.append(body)

# reorder each menu so the chosen option comes first
for menu, want in DEFAULTS.items():
    if menu in menus and want in menus[menu]:
        menus[menu].move_to_end(want, last=False)

seen = set()
out = []
for body in base:
    key = body.split("=", 1)[0]
    if key == "build.extra_flags":       # never override the composite
        continue
    if key in OVERRIDE:
        if key in seen:
            continue
        seen.add(key)
        out.append(f"{DST}.{key}={OVERRIDE[key]}")
    else:
        out.append(f"{DST}.{body}")
for key, val in OVERRIDE.items():        # add any we didn't see in the source
    if key not in seen:
        out.append(f"{DST}.{key}={val}")

used = set(menus)
with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write(HEADER)
    for d in menu_decls:
        if d.split("=")[0].split(".")[1] in used:
            f.write(d + "\n")
    f.write("\n")
    for l in out:
        f.write(l + "\n")
    for menu, opts in menus.items():
        f.write(f"\n# -- {menu} " + "-" * (60 - len(menu)) + "\n")
        for opt, ls in opts.items():
            for l in ls:
                f.write(f"{DST}.{l}\n")

print(f"wrote {OUT}: {len(out)} base properties, {len(menus)} menus")
for menu, opts in menus.items():
    print(f"  {menu:18} default={list(opts)[0]}   ({len(opts)} options)")
