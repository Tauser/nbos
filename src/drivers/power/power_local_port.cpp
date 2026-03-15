#include "drivers/power/power_local_port.hpp"

#ifndef NCOS_NATIVE_TESTS
#include "esp_timer.h"
#endif

namespace ncos::drivers::power {

bool PowerLocalPort::ensure_ready() {
  return true;
}

bool PowerLocalPort::read_sample(ncos::interfaces::power::PowerSampleRaw* out_sample) {
  if (out_sample == nullptr) {
    return false;
  }

  ncos::interfaces::power::PowerSampleRaw sample{};
  sample.valid = true;
  sample.external_power_detected = false;
  sample.thermal_load_percent = 30;

#ifdef NCOS_NATIVE_TESTS
  sample.measured_from_sensor = false;
  sample.battery_mv = 3920;
#else
  // Placeholder until ADC/charger telemetry is wired on board.
  sample.measured_from_sensor = false;
  const uint64_t seconds = static_cast<uint64_t>(esp_timer_get_time() / 1000000ULL);
  const uint16_t ripple = static_cast<uint16_t>((seconds % 90ULL) / 6ULL);
  sample.battery_mv = static_cast<uint16_t>(3920U - ripple);
#endif

  *out_sample = sample;
  return true;
}

ncos::interfaces::power::PowerPort* acquire_shared_power_port() {
  static PowerLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::power
