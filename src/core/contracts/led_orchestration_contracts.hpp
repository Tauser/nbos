#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::core::contracts {

enum class LedPattern : uint8_t {
  kOff = 0,
  kSolid = 1,
  kPulseSlow = 2,
  kPulseFast = 3,
};

enum class LedPriority : uint8_t {
  kLow = 1,
  kMedium = 2,
  kHigh = 3,
  kCritical = 4,
};

struct LedState {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
  uint8_t intensity_percent = 0;
  LedPattern pattern = LedPattern::kOff;
};

struct LedRequest {
  bool active = false;
  uint16_t owner_service = 0;
  LedPriority priority = LedPriority::kLow;
  LedState state{};
  uint64_t expires_at_ms = 0;
};

struct LedRuntimeState {
  bool initialized = false;
  bool last_apply_ok = false;
  LedState applied_state{};
  LedPriority applied_priority = LedPriority::kLow;
  uint16_t applied_owner = 0;
  uint32_t apply_success_total = 0;
  uint32_t apply_failure_total = 0;
  uint32_t arbitration_total = 0;
  uint64_t last_update_ms = 0;
};

constexpr LedState make_led_off_state() {
  return LedState{};
}

constexpr LedRuntimeState make_led_runtime_baseline() {
  return LedRuntimeState{};
}

constexpr bool is_valid(const LedState& state) {
  return state.intensity_percent <= 100;
}

constexpr bool is_valid(const LedRequest& request, uint64_t now_ms) {
  return request.active && request.owner_service != 0 && request.expires_at_ms > now_ms &&
         is_valid(request.state);
}

}  // namespace ncos::core::contracts
