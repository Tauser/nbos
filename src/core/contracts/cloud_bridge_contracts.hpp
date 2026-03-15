#pragma once

#include <stdint.h>

#include "core/contracts/cloud_sync_runtime_contracts.hpp"

namespace ncos::core::contracts {

enum class CloudExtensionCapability : uint8_t {
  kTelemetryHints = 1,
  kRoutineHint = 2,
  kPersonaPatch = 3,
};

struct CloudExtensionRequest {
  uint32_t trace_id = 0;
  CloudExtensionCapability capability = CloudExtensionCapability::kTelemetryHints;
  uint8_t priority = 0;
  uint64_t created_at_ms = 0;
  uint32_t ttl_ms = 0;
};

struct CloudExtensionResponse {
  bool accepted = false;
  bool applied = false;
  bool safe_offline_compatible = true;
  const char* reason = "none";
};

struct CloudBridgeRuntimeState {
  bool initialized = false;
  bool bridge_enabled = false;
  bool offline_authoritative = true;
  bool connected = false;
  bool degraded = false;

  uint64_t last_update_ms = 0;
  uint32_t extension_requests_total = 0;
  uint32_t extension_applied_total = 0;
  uint32_t extension_rejected_total = 0;

  const char* last_reason = "none";
  CloudSyncRuntimeState sync{};
};

constexpr CloudBridgeRuntimeState make_cloud_bridge_runtime_baseline() {
  return CloudBridgeRuntimeState{};
}

inline constexpr bool is_valid(const CloudExtensionRequest& request) {
  return request.trace_id != 0 && request.ttl_ms > 0;
}

}  // namespace ncos::core::contracts
