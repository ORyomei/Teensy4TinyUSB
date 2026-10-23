#include "Teensy4USB.h"

#include <Arduino.h>
#include <imxrt.h>
#include <stdio.h>
#include <string.h>
#include <tusb.h>

namespace {
Teensy4USBConfig s_config;
bool s_running = false;

void usb1Isr() {
    tud_int_handler(0);
}

// ── Default descriptors. Interfaces and endpoints follow the classes compiled in (tusb_config.h); vid / pid /
// strings / power are patched in from Teensy4USBConfig. The tud_descriptor_*_cb below are weak: define your own
// to replace them.
enum { STR_LANG = 0, STR_MANUFACTURER, STR_PRODUCT, STR_SERIAL, STR_CDC, STR_MSC };

#if CFG_TUD_CDC
enum { ITF_CDC = 0, ITF_CDC_DATA, ITF_MSC };
constexpr uint8_t EP_CDC_NOTIF = 0x81, EP_CDC_OUT = 0x02, EP_CDC_IN = 0x82, EP_MSC_OUT = 0x03, EP_MSC_IN = 0x83;
// CDC's interface association needs the MISC/IAD device class
constexpr uint8_t DEV_CLASS = TUSB_CLASS_MISC, DEV_SUBCLASS = MISC_SUBCLASS_COMMON, DEV_PROTOCOL = MISC_PROTOCOL_IAD;
#else
enum { ITF_MSC = 0 };
constexpr uint8_t EP_MSC_OUT = 0x01, EP_MSC_IN = 0x81;
constexpr uint8_t DEV_CLASS = 0, DEV_SUBCLASS = 0, DEV_PROTOCOL = 0;
#endif
constexpr uint8_t ITF_TOTAL = (CFG_TUD_CDC ? 2 : 0) + (CFG_TUD_MSC ? 1 : 0);
static_assert(ITF_TOTAL > 0, "Teensy4TinyUSB: enable CFG_TUD_CDC and/or CFG_TUD_MSC, or provide your own descriptors");
constexpr uint16_t CONFIG_TOTAL_LEN =
    TUD_CONFIG_DESC_LEN + (CFG_TUD_CDC ? TUD_CDC_DESC_LEN : 0) + (CFG_TUD_MSC ? TUD_MSC_DESC_LEN : 0);
constexpr uint8_t BMAXPOWER_OFFSET = 8;

tusb_desc_device_t s_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = DEV_CLASS,
    .bDeviceSubClass = DEV_SUBCLASS,
    .bDeviceProtocol = DEV_PROTOCOL,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0,
    .idProduct = 0,
    .bcdDevice = 0x0100,
    .iManufacturer = STR_MANUFACTURER,
    .iProduct = STR_PRODUCT,
    .iSerialNumber = STR_SERIAL,
    .bNumConfigurations = 1,
};

uint8_t s_desc_cfg_fs[] = {  // Full Speed: 64-byte bulk packets
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
#if CFG_TUD_CDC
    TUD_CDC_DESCRIPTOR(ITF_CDC, STR_CDC, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 64),
#endif
#if CFG_TUD_MSC
    TUD_MSC_DESCRIPTOR(ITF_MSC, STR_MSC, EP_MSC_OUT, EP_MSC_IN, 64),
#endif
};
uint8_t s_desc_cfg_hs[] = {  // High Speed: 512
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
#if CFG_TUD_CDC
    TUD_CDC_DESCRIPTOR(ITF_CDC, STR_CDC, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 512),
#endif
#if CFG_TUD_MSC
    TUD_MSC_DESCRIPTOR(ITF_MSC, STR_MSC, EP_MSC_OUT, EP_MSC_IN, 512),
#endif
};
static_assert(sizeof(s_desc_cfg_fs) == CONFIG_TOTAL_LEN && sizeof(s_desc_cfg_hs) == CONFIG_TOTAL_LEN, "descriptor size");
uint8_t s_desc_other_speed[CONFIG_TOTAL_LEN];

const tusb_desc_device_qualifier_t s_desc_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = 0x0200,
    .bDeviceClass = DEV_CLASS,
    .bDeviceSubClass = DEV_SUBCLASS,
    .bDeviceProtocol = DEV_PROTOCOL,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 1,
    .bReserved = 0,
};

char s_serial[16];
uint16_t s_desc_str[32 + 1];

const char* stringFor(uint8_t index) {
    switch (index) {
        case STR_MANUFACTURER:
            return s_config.manufacturer;
        case STR_PRODUCT:
            return s_config.product;
        case STR_SERIAL:
            if (s_config.serial)
                return s_config.serial;
            if (s_serial[0] == 0) {  // the core's recipe (usb_desc.c): low 24 bits of OCOTP MAC0, decimal
                uint32_t num = HW_OCOTP_MAC0 & 0xFFFFFF;
                if (num < 10000000)
                    num = num * 10;
                snprintf(s_serial, sizeof(s_serial), "%lu", (unsigned long)num);
            }
            return s_serial;
        case STR_CDC:
            return s_config.cdcName;
        case STR_MSC:
            return s_config.mscName;
        default:
            return nullptr;
    }
}

