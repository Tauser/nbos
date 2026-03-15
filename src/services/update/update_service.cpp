#include "services/update/update_service.hpp"

namespace ncos::services::update {

void UpdateService::bind_port(ncos::interfaces::update::UpdatePort* port) {
  port_ = port;
}

bool UpdateService::initialize(uint16_t service_id,
                               uint64_t now_ms,
                               const ncos::config::RuntimeConfig& config) {
  if (service_id == 0 || port_ == nullptr) {
    return false;
  }

  service_id_ = service_id;
  config_ = &config;
  state_ = ncos::core::contracts::make_update_runtime_baseline();
  state_.initialized = true;
  state_.started_ms = now_ms;
  state_.last_update_ms = now_ms;
  state_.ota_enabled = config.ota_enabled;
  state_.remote_allowed = config.ota_remote_allowed;

  if (!port_->ensure_ready() || !port_->read_boot_info(&boot_info_)) {
    state_.health = ncos::core::contracts::OtaHealth::kBlockedUnsafe;
    state_.last_reason = "update_port_unavailable";
    ++state_.blocked_total;
    return true;
  }

  state_.rollback_supported = boot_info_.rollback_supported;
  state_.pending_confirm = boot_info_.pending_verify;
  state_.running_slot = boot_info_.running_slot;

  if (!state_.ota_enabled) {
    state_.health = ncos::core::contracts::OtaHealth::kDisabled;
    state_.last_reason = "ota_disabled_by_policy";
    return true;
  }

  if (state_.remote_allowed && !state_.rollback_supported) {
    state_.health = ncos::core::contracts::OtaHealth::kBlockedUnsafe;
    state_.last_reason = "remote_without_rollback";
    ++state_.blocked_total;
    return true;
  }

  if (state_.pending_confirm) {
    state_.health = ncos::core::contracts::OtaHealth::kAwaitingConfirm;
    state_.confirm_deadline_ms = now_ms + config.ota_confirm_uptime_ms;
    state_.last_reason = "awaiting_image_confirm";
    return true;
  }

  state_.health = ncos::core::contracts::OtaHealth::kNominal;
  state_.last_reason = "ota_policy_nominal";
  return true;
}

ncos::core::contracts::UpdateDecision UpdateService::evaluate_boot_policy(uint64_t now_ms) {
  ++state_.checks_total;
  state_.last_update_ms = now_ms;

  if (!state_.initialized) {
    return make_blocked_decision("update_not_initialized", true, false, now_ms);
  }

  if (!state_.ota_enabled) {
    ncos::core::contracts::UpdateDecision decision{};
    decision.valid = true;
    decision.reason = "ota_disabled_offline_policy";
    return decision;
  }

  if (state_.remote_allowed && !state_.rollback_supported) {
    return make_blocked_decision("ota_remote_blocked_no_rollback", true, false, now_ms);
  }

  ncos::core::contracts::UpdateDecision decision{};
  decision.valid = true;
  decision.reason = "ota_policy_ok";
  return decision;
}

ncos::core::contracts::UpdateDecision UpdateService::tick(
    uint64_t now_ms,
    const ncos::core::runtime::RuntimeStatus& runtime_status) {
  ++state_.checks_total;
  state_.last_update_ms = now_ms;

  ncos::core::contracts::UpdateDecision decision{};
  decision.valid = true;
  decision.reason = "update_idle";

  if (!state_.initialized || !state_.ota_enabled || !state_.pending_confirm || config_ == nullptr) {
    return decision;
  }

  if (runtime_status.safe_mode && now_ms >= state_.confirm_deadline_ms) {
    state_.health = ncos::core::contracts::OtaHealth::kFallbackOperational;
    state_.last_reason = "pending_confirm_timeout_safe_mode";
    ++state_.fallback_total;

    decision.request_safe_fallback = true;
    decision.request_rollback = false;
    decision.reason = state_.last_reason;
    return decision;
  }

  const bool healthy_runtime = runtime_status.started && !runtime_status.safe_mode;
  if (healthy_runtime && now_ms >= state_.confirm_deadline_ms) {
    if (port_->confirm_running_image()) {
      state_.pending_confirm = false;
      state_.health = ncos::core::contracts::OtaHealth::kNominal;
      state_.last_reason = "image_confirmed";
      ++state_.confirm_success_total;

      decision.request_confirm = true;
      decision.reason = state_.last_reason;
      return decision;
    }

    state_.health = ncos::core::contracts::OtaHealth::kBlockedUnsafe;
    state_.last_reason = "image_confirm_failed";
    ++state_.blocked_total;

    decision.request_safe_fallback = true;
    decision.reason = state_.last_reason;
    return decision;
  }

  decision.reason = "awaiting_confirm_window";
  return decision;
}

const ncos::core::contracts::UpdateRuntimeState& UpdateService::state() const {
  return state_;
}

ncos::core::contracts::UpdateDecision UpdateService::make_blocked_decision(const char* reason,
                                                                           bool fallback,
                                                                           bool rollback,
                                                                           uint64_t now_ms) {
  state_.health = ncos::core::contracts::OtaHealth::kBlockedUnsafe;
  state_.last_reason = reason == nullptr ? "blocked" : reason;
  state_.last_update_ms = now_ms;
  ++state_.blocked_total;

  ncos::core::contracts::UpdateDecision decision{};
  decision.valid = true;
  decision.request_safe_fallback = fallback;
  decision.request_rollback = rollback;
  decision.reason = state_.last_reason;
  return decision;
}

}  // namespace ncos::services::update
