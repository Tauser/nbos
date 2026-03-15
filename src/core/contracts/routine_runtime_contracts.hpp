#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/behavior_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

enum class IdleRoutine : uint8_t {
  kNone = 0,
  kAmbientGazeSweep = 1,
  kUserPresencePulse = 2,
  kStimulusScanNudge = 3,
  kEnergySaveSettle = 4,
};

enum class AttentionMode : uint8_t {
  kAmbient = 1,
  kUserEngaged = 2,
  kStimulusTracking = 3,
  kEnergyConserve = 4,
};

struct RoutineProposal {
  bool valid = false;
  IdleRoutine routine = IdleRoutine::kNone;
  AttentionMode attention_mode = AttentionMode::kAmbient;
  ActionProposal proposal{};
  const char* rationale = "none";
};

struct RoutineRuntimeState {
  bool initialized = false;
  AttentionMode attention_mode = AttentionMode::kAmbient;
  IdleRoutine active_routine = IdleRoutine::kNone;
  bool has_pending = false;
  uint64_t last_tick_ms = 0;
  uint64_t last_emit_ms = 0;
  uint64_t last_accept_ms = 0;
  uint32_t emitted_total = 0;
  uint32_t accepted_total = 0;
  uint32_t preempted_total = 0;
  uint32_t rejected_total = 0;
};

}  // namespace ncos::core::contracts
