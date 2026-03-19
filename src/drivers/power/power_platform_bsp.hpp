#pragma once

#include <stdint.h>

namespace ncos::drivers::power {

struct PowerSensingFlags {
  bool battery_sensor_present = false;
  bool external_power_sensor_present = false;
  bool thermal_sensor_present = false;
};

struct PowerFallbackModel {
  bool measured_from_sensor = false;
  bool external_power_detected = false;
  uint16_t baseline_battery_mv = 3920;
  uint16_t ripple_drop_mv = 14;
  uint16_t ripple_period_seconds = 90;
  uint8_t baseline_thermal_load_percent = 30;
};

struct PowerPlatformBsp {
  const char* board_name = "unknown";
  PowerSensingFlags sensing{};
  PowerFallbackModel fallback{};
};

const PowerPlatformBsp& active_power_platform_bsp();

}  // namespace ncos::drivers::power
