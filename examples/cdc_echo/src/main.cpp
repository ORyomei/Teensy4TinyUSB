// Teensy4TinyUSB example: CDC serial echo.
//
// The Teensy shows up on the PC as a USB serial port named "Teensy 4 CDC echo". Open it in any terminal at any
// baud rate; whatever you type is sent back. The LED blinks while nobody has the port open and stays on while a
// program does.
#include <Arduino.h>
#include <Teensy4TinyUSB.h>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // Start USB. VID / PID and the names are what the PC sees.
    // 0x1209:0x0001 is a pid.codes test PID for prototypes; use your own VID / PID for anything you distribute.
    Teensy4USB::begin({
        .vid = 0x1209,
        .pid = 0x0001,
        .manufacturer = "Example",
        .product = "Teensy 4 CDC echo",
    });
}

void loop() {
    Teensy4USB::task();  // runs the USB stack; call it every pass through loop()

    // SerialTinyUSB is the CDC port as an Arduino Stream (the core's Serial does nothing under USB_DISABLED)
    while (SerialTinyUSB.available() > 0) {
        const int c = SerialTinyUSB.read();
        SerialTinyUSB.write((uint8_t)c);
    }

    if (SerialTinyUSB)  // true while a program on the PC has the port open
        digitalWrite(LED_BUILTIN, HIGH);
    else
        digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
}
