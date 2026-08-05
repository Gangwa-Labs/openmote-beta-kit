# USB Microphone (UAC)

Makes the OpenMote enumerate as a **USB audio capture device**. Plug it in and it
appears as *Microphone (usb uac)* — no drivers, no pairing, nothing to install.

The PDM MEMS microphone feeds a ring buffer, and the `espressif/usb_device_uac`
component owns the entire USB side, calling `uac_input_cb()` to fill each frame.

Mono, 16-bit, 16 kHz.

---

## ⚠ This one is ESP-IDF, not Arduino — and not PlatformIO

Every other demo in this kit is an Arduino sketch you can build with PlatformIO.
This one is different in two ways, both of which matter before you start:

**It is pure ESP-IDF C.** There is no Arduino framework here, so it cannot use
the OpenMote library at all. Pin numbers come from its own
`include/pin_definitions.h`.

**It must be built with `idf.py`, not PlatformIO.** PlatformIO's ESP-IDF builder
trips over this project's managed components and fails with:

```
*** Two environments with different actions were specified for the same target:
    managed_components/espressif__usb_device_uac/tusb/usb_descriptors.c.o
```

The `usb_device_uac` component bundles its own `usb_descriptors.c`, and the
PlatformIO builder registers a compile action for it twice — once through the
component and once through TinyUSB — with different `-D` flags each time. It is
a builder limitation, not a problem with this firmware.

There is deliberately **no `platformio.ini` here**, so nobody wastes an hour on
a build that cannot succeed. Use the ESP-IDF toolchain below, which is how this
firmware was built and confirmed working in the first place.

## Building

Requires **ESP-IDF 5.5** or newer.
([Espressif's getting-started guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html))

```bash
# once per terminal, from your ESP-IDF checkout
. $HOME/esp/esp-idf/export.sh          # Linux / macOS
%USERPROFILE%\esp\esp-idf\export.bat   # Windows

cd demos/usb_microphone
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor           # substitute your port
```

The first build downloads `espressif/usb_device_uac` and `tinyusb` through the
ESP-IDF Component Manager into `managed_components/`. `dependencies.lock` pins
the versions; neither that directory nor the generated `sdkconfig` is committed.

## Checking it worked

Windows shows it under *Sound settings → Input* as **Microphone (usb uac)**,
VID `303A` / PID `8000`. macOS and Linux list it as a standard USB audio input.

Record something and confirm you get audio rather than silence — the mic is on
the front edge of the board.

## Configuration

Audio parameters live in `sdkconfig.defaults`:

```
CONFIG_UAC_MIC_CHANNEL_NUM=1           # mono
CONFIG_UAC_SPEAKER_CHANNEL_NUM=0       # capture only, no playback interface
CONFIG_UAC_SAMPLE_RATE=16000
CONFIG_UAC_BIT_RESOLUTION_16=y
```

Change them and rebuild, or use `idf.py menuconfig`.

## Two OpenMote-specific details

**The console is on UART0, not USB.** `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` puts
logging on GPIO43/44 because the native USB port and PHY are needed for the UAC
device itself. On the OpenMote those pins are **D-pad Up and Down**, so there is
effectively no serial console — progress is shown on the LEDs instead. Pressing
Up or Down will inject noise into UART0 if you do attach a probe.

**GPIO21 is driven low at startup.** That is the MAX98357A's SD_MODE pin. On
Gen 3.3 the amp powers up in whatever state the R17 + JP1 bias network leaves
it, and an enabled amp with no I2S clocks just burns current and hisses, so
`app_main()` holds it in shutdown. This is a mic-only build.

**PSRAM is off.** The ring buffer is a few KB of internal RAM and doesn't need
it. To turn it on, add `CONFIG_SPIRAM=y` and `CONFIG_SPIRAM_MODE_OCT=y` — it
must be **OCT** on the N16R8. Either way GPIO33–37 belong to PSRAM on this
module and must not be assigned.
