#include "hal/platform/monotonic_clock.hpp"

#if defined(ESP_PLATFORM)
#include "esp_timer.h"
#else
#include <chrono>
#endif

namespace ncos::hal::platform {

uint64_t monotonic_time_us() {
#if defined(ESP_PLATFORM)
  return static_cast<uint64_t>(esp_timer_get_time());
#else
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
#endif
}

uint64_t monotonic_time_ms() {
  return monotonic_time_us() / 1000ULL;
}

}  // namespace ncos::hal::platform
