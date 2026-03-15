#pragma once

#include <stdint.h>

#include "core/contracts/cloud_bridge_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

struct TelemetryCollectionSurface {
  bool runtime = true;
  bool governance = true;
  bool energetic = true;
  bool cloud = true;
  bool interactional = false;
  bool emotional = false;
  bool transient = false;
};

struct TelemetryRuntimeInput {
  bool initialized = false;
  bool started = false;
  bool safe_mode = false;
  uint32_t scheduler_tasks = 0;
  uint32_t fault_count = 0;
  uint32_t bus_published_total = 0;
  uint32_t bus_dispatched_total = 0;
  uint32_t bus_dropped_total = 0;
  uint32_t governance_allowed_total = 0;
  uint32_t governance_preempted_total = 0;
  uint32_t governance_rejected_total = 0;
  uint32_t companion_state_revision = 0;
};

struct TelemetrySample {
  uint16_t source_service = 0;
  uint64_t captured_at_ms = 0;
  uint64_t sequence = 0;

  TelemetryCollectionSurface surface{};
  TelemetryRuntimeInput runtime{};

  bool include_energetic = false;
  CompanionEnergeticState energetic{};

  bool include_cloud = false;
  bool cloud_connected = false;
  bool cloud_degraded = false;
  bool cloud_offline_authoritative = true;

  bool include_interactional = false;
  CompanionInteractionState interactional{};

  bool include_emotional = false;
  CompanionEmotionalState emotional{};

  bool include_transient = false;
  CompanionTransientState transient{};
};

struct TelemetryRuntimeState {
  bool initialized = false;
  bool telemetry_enabled = false;
  bool export_off_device = false;
  bool component_available = false;
  bool degraded = false;

  uint64_t last_tick_ms = 0;
  uint64_t last_success_ms = 0;
  uint64_t next_send_ms = 0;
  uint64_t sequence = 0;

  uint32_t sampled_total = 0;
  uint32_t sent_total = 0;
  uint32_t rejected_total = 0;

  const char* last_reason = "none";
};

constexpr TelemetryRuntimeState make_telemetry_runtime_baseline() {
  return TelemetryRuntimeState{};
}

inline constexpr bool is_surface_valid(const TelemetryCollectionSurface& surface) {
  return surface.runtime || surface.governance || surface.energetic || surface.cloud ||
         surface.interactional || surface.emotional || surface.transient;
}

}  // namespace ncos::core::contracts
