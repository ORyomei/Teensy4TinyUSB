#include "Teensy4USBSerial.h"

#if CFG_TUD_CDC
#include <Arduino.h>
#include <tusb.h>

namespace {
constexpr uint32_t kWriteTimeoutMs = 120;  // as the core: stop waiting when the host stops draining the port
}

Teensy4USBSerial SerialTinyUSB;

int Teensy4USBSerial::available() {
    return (int)tud_cdc_available();
}

int Teensy4USBSerial::read() {
    return tud_cdc_read_char();
}

int Teensy4USBSerial::peek() {
    uint8_t c;
    return tud_cdc_peek(&c) ? c : -1;
}

int Teensy4USBSerial::availableForWrite() {
    return (int)tud_cdc_write_available();
}

size_t Teensy4USBSerial::write(uint8_t c) {
    return write(&c, 1);
}

size_t Teensy4USBSerial::write(const uint8_t* buffer, size_t size) {
    if (!tud_cdc_connected())
        return size;
    size_t done = 0;
    const uint32_t t0 = millis();
    while (done < size) {
        done += tud_cdc_write(buffer + done, (uint32_t)(size - done));
        if (done < size) {
            tud_cdc_write_flush();
            tud_task();
            if (!tud_cdc_connected() || millis() - t0 > kWriteTimeoutMs)
                break;
        }
    }
    return done;
}

void Teensy4USBSerial::flush() {
    tud_cdc_write_flush();
}

Teensy4USBSerial::operator bool() {
    return tud_cdc_connected();
}

bool Teensy4USBSerial::dtr() {
    return tud_cdc_get_line_state() & 0x01;
}

bool Teensy4USBSerial::rts() {
    return tud_cdc_get_line_state() & 0x02;
}

uint32_t Teensy4USBSerial::baud() {
    cdc_line_coding_t coding;
    tud_cdc_get_line_coding(&coding);
    return coding.bit_rate;
}
#endif
