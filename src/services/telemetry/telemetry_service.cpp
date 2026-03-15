#include "services/telemetry/telemetry_service.hpp"

namespace ncos::services::telemetry {

void TelemetryService::bind_port(ncos::interfaces::telemetry::TelemetryPort* port) {
  port_ = port;
}

bool TelemetryService::initialize(uint16_t service_id,
                                  uint64_t now_ms,
                                  const ncos::config::RuntimeConfig& config) {
  if (service_id == 0 || port_ == nullptr) {
    return false;
  }

  service_id_ = service_id;
  config_ = &config;
  state_ = ncos::core::contracts::make_telemetry_runtime_baseline();
  state_.initialized = true;
  state_.telemetry_enabled = config.telemetry_enabled;
  state_.export_off_device = config.telemetry_export_off_device;
  state_.component_available = port_->ensure_ready();
  state_.next_send_ms = now_ms;
  state_.last_reason = state_.telemetry_enabled ? "telemetry_ready" : "telemetry_disabled";

  return true;
}

bool TelemetryService::tick(const ncos::core::contracts::TelemetryRuntimeInput& runtime,
                            const ncos::core::contracts::CompanionSnapshot& companion,
                            const ncos::core::contracts::CloudBridgeRuntimeState& cloud,
                            uint64_t now_ms) {
  if (!state_.initialized || config_ == nullptr) {
    return false;
  }

  state_.last_tick_ms = now_ms;

  if (!state_.telemetry_enabled) {
    state_.last_reason = "telemetry_disabled";
    return false;
  }

  if (!state_.export_off_device) {
    state_.last_reason = "telemetry_local_only";
    return false;
  }

  if (!state_.component_available || port_ == nullptr) {
    state_.degraded = true;
    state_.last_reason = "telemetry_port_unavailable";
    return false;
  }

  if (now_ms < state_.next_send_ms) {
    state_.last_reason = "telemetry_rate_limited";
    return false;
  }

  const ncos::core::contracts::TelemetryCollectionSurface surface = surface_from_config();
  if (!ncos::core::contracts::is_surface_valid(surface)) {
    ++state_.rejected_total;
    state_.last_reason = "telemetry_surface_invalid";
    state_.next_send_ms = now_ms + config_->telemetry_interval_ms;
    return false;
  }

  ncos::core::contracts::TelemetrySample sample = make_sample(runtime, companion, cloud, now_ms);
  sample.surface = surface;

  ++state_.sampled_total;

  const bool sent = port_->publish_sample(sample);
  state_.next_send_ms = now_ms + config_->telemetry_interval_ms;

  if (!sent) {
    ++state_.rejected_total;
    state_.degraded = true;
    state_.last_reason = "telemetry_publish_failed";
    return false;
  }

  ++state_.sent_total;
  state_.sequence = sample.sequence;
  state_.last_success_ms = now_ms;
  state_.degraded = false;
  state_.last_reason = "telemetry_publish_ok";
  return true;
}

const ncos::core::contracts::TelemetryRuntimeState& TelemetryService::state() const {
  return state_;
}

ncos::core::contracts::TelemetryCollectionSurface TelemetryService::surface_from_config() const {
  ncos::core::contracts::TelemetryCollectionSurface surface{};
  if (config_ == nullptr) {
    return surface;
  }

  surface.runtime = true;
  surface.governance = true;
  surface.energetic = true;
  surface.cloud = true;
  surface.interactional = config_->telemetry_collect_interactional;
  surface.emotional = config_->telemetry_collect_emotional;
  surface.transient = config_->telemetry_collect_transient;
  return surface;
}

ncos::core::contracts::TelemetrySample TelemetryService::make_sample(
    const ncos::core::contracts::TelemetryRuntimeInput& runtime,
    const ncos::core::contracts::CompanionSnapshot& companion,
    const ncos::core::contracts::CloudBridgeRuntimeState& cloud,
    uint64_t now_ms) const {
  ncos::core::contracts::TelemetrySample sample{};
  sample.source_service = service_id_;
  sample.captured_at_ms = now_ms;
  sample.sequence = state_.sequence + 1ULL;
  sample.runtime = runtime;

  sample.include_energetic = true;
  sample.energetic = companion.energetic;

  sample.include_cloud = true;
  sample.cloud_connected = cloud.connected;
  sample.cloud_degraded = cloud.degraded;
  sample.cloud_offline_authoritative = cloud.offline_authoritative;

  if (config_ != nullptr && config_->telemetry_collect_interactional) {
    sample.include_interactional = true;
    sample.interactional = companion.interactional;
  }

  if (config_ != nullptr && config_->telemetry_collect_emotional) {
    sample.include_emotional = true;
    sample.emotional = companion.emotional;
  }

  if (config_ != nullptr && config_->telemetry_collect_transient) {
    sample.include_transient = true;
    sample.transient = companion.transient;
  }

  return sample;
}

}  // namespace ncos::services::telemetry
