#include "services/power/power_service.hpp"

#include "config/system_config.hpp"
#include "hal/platform/runtime_logging.hpp"

namespace {
constexpr const char* Tag = "NCOS_POWER_SVC";
}

namespace ncos::services::power {

void PowerService::bind_port(ncos::interfaces::power::PowerPort* port) {
  port_ = port;
}

bool PowerService::initialize(uint16_t service_id, uint64_t now_ms) {
  if (service_id == 0 || port_ == nullptr) {
    NCOS_LOGW(Tag, "PowerService sem porta valida");
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
    if (state_.consecutive_sample_failures < 255U) {
      ++state_.consecutive_sample_failures;
    }

    const bool read_guard =
        state_.consecutive_sample_failures >= ncos::config::kGlobalConfig.runtime.power_guard_sample_failure_limit;

    if (read_guard) {
      state_.electrical_guard_active = true;
      if (!state_.electrical_guard_latched) {
        state_.electrical_guard_latched = true;
        ++state_.guard_trip_total;
      }
      state_.mode = ncos::core::contracts::EnergyMode::kCritical;

      ncos::core::contracts::CompanionEnergeticSignal energetic{};
      energetic.mode = state_.mode;
      energetic.battery_percent = state_.battery_percent;
      energetic.thermal_load_percent = state_.thermal_load_percent;
      energetic.external_power = state_.external_power;
      *out_energetic = energetic;

      next_probe_ms_ = now_ms + ncos::config::kGlobalConfig.runtime.power_probe_interval_ms;
      return true;
    }

    next_probe_ms_ = now_ms + ncos::config::kGlobalConfig.runtime.power_probe_interval_ms;
    return false;
  }

  state_.consecutive_sample_failures = 0;
  ++state_.sample_success_total;
  state_.sensor_backed = sample.measured_from_sensor;
  state_.external_power = sample.external_power_detected;
  state_.battery_mv = sample.battery_mv;
  state_.thermal_load_percent = sample.thermal_load_percent > 100 ? 100 : sample.thermal_load_percent;

  state_.battery_percent =
      ncos::core::contracts::battery_percent_from_mv(sample.battery_mv, BatteryMinMv, BatteryMaxMv);

  const bool brownout_guard = sample.battery_mv <= ncos::config::kGlobalConfig.runtime.power_guard_brownout_mv;
  const bool thermal_critical_guard =
      state_.thermal_load_percent >= ncos::config::kGlobalConfig.runtime.power_guard_thermal_critical_percent;
  const bool thermal_guard =
      state_.thermal_load_percent >= ncos::config::kGlobalConfig.runtime.power_guard_thermal_constrained_percent;
  const bool electrical_guard = brownout_guard || state_.battery_percent <= 12;

  state_.electrical_guard_active = electrical_guard;
  state_.thermal_guard_active = thermal_guard;

  if (state_.electrical_guard_active && !state_.electrical_guard_latched) {
    state_.electrical_guard_latched = true;
    ++state_.guard_trip_total;
  }

  if (state_.thermal_guard_active && !state_.thermal_guard_latched) {
    state_.thermal_guard_latched = true;
    ++state_.guard_trip_total;
  }

  state_.mode = choose_mode(sample,
                            state_.battery_percent,
                            companion.runtime.safe_mode,
                            state_.electrical_guard_active,
                            state_.thermal_guard_active,
                            thermal_critical_guard);

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
    bool runtime_safe_mode,
    bool electrical_guard,
    bool thermal_guard,
    bool thermal_critical_guard) {
  if (runtime_safe_mode || electrical_guard || thermal_critical_guard) {
    return ncos::core::contracts::EnergyMode::kCritical;
  }

  if (thermal_guard || battery_percent <= 30) {
    return ncos::core::contracts::EnergyMode::kConstrained;
  }

  if (sample.external_power_detected) {
    return ncos::core::contracts::EnergyMode::kCharging;
  }

  return ncos::core::contracts::EnergyMode::kNominal;
}

}  // namespace ncos::services::power
