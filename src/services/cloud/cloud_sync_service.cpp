#include "services/cloud/cloud_sync_service.hpp"

namespace {
constexpr uint8_t AttentionIncludeThreshold = 40;
constexpr uint8_t EmotionalIncludeThreshold = 35;
}

namespace ncos::services::cloud {

void CloudSyncService::bind_port(ncos::interfaces::cloud::CloudSyncPort* port) {
  port_ = port;
}

bool CloudSyncService::initialize(uint16_t service_id,
                                  uint64_t now_ms,
                                  const ncos::config::RuntimeConfig& config) {
  if (service_id == 0 || port_ == nullptr) {
    return false;
  }

  service_id_ = service_id;
  config_ = &config;
  state_ = ncos::core::contracts::make_cloud_sync_runtime_baseline();
  state_.initialized = true;
  state_.sync_enabled = config.cloud_sync_enabled;
  state_.component_available = port_->ensure_ready();
  state_.connected = false;
  state_.degraded = false;
  state_.offline_fallback_active = false;
  state_.next_attempt_ms = now_ms;
  state_.last_reason = state_.sync_enabled ? "sync_ready" : "sync_disabled_offline_first";
  return true;
}

bool CloudSyncService::tick(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!state_.initialized || config_ == nullptr || !state_.sync_enabled || !state_.component_available) {
    return false;
  }

  if (now_ms < state_.next_attempt_ms) {
    return false;
  }

  state_.last_attempt_ms = now_ms;

  const ncos::core::contracts::CloudSyncPacket packet = make_selective_packet(snapshot);
  const bool ok = port_->send_packet(packet);

  if (ok) {
    state_.connected = true;
    state_.degraded = false;
    state_.offline_fallback_active = false;
    state_.consecutive_failures = 0;
    ++state_.synced_total;
    state_.last_success_ms = now_ms;
    state_.next_attempt_ms = now_ms + config_->cloud_sync_interval_ms;
    state_.last_reason = "sync_ok";
    return true;
  }

  state_.connected = false;
  ++state_.failed_total;
  if (state_.consecutive_failures < 255U) {
    ++state_.consecutive_failures;
  }

  if (state_.consecutive_failures >= config_->cloud_sync_failure_threshold) {
    state_.degraded = true;
    state_.offline_fallback_active = true;
    state_.last_reason = "offline_fallback_active";
  } else {
    state_.last_reason = "sync_retry_backoff";
  }

  state_.next_attempt_ms = now_ms + config_->cloud_sync_retry_backoff_ms;
  return false;
}

const ncos::core::contracts::CloudSyncRuntimeState& CloudSyncService::state() const {
  return state_;
}

bool CloudSyncService::should_include_attentional(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return snapshot.attentional.target != ncos::core::contracts::AttentionTarget::kNone &&
         snapshot.attentional.focus_confidence_percent >= AttentionIncludeThreshold;
}

bool CloudSyncService::should_include_interactional(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return snapshot.interactional.session_active || snapshot.interactional.response_pending;
}

bool CloudSyncService::should_include_emotional(const ncos::core::contracts::CompanionSnapshot& snapshot) {
  return snapshot.emotional.intensity_percent >= EmotionalIncludeThreshold;
}

ncos::core::contracts::CloudSyncPacket CloudSyncService::make_selective_packet(
    const ncos::core::contracts::CompanionSnapshot& snapshot) {
  ncos::core::contracts::CloudSyncPacket packet{};
  packet.revision = snapshot.revision;
  packet.captured_at_ms = snapshot.captured_at_ms;

  // Baseline cloud sync stays selective: structural offline-first remains local authority.
  packet.include_runtime = true;
  packet.runtime = snapshot.runtime;

  packet.include_governance = true;
  packet.governance = snapshot.governance;

  packet.include_energetic = true;
  packet.energetic = snapshot.energetic;

  packet.include_attentional = should_include_attentional(snapshot);
  if (packet.include_attentional) {
    packet.attentional = snapshot.attentional;
  }

  packet.include_interactional = should_include_interactional(snapshot);
  if (packet.include_interactional) {
    packet.interactional = snapshot.interactional;
  }

  packet.include_emotional = should_include_emotional(snapshot);
  if (packet.include_emotional) {
    packet.emotional = snapshot.emotional;
  }

  return packet;
}

}  // namespace ncos::services::cloud
