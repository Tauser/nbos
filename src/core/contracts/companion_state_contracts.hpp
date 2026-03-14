#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"

namespace ncos::core::contracts {

enum class CompanionPresenceMode : uint8_t {
  kIdle = 1,
  kAttending = 2,
  kDormant = 3,
};

enum class GovernanceHealth : uint8_t {
  kUnknown = 1,
  kStable = 2,
  kContended = 3,
};

struct CompanionStructuralState {
  bool offline_first = true;
  uint16_t semantic_taxonomy_version = 0;
  const char* board_name = "unknown";
};

struct CompanionTransientState {
  bool has_active_trace = false;
  uint32_t active_trace_id = 0;
  ActionDomain active_domain = ActionDomain::kFace;
  uint16_t active_owner_service = 0;
  uint64_t last_transition_ms = 0;
};

struct CompanionRuntimeState {
  bool initialized = false;
  bool started = false;
  bool safe_mode = false;
  uint32_t scheduler_tasks = 0;
  uint32_t fault_count = 0;
  CompanionPresenceMode presence_mode = CompanionPresenceMode::kIdle;
};

struct CompanionGovernanceState {
  uint32_t allowed_total = 0;
  uint32_t preempted_total = 0;
  uint32_t rejected_total = 0;
  GovernanceHealth health = GovernanceHealth::kUnknown;
};

struct CompanionSnapshot {
  CompanionStructuralState structural{};
  CompanionRuntimeState runtime{};
  CompanionGovernanceState governance{};
  CompanionTransientState transient{};
  uint32_t revision = 0;
  uint64_t captured_at_ms = 0;
};

struct CompanionRuntimeSignal {
  bool initialized = false;
  bool started = false;
  bool safe_mode = false;
  uint32_t scheduler_tasks = 0;
  uint32_t fault_count = 0;
  uint32_t governance_allowed_total = 0;
  uint32_t governance_preempted_total = 0;
  uint32_t governance_rejected_total = 0;
};

GovernanceHealth evaluate_governance_health(uint32_t allowed_total, uint32_t preempted_total,
                                            uint32_t rejected_total);

}  // namespace ncos::core::contracts
