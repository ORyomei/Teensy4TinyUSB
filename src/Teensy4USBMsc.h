#pragma once
#include <stdint.h>

#include "tusb_config.h"

#if CFG_TUD_MSC
// The medium behind the Mass Storage interface: fixed-size sectors, nothing more. The host mounts whatever
// filesystem is on it; nothing is mounted on the Teensy. Called from Teensy4USB::task().
class Teensy4USBBlockDevice {
public:
    virtual ~Teensy4USBBlockDevice() = default;
    virtual bool ready() = 0;  // false: the host sees "no medium"
    virtual uint32_t sectorCount() = 0;
    virtual uint16_t sectorSize() { return 512; }
    virtual bool readSectors(uint32_t lba, uint8_t* buffer, uint32_t count) = 0;
    virtual bool writeSectors(uint32_t lba, const uint8_t* buffer, uint32_t count) = 0;
    virtual bool sync() { return true; }  // called when the host ejects
    virtual bool writable() { return true; }
};

// One LUN. Set the block device before Teensy4USB::begin(); one set later shows up when the host next polls.
class Teensy4USBMsc {
public:
    static void setBlockDevice(Teensy4USBBlockDevice* device);  // nullptr: no medium
    static Teensy4USBBlockDevice* blockDevice();
    static void setInquiry(const char* vendor, const char* product, const char* revision);  // 8 / 16 / 4 characters

    // The host ejected the medium ("Eject", "Safely remove"): sync() has run and the medium reads as absent until
    // clearEjected(). The moment to finish up, reboot, or remount the card on the Teensy side.
    static bool ejected();
    static void clearEjected();
};
#endif
