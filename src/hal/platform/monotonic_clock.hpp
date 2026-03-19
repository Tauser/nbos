#pragma once

#include <stdint.h>

namespace ncos::hal::platform {

uint64_t monotonic_time_us();
uint64_t monotonic_time_ms();

}  // namespace ncos::hal::platform
