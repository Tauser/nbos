#pragma once

#include <stdint.h>

namespace ncos::core::contracts {

struct TouchRuntimeState {
  bool initialized = false;
  bool last_read_ok = false;
  uint32_t last_raw = 0;
  uint16_t normalized_level = 0;  // 0..1000
  uint32_t baseline_raw = 0;
  uint32_t trigger_delta = 0;
  uint64_t last_read_ms = 0;
  uint32_t read_success_total = 0;
  uint32_t read_failure_total = 0;
};

struct ImuRuntimeState {
  bool initialized = false;
  bool last_read_ok = false;
  int16_t ax_raw = 0;
  int16_t ay_raw = 0;
  int16_t az_raw = 0;
  int16_t gx_raw = 0;
  int16_t gy_raw = 0;
  int16_t gz_raw = 0;
  int16_t ax_mg = 0;
  int16_t ay_mg = 0;
  int16_t az_mg = 0;
  int16_t gx_dps = 0;
  int16_t gy_dps = 0;
  int16_t gz_dps = 0;
  uint64_t last_read_ms = 0;
  uint32_t read_success_total = 0;
  uint32_t read_failure_total = 0;
};

constexpr TouchRuntimeState make_touch_runtime_baseline() {
  return TouchRuntimeState{};
}

constexpr ImuRuntimeState make_imu_runtime_baseline() {
  return ImuRuntimeState{};
}

}  // namespace ncos::core::contracts
