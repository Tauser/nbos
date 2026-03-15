#include "services/cloud/cloud_bridge_service.hpp"

namespace ncos::services::cloud {

void CloudBridgeService::bind_sync_port(ncos::interfaces::cloud::CloudSyncPort* port) {
  sync_service_.bind_port(port);
}

void CloudBridgeService::bind_extension_port(ncos::interfaces::cloud::CloudExtensionPort* port) {
  extension_port_ = port;
}

bool CloudBridgeService::initialize(uint16_t service_id,
                                    uint64_t now_ms,
                                    const ncos::config::RuntimeConfig& config) {
  if (service_id == 0) {
    return false;
  }

  service_id_ = service_id;
  config_ = &config;
  state_ = ncos::core::contracts::make_cloud_bridge_runtime_baseline();
  state_.initialized = true;
  state_.bridge_enabled = config.cloud_bridge_enabled;
  state_.offline_authoritative = true;

  const bool sync_ok = sync_service_.initialize(service_id, now_ms, config);
  state_.sync = sync_service_.state();

  if (!sync_ok) {
    state_.last_reason = "cloud_sync_init_failed";
    state_.degraded = true;
    return false;
  }

  if (!state_.bridge_enabled) {
    state_.last_reason = "bridge_disabled_offline_first";
    return true;
  }

  if (!state_.sync.component_available) {
    state_.degraded = true;
    state_.last_reason = "sync_component_unavailable";
    return true;
  }

  state_.last_reason = "bridge_ready_offline_authoritative";
  return true;
}

bool CloudBridgeService::tick(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) {
  if (!state_.initialized || config_ == nullptr) {
    return false;
  }

  if (!state_.bridge_enabled) {
    state_.last_update_ms = now_ms;
    state_.last_reason = "bridge_disabled_offline_first";
    return false;
  }

  const bool synced = sync_service_.tick(snapshot, now_ms);
  refresh_state_from_sync(now_ms);
  return synced;
}

bool CloudBridgeService::submit_extension(
    const ncos::core::contracts::CloudExtensionRequest& request,
    uint64_t now_ms,
    ncos::core::contracts::CloudExtensionResponse* out_response) {
  if (out_response == nullptr) {
    return false;
  }

  *out_response = ncos::core::contracts::CloudExtensionResponse{};

  if (!state_.initialized || config_ == nullptr) {
    out_response->reason = "bridge_not_initialized";
    return false;
  }

  ++state_.extension_requests_total;

  if (!ncos::core::contracts::is_valid(request)) {
    out_response->reason = "invalid_extension_request";
    ++state_.extension_rejected_total;
    return false;
  }

  if (!state_.bridge_enabled || !config_->cloud_extension_enabled) {
    out_response->reason = "extension_disabled_offline_first";
    ++state_.extension_rejected_total;
    return false;
  }

  if (!state_.connected || state_.degraded) {
    out_response->reason = "bridge_unavailable_keep_local";
    ++state_.extension_rejected_total;
    return false;
  }

  if (request.created_at_ms > now_ms || (now_ms - request.created_at_ms) > request.ttl_ms) {
    out_response->reason = "extension_request_expired";
    ++state_.extension_rejected_total;
    return false;
  }

  if (extension_port_ == nullptr || !extension_port_->ensure_ready()) {
    out_response->reason = "extension_port_unavailable";
    ++state_.extension_rejected_total;
    state_.degraded = true;
    return false;
  }

  ncos::core::contracts::CloudExtensionResponse response{};
  const bool submitted = extension_port_->submit_extension(request, &response);

  *out_response = response;
  if (!submitted || !response.accepted) {
    ++state_.extension_rejected_total;
    state_.last_reason = response.reason;
    return false;
  }

  if (response.applied) {
    ++state_.extension_applied_total;
  }

  state_.last_reason = response.reason;
  return true;
}

const ncos::core::contracts::CloudBridgeRuntimeState& CloudBridgeService::state() const {
  return state_;
}

void CloudBridgeService::refresh_state_from_sync(uint64_t now_ms) {
  state_.sync = sync_service_.state();
  state_.connected = state_.sync.connected;
  state_.degraded = state_.sync.degraded || !state_.sync.component_available;
  state_.offline_authoritative = true;
  state_.last_update_ms = now_ms;
  state_.last_reason = state_.sync.last_reason;
}

}  // namespace ncos::services::cloud
