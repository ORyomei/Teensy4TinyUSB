#pragma once
#include <stdint.h>

// Strings: ASCII, up to 31 characters, must outlive begin() (literals are fine). vid and pid are required;
// 0x1209:0x0001 .. 0x0010 are pid.codes test PIDs for prototypes only.
struct Teensy4USBConfig {
    uint16_t vid = 0;
    uint16_t pid = 0;
    uint16_t bcdDevice = 0x0100;
    const char* manufacturer = "Teensy4TinyUSB";
    const char* product = "Teensy 4";
    const char* serial = nullptr;  // nullptr: the chip's unique number, same as the Teensy core reports
    const char* cdcName = "Serial";
    const char* mscName = "Mass Storage";
    uint16_t maxPowerMa = 100;
};

// One USB1, one device: everything is static.
//   setup(): Teensy4USB::begin({.vid = ..., .pid = ...});
//   loop():  Teensy4USB::task();   // class callbacks (MSC reads and writes, CDC) run from here
class Teensy4USB {
public:
    static bool begin(const Teensy4USBConfig& config);  // false: vid or pid is 0, already running, or TinyUSB failed
    static void task();

    static bool mounted();    // configured by the host
    static bool connected();  // addressed by the host
    static bool suspended();
    // USB1's frame counter. It advances while a host sends SOF and stops when the cable is pulled (or the bus is
    // suspended), which is how to tell "host gone" from "host idle".
    static uint32_t frameIndex();

    static const Teensy4USBConfig& config();
};
