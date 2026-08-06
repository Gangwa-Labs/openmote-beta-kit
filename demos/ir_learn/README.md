# IR Learn & Replay

Turns the OpenMote into a learning universal remote. Capture a code from any
household remote, bind it to a button, and from then on pressing that button
transmits it.

Built on the OpenMote library (`OpenMote` + `OpenMoteIR`), so it doubles as a
worked example of both.

---

## Using it

**1. Hold A + B together for 2 seconds.**
One rumble, then the LEDs run a chase pattern — it is listening.

**2. Point a remote at the front of the OpenMote and press a button.**
All four LEDs slow-pulse: a code has been captured and is waiting for a home.

**3. Press the OpenMote button you want it bound to.**
Two rapid flashes and two rumbles: saved.

**4. Press that button any time to transmit.**
LED 1 blips and the motor gives a short tick.

Any of the twelve buttons can be bound, **including A and B themselves**.
Binding a button that already holds a code just overwrites it.

**Getting out:** hold A + B again for 2 seconds while it is listening. Either
learn step also gives up on its own after 30 seconds, with one long buzz.

## LED vocabulary

| Pattern | Meaning |
|---|---|
| Chase, one LED at a time | listening for an IR code |
| All four pulsing slowly | code captured — now press a button to bind it |
| Two rapid flashes + two rumbles | bound and saved |
| One long buzz, LEDs off | cancelled or timed out, nothing stored |
| LED 1 blip + short tick | transmitting |
| LED 4 blip | that button has nothing bound to it |

## Mappings persist

Codes are stored in NVS, so they survive a power cycle and a re-flash of the
same firmware. A full set of twelve uses about 3.2 KB.

To wipe them, erase the flash:

```bash
esptool --chip esp32s3 --port COM5 --before no_reset erase_flash
```

## Serial is a backup, not a requirement

Everything is echoed over USB CDC at 115200 — state changes, captured codes,
and a full table of bindings on boot and after every save. Raw frames are dumped
in full when learned, so you can reconstruct a mapping by hand if you need to.

**The firmware never depends on a monitor being attached.** Every message goes
through a `logf()` helper that returns immediately when the USB CDC port is not
open:

```cpp
static void logf(const char *fmt, ...) {
    if (!Serial) return;
    ...
}
```

Without that guard, writes to an unread port stall once the driver's buffer
fills, which would drag out the main loop and make the buttons feel sticky. It
is worth copying into any OpenMote firmware meant to run standalone.

## What it can and cannot capture

NEC codes — which is most cheap remotes — are decoded and stored as 32 bits.
Anything the decoder does not recognise is stored as **raw mark/space timing**
and replayed on a 38 kHz carrier, so a lot of non-NEC remotes work anyway.

Limits worth knowing:

- Raw capture is capped at **128 durations** per button (a NEC frame is ~68).
  Longer frames are truncated, and the serial log says so.
- Replay always uses a **38 kHz** carrier. Remotes at 36 or 40 kHz usually still
  work, since receivers are broad, but range may suffer.
- Air-conditioner remotes send very long state-encoded frames and will not fit.
  For those, use **IRremoteESP8266** — the `tv_remote` demo shows the pattern.
- Transmit timing is generated in software, so heavy Wi-Fi activity can jitter
  it. This firmware doesn't use Wi-Fi, so that isn't a concern here.

## A design detail: why transmit on release

Codes are sent when a button is **released**, not pressed. Otherwise starting
the A + B combo would fire A's and B's own codes on the way in, spraying IR at
whatever happens to be in front of you every time you enter learn mode.

Binding, in contrast, happens on **press** — that is the one place a bare A or B
press is unambiguous, and the release that follows is suppressed so the freshly
bound code doesn't immediately transmit itself.

## Build

```bash
pio run -t upload
pio device monitor
```

Requires the whole repo checked out: `platformio.ini` refers to the shared board
definition in `../../boards` and the library at the repo root.

### Arduino IDE

The same firmware ships as a sketch in the library repo at
[`examples/demos/IRLearn`](https://github.com/Gangwa-Labs/OpenMote/tree/main/examples/demos/IRLearn),
so once the OpenMote library is installed it opens straight from
**File → Examples → OpenMote → demos → IRLearn**. Use the standard board
settings from [ARDUINO_IDE_SETUP.md](../../docs/ARDUINO_IDE_SETUP.md).

The custom `partitions.csv` here is a PlatformIO thing and has no Arduino
equivalent to set — any Partition Scheme with a normal NVS partition holds the
codes, including the stock **16M Flash (3MB APP/9.9MB FATFS)**. Do keep
**Erase All Flash Before Sketch Upload** disabled, though: it wipes NVS, and
with it every learned code.
