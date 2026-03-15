#pragma once

#include <stdint.h>

#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

enum class CloudSyncDomain : uint8_t {
  kRuntime = 1,
  kGovernance = 2,
  kEnergetic = 3,
  kAttentional = 4,
  kInteractional = 5,
  kEmotional = 6,
};

struct CloudSyncPacket {
  uint32_t revision = 0;
  uint64_t captured_at_ms = 0;

  bool include_runtime = false;
  CompanionRuntimeState runtime{};

  bool include_governance = false;
  CompanionGovernanceState governance{};

  bool include_energetic = false;
  CompanionEnergeticState energetic{};

  bool include_attentional = false;
  CompanionAttentionalState attentional{};

  bool include_interactional = false;
  CompanionInteractionState interactional{};

  bool include_emotional = false;
  CompanionEmotionalState emotional{};
};

struct CloudSyncRuntimeState {
  bool initialized = false;
  bool sync_enabled = false;
  bool component_available = false;
  bool connected = false;
  bool degraded = false;
  bool offline_fallback_active = false;

  uint64_t last_attempt_ms = 0;
  uint64_t last_success_ms = 0;
  uint64_t next_attempt_ms = 0;

  uint32_t synced_total = 0;
  uint32_t failed_total = 0;
  uint8_t consecutive_failures = 0;

  const char* last_reason = "none";
};

constexpr CloudSyncRuntimeState make_cloud_sync_runtime_baseline() {
  return CloudSyncRuntimeState{};
}

}  // namespace ncos::core::contracts
