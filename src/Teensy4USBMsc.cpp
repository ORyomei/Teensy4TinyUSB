#include "Teensy4USBMsc.h"

#if CFG_TUD_MSC
#include <string.h>
#include <tusb.h>

namespace {
Teensy4USBBlockDevice* s_device = nullptr;
bool s_ejected = false;
uint8_t s_vendor[8], s_product[16], s_revision[4];
bool s_inquirySet = false;

void padCopy(uint8_t* dst, size_t n, const char* src) {
    const size_t len = src ? strnlen(src, n) : 0;
    memcpy(dst, src, len);
    memset(dst + len, ' ', n - len);
}

bool mediumReady() {
    return s_device != nullptr && !s_ejected && s_device->ready();
}
}  // namespace

void Teensy4USBMsc::setBlockDevice(Teensy4USBBlockDevice* device) {
    s_device = device;
    s_ejected = false;
}

Teensy4USBBlockDevice* Teensy4USBMsc::blockDevice() {
    return s_device;
}

void Teensy4USBMsc::setInquiry(const char* vendor, const char* product, const char* revision) {
    padCopy(s_vendor, sizeof(s_vendor), vendor);
    padCopy(s_product, sizeof(s_product), product);
    padCopy(s_revision, sizeof(s_revision), revision);
    s_inquirySet = true;
}

bool Teensy4USBMsc::ejected() {
    return s_ejected;
}

void Teensy4USBMsc::clearEjected() {
    s_ejected = false;
}

// ── TinyUSB MSC callbacks ─────────────────────────────────────────────────────────────────────────────────────────
extern "C" {

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void)lun;
    if (!s_inquirySet)
        Teensy4USBMsc::setInquiry("Teensy4", "USB Mass Storage", "1.0");
    memcpy(vendor_id, s_vendor, 8);
    memcpy(product_id, s_product, 16);
    memcpy(product_rev, s_revision, 4);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    if (!mediumReady()) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);  // medium not present
        return false;
    }
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    (void)lun;
    *block_count = mediumReady() ? s_device->sectorCount() : 0;
    *block_size = s_device ? s_device->sectorSize() : 512;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void)lun;
    (void)power_condition;
    if (load_eject && !start) {  // eject
        if (s_device)
            s_device->sync();
        s_ejected = true;
    }
    return true;
}

// offset is nonzero only after a callback returned part of a sector, which these never do
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    (void)lun;
    if (!mediumReady())
        return -1;
    const uint16_t sector = s_device->sectorSize();
    if (offset != 0 || bufsize < sector)
        return -1;
    const uint32_t count = bufsize / sector;
    return s_device->readSectors(lba, (uint8_t*)buffer, count) ? (int32_t)(count * sector) : -1;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    (void)lun;
    return mediumReady() && s_device->writable();
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    (void)lun;
    if (!mediumReady() || !s_device->writable())
        return -1;
    const uint16_t sector = s_device->sectorSize();
    if (offset != 0 || bufsize < sector)
        return -1;
    const uint32_t count = bufsize / sector;
    return s_device->writeSectors(lba, buffer, count) ? (int32_t)(count * sector) : -1;
}

// Commands TinyUSB does not handle itself: none are supported
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
    (void)scsi_cmd;
    (void)buffer;
    (void)bufsize;
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);  // invalid command operation code
    return -1;
}

}  // extern "C"
#endif
