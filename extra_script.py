# Runs when PlatformIO's dependency finder picks up this library (library.json "extraScript").
import os

Import("env", "pio_lib_builder")
from SCons.Script import DefaultEnvironment  # pylint: disable=import-error

global_env = DefaultEnvironment()
defines = [d[0] if isinstance(d, (tuple, list)) else str(d) for d in global_env.get("CPPDEFINES", [])]

# Two stacks on USB1 never enumerate: the core's must be off.
if "USB_DISABLED" not in defines:
    print("\nTeensy4TinyUSB: the Teensy core's USB must be turned off. Add to build_flags in platformio.ini:")
    print("    -DUSB_DISABLED\n")
    global_env.Exit(1)

# Core 1.59+ does not compile with USB_DISABLED on its own (PaulStoffregen/cores PR #735): yield.cpp calls
# serialEvent() without a declaration. Force-include one into every translation unit, the core's included.
global_env.Append(CCFLAGS=["-include", os.path.join(pio_lib_builder.path, "src", "teensy4_no_usb_shim.h")])
