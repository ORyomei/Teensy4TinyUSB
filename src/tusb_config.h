#pragma once
// TinyUSB settings for Teensy 4.x (i.MX RT1062, USB1 as a High Speed device, no RTOS).
// Everything here can be overridden from build_flags, e.g. -DCFG_TUD_MSC=0.

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_MIMXRT1XXX
#endif
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#define CFG_TUD_ENABLED 1
#ifndef CFG_TUD_MAX_SPEED
#define CFG_TUD_MAX_SPEED OPT_MODE_HIGH_SPEED
#endif

// TinyUSB's buffers go in the default data section, which is DTCM on Teensy 4 and not cached, so the controller's
// DMA needs no cache maintenance. Moving them to DMAMEM needs CFG_TUD_MEM_DCACHE_ENABLE 1.
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(64)))
#endif
#ifndef CFG_TUD_MEM_DCACHE_ENABLE
#define CFG_TUD_MEM_DCACHE_ENABLE 0
#endif
#ifndef CFG_TUH_MEM_DCACHE_ENABLE
#define CFG_TUH_MEM_DCACHE_ENABLE 0  // no host, but the i.MX RT default of 1 pulls in cache code
#endif

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

// The default descriptors (Teensy4USB.cpp) cover CDC and MSC. Other classes need your own tud_descriptor_*_cb.
#ifndef CFG_TUD_CDC
#define CFG_TUD_CDC 1
#endif
#ifndef CFG_TUD_MSC
#define CFG_TUD_MSC 1
#endif
#ifndef CFG_TUD_HID
#define CFG_TUD_HID 0
#endif
#ifndef CFG_TUD_MIDI
#define CFG_TUD_MIDI 0
#endif
#ifndef CFG_TUD_VENDOR
#define CFG_TUD_VENDOR 0
#endif

#ifndef CFG_TUD_CDC_RX_BUFSIZE
#define CFG_TUD_CDC_RX_BUFSIZE 512
#endif
#ifndef CFG_TUD_CDC_TX_BUFSIZE
#define CFG_TUD_CDC_TX_BUFSIZE 512
#endif
#ifndef CFG_TUD_CDC_EP_BUFSIZE
#define CFG_TUD_CDC_EP_BUFSIZE 512
#endif

// Bytes per READ(10) / WRITE(10) handed to the block device. Bigger is faster (fewer USB / medium round trips).
#ifndef CFG_TUD_MSC_EP_BUFSIZE
#define CFG_TUD_MSC_EP_BUFSIZE 16384
#endif
