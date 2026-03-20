#include "services/sensing/touch_service.hpp"

#include "config/system_config.hpp"
#include "hal/platform/runtime_logging.hpp"

namespace {
constexpr const char* kTag = "NCOS_TOUCH_SVC";
constexpr uint16_t TriggerOnLevel = 360;
constexpr uint16_t TriggerOffLevel = 220;

bool resolve_trigger_active(bool currently_active, uint16_t normalized_level) {
  if (currently_active) {
    return normalized_level >= TriggerOffLevel;
  }

  return normalized_level >= TriggerOnLevel;
}
}

namespace ncos::services::sensing {

void TouchService::bind_port(ncos::interfaces::sensing::TouchPort* port) {
  port_ = port;
}

bool TouchService::initialize(uint64_t now_ms) {
  if (port_ == nullptr) {
    NCOS_LOGW(kTag, "Porta de touch nao conectada");
    return false;
  }

  const bool ready = port_->ensure_ready();
  const bool calibrated = ready && port_->calibrate_idle();

  state_.initialized = ready;
  state_.baseline_raw = port_->baseline_raw();
  state_.trigger_delta = port_->trigger_delta();
  state_.last_read_ok = false;
  state_.trigger_active = false;
  state_.trigger_rising_edge = false;
  state_.last_raw = 0;
  state_.normalized_level = 0;
  state_.last_read_ms = now_ms;
  state_.trigger_started_ms = 0;
  state_.last_trigger_ms = 0;
  state_.read_success_total = 0;
  state_.read_failure_total = 0;
  state_.trigger_activations_total = 0;
  next_probe_ms_ = now_ms;

  return ready && calibrated;
}

void TouchService::tick(uint64_t now_ms) {
  if (!state_.initialized || port_ == nullptr) {
    return;
  }

  if (now_ms < next_probe_ms_) {
    return;
  }

  uint32_t raw = 0;
  const bool ok = port_->read_raw(&raw);
  state_.last_read_ok = ok;
  state_.last_read_ms = now_ms;
  state_.trigger_rising_edge = false;

  if (ok) {
    state_.last_raw = raw;
    state_.normalized_level = normalize_touch_level(raw, state_.baseline_raw, state_.trigger_delta);

    const bool was_active = state_.trigger_active;
    state_.trigger_active = resolve_trigger_active(state_.trigger_active, state_.normalized_level);
    if (state_.trigger_active && !was_active) {
      state_.trigger_rising_edge = true;
      state_.trigger_started_ms = now_ms;
      state_.last_trigger_ms = now_ms;
      ++state_.trigger_activations_total;
    }

    ++state_.read_success_total;
  } else {
    ++state_.read_failure_total;
  }

  next_probe_ms_ = now_ms + ncos::config::kGlobalConfig.runtime.touch_probe_interval_ms;
}

const ncos::core::contracts::TouchRuntimeState& TouchService::state() const {
  return state_;
}

uint16_t TouchService::normalize_touch_level(uint32_t raw, uint32_t baseline, uint32_t trigger_delta) {
  if (trigger_delta == 0 || raw >= baseline) {
    return 0;
  }

  const uint32_t delta = baseline - raw;
  if (delta >= trigger_delta) {
    return 1000;
  }

  return static_cast<uint16_t>((delta * 1000U) / trigger_delta);
}

}  // namespace ncos::services::sensing
