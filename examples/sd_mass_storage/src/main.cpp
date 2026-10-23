// Teensy4TinyUSB example: the SD card as a USB drive.
//
// Put a card the PC can read (FAT32, exFAT, ...) in the Teensy 4.1's SD slot. The Teensy shows up on the PC as a
// removable drive named "Teensy 4 SD Reader", plus a serial port that prints the status once a second.
// The Teensy never mounts the card itself: the PC reads and writes raw sectors through USB.
//
// Eject the drive on the PC ("Eject" / "Safely remove") and the LED goes solid. That is the moment a real
// application would reboot into its normal firmware or start using the card itself.
//
// LED: fast blink = running with a card, slow blink = no card, solid = ejected by the PC.
#include <Arduino.h>
#include <SdFat.h>
#include <Teensy4TinyUSB.h>
#include <Teensy4USBMscSdFat.h>  // adapter: SdFat card -> Mass Storage medium

SdFs sd;
Teensy4USBMscSdFat sdCard;  // the medium the PC sees; "no medium" until a card is attached below

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // 1. Initialize the card only (cardBegin, not begin): no filesystem is mounted on the Teensy.
    if (sd.cardBegin(SdioConfig(FIFO_SDIO)))
        sdCard.setCard(sd.card());

    // 2. Hand the medium to the Mass Storage class before USB starts, so the PC's first look already finds it.
    Teensy4USBMsc::setBlockDevice(&sdCard);
    Teensy4USBMsc::setInquiry("Example", "SD Card", "1.0");  // the OS shows these as the drive's vendor / model

    // 3. Start USB. 0x1209:0x0001 is a pid.codes test PID for prototypes; use your own VID / PID for anything
    //    you distribute.
    Teensy4USB::begin({
        .vid = 0x1209,
        .pid = 0x0001,
        .manufacturer = "Example",
        .product = "Teensy 4 SD Reader",
    });
}

void loop() {
    Teensy4USB::task();  // runs the USB stack; the PC's reads and writes of the card happen inside this call

    // Status over the serial port once a second (dropped while no terminal is open)
    static uint32_t lastReport = 0;
    if (millis() - lastReport >= 1000) {
        lastReport = millis();
        SerialTinyUSB.printf("card: %s (%lu sectors), USB: %s%s\n", sdCard.ready() ? "present" : "none",
                             (unsigned long)sdCard.sectorCount(), Teensy4USB::mounted() ? "mounted" : "not mounted",
                             Teensy4USBMsc::ejected() ? ", ejected by the PC" : "");
    }

    if (Teensy4USBMsc::ejected())
        digitalWrite(LED_BUILTIN, HIGH);  // the PC is done with the card
    else
        digitalWrite(LED_BUILTIN, (millis() / (sdCard.ready() ? 250 : 1000)) & 1);
}
