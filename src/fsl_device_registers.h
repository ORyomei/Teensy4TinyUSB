#pragma once
// Stand-in for the NXP SDK header that TinyUSB's i.MX RT port includes (tusb_mcu.h, ci_hs_imxrt.h). The Teensy
// core's imxrt.h has everything the port uses; only the names differ.
#include <imxrt.h>
#include <stdint.h>

#define USB1_BASE IMXRT_USB1_ADDRESS
#define USB2_BASE IMXRT_USB2_ADDRESS
#define USB_OTG1_IRQn IRQ_USB1
#define USB_OTG2_IRQn IRQ_USB2
#define FSL_FEATURE_SOC_USBHS_COUNT 2
#define __CORTEX_M 7

typedef int IRQn_Type;
#define NVIC_EnableIRQ(n) NVIC_ENABLE_IRQ(n)
#define NVIC_DisableIRQ(n) NVIC_DISABLE_IRQ(n)

// Used only with CFG_TUD_MEM_DCACHE_ENABLE 1 (buffers outside DTCM)
static inline void SCB_CleanDCache_by_Addr(uint32_t* addr, int32_t size) {
    arm_dcache_flush(addr, (uint32_t)size);
}
static inline void SCB_InvalidateDCache_by_Addr(void* addr, int32_t size) {
    arm_dcache_delete(addr, (uint32_t)size);
}
static inline void SCB_CleanInvalidateDCache_by_Addr(uint32_t* addr, int32_t size) {
    arm_dcache_flush_delete(addr, (uint32_t)size);
}
