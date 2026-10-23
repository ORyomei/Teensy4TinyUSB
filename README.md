# Teensy4TinyUSB

[TinyUSB](https://github.com/hathach/tinyusb) device stack for **Teensy 4.0 / 4.1 / MicroMod** (i.MX RT1062) on the
Arduino framework, built with PlatformIO. It gives you what the Teensy core does not have:

- **USB Mass Storage**: the SD card (or any block device) shows up on the PC as a removable drive. The host mounts
  the filesystem directly; nothing is mounted on the Teensy. About 15 MB/s on a Teensy 4.1's SDIO slot.
- **CDC serial** alongside it, as an Arduino `Stream` (`SerialTinyUSB`).
- The rest of TinyUSB's device classes (HID, MIDI, vendor, ...) if you bring your own descriptors.

The Teensy core's USB stack is switched off (`-DUSB_DISABLED`) and TinyUSB drives USB1 instead. Everything else in
the core keeps working.

## Requirements

- PlatformIO, `platform = teensy`, `framework = arduino`, a Teensy 4.x board.
- `-DUSB_DISABLED` in `build_flags`. The library checks this and stops the build with a message if it is missing.
- Not usable from the Arduino IDE: it hides the "No USB" type for Teensy 4.x, and the core needs a compiler flag the
  IDE cannot add (see "How it works").

## Quick start

`platformio.ini`:

```ini
[env:teensy41]
platform = teensy
board = teensy41
framework = arduino
lib_deps = Teensy4TinyUSB          ; or the git URL, or lib_extra_dirs pointing at a checkout
build_flags = -DUSB_DISABLED
```

The SD card as a USB drive, with a serial port next to it (`examples/sd_mass_storage`):

```cpp
#include <SdFat.h>
#include <Teensy4TinyUSB.h>
#include <Teensy4USBMscSdFat.h>

SdFs sd;
Teensy4USBMscSdFat sdCard;

void setup() {
    if (sd.cardBegin(SdioConfig(FIFO_SDIO)))   // card only, no filesystem on the Teensy
        sdCard.setCard(sd.card());
    Teensy4USBMsc::setBlockDevice(&sdCard);
    Teensy4USB::begin({.vid = 0x1209, .pid = 0x0001, .manufacturer = "Me", .product = "SD Reader"});
}

void loop() {
    Teensy4USB::task();                        // run the stack; MSC reads/writes happen in here
    if (Teensy4USBMsc::ejected()) { /* the PC ejected the drive: finish up */ }
    SerialTinyUSB.println("hello");
}
```

`0x1209:0x0001` is a [pid.codes](https://pid.codes) test PID for prototypes. Use your own VID/PID for anything you
distribute.

## API

### `Teensy4USB` (src/Teensy4USB.h)

| | |
|---|---|
| `begin(const Teensy4USBConfig&)` | Powers the PHY, hooks the USB1 interrupt, starts TinyUSB. `false` if vid/pid is 0 or already started. |
| `task()` | Call from `loop()` as often as possible. Class callbacks run from here. |
| `mounted()` / `connected()` / `suspended()` | Bus state. |
| `frameIndex()` | USB1's frame counter. It stops advancing when the cable is pulled; poll it to detect "host gone". |

`Teensy4USBConfig`: `vid`, `pid` (required), `bcdDevice`, `manufacturer`, `product`, `serial` (default: the chip's
unique number, same as the Teensy core), `cdcName`, `mscName`, `maxPowerMa`.

### `Teensy4USBSerial` / `SerialTinyUSB` (src/Teensy4USBSerial.h)

An Arduino `Stream` over CDC ACM: `available()`, `read()`, `peek()`, `write()`, `print*()`, `flush()`,
`availableForWrite()`, `operator bool()` (a program has the port open), `dtr()`, `rts()`, `baud()`.
Writes are buffered and sent on the next `task()`. When no program has the port open, writes are dropped, like the
Teensy core does. Do not write from TinyUSB callbacks.

### `Teensy4USBMsc` (src/Teensy4USBMsc.h)

One LUN backed by a `Teensy4USBBlockDevice`: implement `ready()`, `sectorCount()`, `readSectors()`,
`writeSectors()`, optionally `sectorSize()` (512), `sync()`, `writable()`.

- `setBlockDevice(dev)` before `begin()` so the first inquiry already sees the medium. `nullptr` = no medium.
- `setInquiry(vendor, product, revision)`: the SCSI INQUIRY strings (8 / 16 / 4 characters).
- `ejected()`: the host ejected the drive ("Eject" / "Safely remove"). `sync()` was called and the medium reads as
  absent until `clearEjected()`. A good moment to reboot or re-mount the card on the Teensy side.

`Teensy4USBMscSdFat` (src/Teensy4USBMscSdFat.h, header-only) wraps an SdFat card (`sd.card()` after `cardBegin()`),
raw sectors only.

### Configuration

`src/tusb_config.h` holds the TinyUSB settings; every value is `#ifndef`-guarded so `build_flags` can override it:

| flag | default | |
|---|---|---|
| `CFG_TUD_CDC` | 1 | CDC serial on/off |
| `CFG_TUD_MSC` | 1 | Mass Storage on/off |
| `CFG_TUD_MSC_EP_BUFSIZE` | 16384 | bytes per READ/WRITE handed to the block device; larger is faster, costs RAM |
| `CFG_TUD_CDC_RX_BUFSIZE` / `TX_BUFSIZE` | 512 | |
| `CFG_TUD_MAX_SPEED` | High Speed | |

The default descriptors (CDC and/or MSC, IAD composite when CDC is present, Full and High Speed configurations,
device qualifier) are weak `tud_descriptor_*_cb` functions in `src/Teensy4USB.cpp`. Define your own to replace them,
for example to add HID.

## How it works

- `Teensy4USB::begin()` repeats the Teensy core's PHY and clock setup from `usb.c`, then hands USB1 to TinyUSB's
  ChipIdea High Speed driver (`portable/chipidea/ci_hs`, the same controller as the NXP SDK targets). The USB
  interrupt is attached with `attachInterruptVector` at the core's priority.
- TinyUSB's i.MX RT port includes `fsl_device_registers.h` from the NXP SDK. `src/fsl_device_registers.h` is a
  small stand-in on top of the Teensy core's `imxrt.h`: base addresses, IRQ numbers, NVIC and the cache calls.
- TinyUSB's buffers stay in DTCM (not cached), so no cache maintenance is needed (`CFG_TUD_MEM_DCACHE_ENABLE 0`).
- Teensy core 1.59 and later do not compile with `USB_DISABLED` on their own
  ([PaulStoffregen/cores PR #735](https://github.com/PaulStoffregen/cores/pull/735)): `yield.cpp` calls
  `serialEvent()` without a declaration and `startup.c` references `usb_isr` which `usb.c` then never defines.
  `extra_script.py` force-includes `src/teensy4_no_usb_shim.h` (the declaration) into every translation unit, and
  `Teensy4USB.cpp` defines a weak empty `usb_isr`. Both become no-ops once the core is fixed.
- `src/tinyusb/` is TinyUSB 0.18.0 reduced to the device stack, class drivers and the ChipIdea port
  (`tools/update_tinyusb.sh <tag>` re-vendors it). It is bundled rather than pulled as a dependency because TinyUSB
  has to see this library's `tusb_config.h`.

## Status

Tested on a Teensy 4.1 (core 1.162 / PlatformIO) with Linux, Windows and macOS hosts: enumeration as CDC + MSC with
the stock drivers, mounting an exFAT SD card, reads at 15 MB/s, writes, eject, and CDC at the same time.
Device mode on USB1 only; for host mode use the core's USBHost_t36.

## License

MIT (see LICENSE). TinyUSB is MIT, Copyright (c) 2018 Ha Thach.
