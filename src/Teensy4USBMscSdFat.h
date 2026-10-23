#pragma once
// An SdFat card as the Mass Storage medium, raw sectors only (the card is initialized, no filesystem is mounted).
// Header-only so that SdFat is pulled in only by projects that use it.
//
//   SdFs sd;
//   Teensy4USBMscSdFat sdCard;
//   if (sd.cardBegin(SdioConfig(FIFO_SDIO))) sdCard.setCard(sd.card());   // no card: the host sees "no medium"
//   Teensy4USBMsc::setBlockDevice(&sdCard);
#include <SdFat.h>

#include "Teensy4USBMsc.h"

#if CFG_TUD_MSC
class Teensy4USBMscSdFat : public Teensy4USBBlockDevice {
public:
    explicit Teensy4USBMscSdFat(SdCard* card = nullptr) : card_(card) {}
    void setCard(SdCard* card) { card_ = card; }

    bool ready() override { return card_ != nullptr && card_->sectorCount() > 0; }
    uint32_t sectorCount() override { return card_ ? card_->sectorCount() : 0; }
    bool readSectors(uint32_t lba, uint8_t* buffer, uint32_t count) override {
        return card_ && card_->readSectors(lba, buffer, count);
    }
    bool writeSectors(uint32_t lba, const uint8_t* buffer, uint32_t count) override {
        return card_ && card_->writeSectors(lba, buffer, count);
    }
    bool sync() override { return card_ ? card_->syncDevice() : true; }

private:
    SdCard* card_;
};
#endif
