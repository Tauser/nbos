#pragma once

#include <stdint.h>

namespace ncos::interfaces::power {

struct PowerSampleRaw {
  bool valid = false;
  bool external_power_detected = false;
  bool measured_from_sensor = false;
  uint16_t battery_mv = 0;
  uint8_t thermal_load_percent = 0;
};

class PowerPort {
 public:
  virtual ~PowerPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool read_sample(PowerSampleRaw* out_sample) = 0;
};

}  // namespace ncos::interfaces::power
