#pragma once
#include <IOKit/IOKitLib.h>

// kIOMainPortDefault is the macOS 12+ rename of kIOMasterPortDefault.
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 120000
static constexpr mach_port_t kLLIOMainPort = kIOMainPortDefault;
#else
static constexpr mach_port_t kLLIOMainPort = kIOMasterPortDefault;
#endif
