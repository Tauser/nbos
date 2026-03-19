#pragma once

#include <stdint.h>

#include "core/contracts/behavior_runtime_contracts.hpp"

namespace ncos::services::behavior {

class BehaviorService final {
 public:
  bool initialize(uint16_t service_id, uint64_t now_ms);

  bool tick(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms,
            ncos::core::contracts::BehaviorProposal* out_proposal);

  void on_governance_decision(const ncos::core::contracts::GovernanceDecision& decision,
                              uint64_t now_ms);

  [[nodiscard]] const ncos::core::contracts::BehaviorRuntimeState& state() const;

 private:
  static constexpr uint64_t BehaviorCooldownMs = 220;

  ncos::core::contracts::BehaviorProposal propose_energy_protect(
      const ncos::core::contracts::CompanionSnapshot& snapshot) const;
  ncos::core::contracts::BehaviorProposal propose_alert_scan(
      const ncos::core::contracts::CompanionSnapshot& snapshot) const;
  ncos::core::contracts::BehaviorProposal propose_attend_user(
      const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms) const;

  uint16_t service_id_ = 0;
  ncos::core::contracts::BehaviorRuntimeState state_{};
};

}  // namespace ncos::services::behavior

