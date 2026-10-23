#pragma once
// Force-included by extra_script.py. Teensy core 1.59+ with USB_DISABLED: yield.cpp calls serialEvent() but
// usb_serial.h declares it only when USB is on (PaulStoffregen/cores PR #735). Nothing defines it, so the weak
// symbol is null; yield() never calls it because only the core's usb_serial.c raises YIELD_CHECK_USB_SERIAL.
#ifdef __cplusplus
void serialEvent(void) __attribute__((weak));
#endif