void applyConfig(const Teensy4USBConfig& cfg) {
    s_desc_device.idVendor = cfg.vid;
    s_desc_device.idProduct = cfg.pid;
    s_desc_device.bcdDevice = cfg.bcdDevice;
    const uint16_t ma = cfg.maxPowerMa > 500 ? 500 : cfg.maxPowerMa;
    s_desc_cfg_fs[BMAXPOWER_OFFSET] = s_desc_cfg_hs[BMAXPOWER_OFFSET] = (uint8_t)(ma / 2);
}
}  // namespace

// startup.c references usb_isr even with USB_DISABLED, while usb.c then defines nothing (cores PR #735). Weak, so a
// core that does define it still links.
extern "C" __attribute__((weak)) void usb_isr(void) {}

bool Teensy4USB::begin(const Teensy4USBConfig& config) {
    if (s_running || config.vid == 0 || config.pid == 0)
        return false;
    s_config = config;
    applyConfig(config);

    // Same PHY bring-up as the core's usb_init() (usb.c); the 480 MHz USB PLL already runs from startup.
    // PHY and controller are reset first because the previous firmware or the bootloader may have left them running.
    PMU_REG_3P0 = PMU_REG_3P0_OUTPUT_TRG(0x0F) | PMU_REG_3P0_BO_OFFSET(6) | PMU_REG_3P0_ENABLE_LINREG;
    CCM_CCGR6 |= CCM_CCGR6_USBOH3(CCM_CCGR_ON);
    USB1_BURSTSIZE = 0x0404;

    USBPHY1_CTRL_SET = USBPHY_CTRL_SFTRST;
    USB1_USBCMD |= USB_USBCMD_RST;
    while (USB1_USBCMD & USB_USBCMD_RST) {
    }
    NVIC_CLEAR_PENDING(IRQ_USB1);
    USBPHY1_CTRL_CLR = USBPHY_CTRL_SFTRST;
    delay(25);
    USBPHY1_CTRL_CLR = USBPHY_CTRL_CLKGATE;
    USBPHY1_PWD = 0;

    attachInterruptVector(IRQ_USB1, usb1Isr);
    NVIC_SET_PRIORITY(IRQ_USB1, 112);  // the core's USB priority
    s_running = tud_init(0);
    return s_running;
}

void Teensy4USB::task() {
    tud_task();
#if CFG_TUD_CDC
    if (tud_cdc_connected())
        tud_cdc_write_flush();  // send what Teensy4USBSerial::write() buffered
#endif
}

bool Teensy4USB::mounted() {
    return tud_mounted();
}
bool Teensy4USB::connected() {
    return tud_connected();
}
bool Teensy4USB::suspended() {
    return tud_suspended();
}
uint32_t Teensy4USB::frameIndex() {
    return USB1_FRINDEX;
}
const Teensy4USBConfig& Teensy4USB::config() {
    return s_config;
}

extern "C" {

// TinyUSB without an RTOS asks the application for time
uint32_t tusb_time_millis_api(void) {
    return millis();
}
void tusb_time_delay_ms_api(uint32_t ms) {
    delay(ms);
}

__attribute__((weak)) uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&s_desc_device;
}

__attribute__((weak)) uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return tud_speed_get() == TUSB_SPEED_HIGH ? s_desc_cfg_hs : s_desc_cfg_fs;
}

__attribute__((weak)) uint8_t const* tud_descriptor_device_qualifier_cb(void) {
    return (uint8_t const*)&s_desc_qualifier;
}

__attribute__((weak)) uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index) {
    (void)index;
    memcpy(s_desc_other_speed, tud_speed_get() == TUSB_SPEED_HIGH ? s_desc_cfg_fs : s_desc_cfg_hs, CONFIG_TOTAL_LEN);
    s_desc_other_speed[1] = TUSB_DESC_OTHER_SPEED_CONFIG;
    return s_desc_other_speed;
}

__attribute__((weak)) uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t count;
    if (index == STR_LANG) {
        s_desc_str[1] = 0x0409;  // English (US)
        count = 1;
    } else {
        const char* s = stringFor(index);
        if (s == nullptr)
            return nullptr;
        count = strnlen(s, 32);
        for (size_t i = 0; i < count; i++)
            s_desc_str[1 + i] = (uint8_t)s[i];
    }
    s_desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return s_desc_str;
}

}  // extern "C"
