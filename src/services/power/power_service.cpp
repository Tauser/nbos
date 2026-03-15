#include "services/power/power_service.hpp"

#include "config/system_config.hpp"

#ifndef NCOS_NATIVE_TESTS
#include "esp_log.h"
#else
#define ESP_LOGW(tag, fmt, ...) ((void)0)
#endif

namespace {
constexpr const char* Tag = "NCOS_POWER_SVC";
}

namespace ncos::services::power {

void PowerService::bind_port(ncos::interfaces::power::PowerPort* port) {
  port_ = port;
}

bool PowerService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0 || port_ == nullptr) {
    ESP_LOGW(Tag, "PowerService sem porta valida");
    return false;
  }

  service_id_ = service_id;
  state_ = ncos::core::contracts::make_power_runtime_baseline();
  state_.initialized = true;
  state_.sample_ready = port_->ensure_ready();
  state_.last_update_ms = now_ms;
  next_probe_ms_ = now_ms;
  return state_.sample_ready;
}

bool PowerService::tick(const ncos::core::contracts::CompanionSnapshot& companion,
                        uint64_t now_ms,
                        ncos::core::contracts::CompanionEnergeticSignal* out_energetic) {
  if (!state_.initialized || !state_.sample_ready || port_ == nullptr || out_energetic == nullptr) {
    return false;
  }

  if (now_ms < next_probe_ms_) {
    return false;
  }

  ncos::interfaces::power::PowerSampleRaw sample{};
  const bool ok = port_->read_sample(&sample);
  state_.last_update_ms = now_ms;

  if (!ok || !sample.valid) {
    ++state_.sample_failure_total;
    next_probe_ms_ = now_ms + ncos::config::kGlobalConfig.runtime.power_probe_interval_ms;
    return false;
  }

  ++state_.sample_success_total;
  state_.sensor_backed = sample.measured_from_sensor;
  state_.external_power = sample.external_power_detected;
  state_.battery_mv = sample.battery_mv;
  state_.thermal_load_percent = sample.thermal_load_percent > 100 ? 100 : sample.thermal_load_percent;

  state_.battery_percent =
      ncos::core::contracts::battery_percent_from_mv(sample.battery_mv, BatteryMinMv, BatteryMaxMv);
  state_.mode = choose_mode(sample, state_.battery_percent, companion.runtime.safe_mode);

  ncos::core::contracts::CompanionEnergeticSignal energetic{};
  energetic.mode = state_.mode;
  energetic.battery_percent = state_.battery_percent;
  energetic.thermal_load_percent = state_.thermal_load_percent;
  energetic.external_power = state_.external_power;

  *out_energetic = energetic;
  next_probe_ms_ = now_ms + ncos::config::kGlobalConfig.runtime.power_probe_interval_ms;
  return true;
}

const ncos::core::contracts::PowerRuntimeState& PowerService::state() const {
  return state_;
}

ncos::core::contracts::EnergyMode PowerService::choose_mode(
    const ncos::interfaces::power::PowerSampleRaw& sample,
    uint8_t battery_percent,
    bool runtime_safe_mode) {
  if (sample.external_power_detected) {
    return ncos::core::contracts::EnergyMode::kCharging;
  }

  if (runtime_safe_mode || battery_percent <= 12 || sample.battery_mv <= 3400) {
    return ncos::core::contracts::EnergyMode::kCritical;
  }

  if (battery_percent <= 30) {
    return ncos::core::contracts::EnergyMode::kConstrained;
  }

  return ncos::core::contracts::EnergyMode::kNominal;
}

}  // namespace ncos::services::power
