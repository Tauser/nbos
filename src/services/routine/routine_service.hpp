#pragma once

#include <stdint.h>

#include "core/contracts/routine_runtime_contracts.hpp"

namespace ncos::services::routine {

class RoutineService final {
 public:
  bool initialize(uint16_t service_id, uint64_t now_ms);

  bool tick(const ncos::core::contracts::CompanionSnapshot& snapshot,
            const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
            uint64_t now_ms,
            ncos::core::contracts::RoutineProposal* out_proposal);

  void on_governance_decision(const ncos::core::contracts::GovernanceDecision& decision,
                              uint64_t now_ms);

  [[nodiscard]] const ncos::core::contracts::RoutineRuntimeState& state() const;

 private:
  static constexpr uint64_t RoutineCooldownMs = 900;
  static constexpr uint64_t BehaviorSuppressionMs = 650;

  ncos::core::contracts::AttentionMode resolve_attention_mode(
      const ncos::core::contracts::CompanionSnapshot& snapshot) const;

  ncos::core::contracts::RoutineProposal propose_for_mode(
      ncos::core::contracts::AttentionMode mode,
      const ncos::core::contracts::CompanionSnapshot& snapshot,
      uint64_t now_ms) const;

  bool should_suppress_by_behavior(const ncos::core::contracts::BehaviorRuntimeState& behavior_state,
                                   uint64_t now_ms) const;

  uint16_t service_id_ = 0;
  ncos::core::contracts::RoutineRuntimeState state_{};
};

}  // namespace ncos::services::routine

