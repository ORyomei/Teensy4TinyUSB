#pragma once
#include <Stream.h>

#include "tusb_config.h"

#if CFG_TUD_CDC
// The CDC port as an Arduino Stream (the core's Serial is a dummy under USB_DISABLED). write() buffers; the data
// leaves on the next Teensy4USB::task() or flush(). While no program holds the port open (DTR low) writes are
// dropped, as the core does. Not for use inside TinyUSB callbacks: write() may run tud_task() to make room.
class Teensy4USBSerial : public Stream {
public:
    int available() override;
    int read() override;
    int peek() override;
    int availableForWrite() override;
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buffer, size_t size) override;
    void flush() override;
    using Print::write;

    operator bool();  // a program has the port open (DTR)
    bool dtr();
    bool rts();
    uint32_t baud();  // what the host set; informational, USB has no baud rate
};

extern Teensy4USBSerial SerialTinyUSB;
#endif
