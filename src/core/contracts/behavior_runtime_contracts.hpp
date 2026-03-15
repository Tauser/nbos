#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

enum class BehaviorProfile : uint8_t {
  kIdleObserve = 1,
  kAttendUser = 2,
  kAlertScan = 3,
  kEnergyProtect = 4,
};

struct BehaviorProposal {
  bool valid = false;
  BehaviorProfile profile = BehaviorProfile::kIdleObserve;
  ActionProposal proposal{};
  const char* rationale = "none";
};

struct BehaviorRuntimeState {
  bool initialized = false;
  bool has_pending = false;
  BehaviorProfile active_profile = BehaviorProfile::kIdleObserve;
  uint64_t last_tick_ms = 0;
  uint64_t last_emit_ms = 0;
  uint64_t last_accept_ms = 0;
  uint32_t emitted_total = 0;
  uint32_t accepted_total = 0;
  uint32_t preempted_total = 0;
  uint32_t rejected_total = 0;
};

}  // namespace ncos::core::contracts
