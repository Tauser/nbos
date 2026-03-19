#include "drivers/power/power_local_port.hpp"

#include "drivers/power/power_platform_bsp.hpp"
#include "hal/platform/monotonic_clock.hpp"

namespace ncos::drivers::power {

bool PowerLocalPort::ensure_ready() {
  return true;
}

bool PowerLocalPort::read_sample(ncos::interfaces::power::PowerSampleRaw* out_sample) {
  if (out_sample == nullptr) {
    return false;
  }

  const auto& bsp = active_power_platform_bsp();
  ncos::interfaces::power::PowerSampleRaw sample{};
  sample.valid = true;
  sample.external_power_detected = bsp.fallback.external_power_detected;
  sample.thermal_load_percent = bsp.fallback.baseline_thermal_load_percent;
  sample.measured_from_sensor = bsp.fallback.measured_from_sensor;

  const uint64_t seconds = ncos::hal::platform::monotonic_time_ms() / 1000ULL;
  const uint16_t ripple = bsp.fallback.ripple_period_seconds == 0
                              ? 0
                              : static_cast<uint16_t>((seconds % bsp.fallback.ripple_period_seconds) / 6ULL);
  const uint16_t ripple_drop = ripple > bsp.fallback.ripple_drop_mv ? bsp.fallback.ripple_drop_mv : ripple;
  sample.battery_mv = static_cast<uint16_t>(bsp.fallback.baseline_battery_mv - ripple_drop);

  *out_sample = sample;
  return true;
}

ncos::interfaces::power::PowerPort* acquire_shared_power_port() {
  static PowerLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::power
