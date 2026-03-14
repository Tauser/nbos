#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "interfaces/state/companion_state_port.hpp"

namespace ncos::core::state {

class CompanionStateStore final : public ncos::interfaces::state::CompanionStatePort {
 public:
  void initialize(const ncos::core::contracts::CompanionStructuralState& structural,
                  uint64_t now_ms) override;
  void ingest_runtime(const ncos::core::contracts::CompanionRuntimeSignal& runtime,
                      uint64_t now_ms) override;
  void ingest_governance_decision(const ncos::core::contracts::GovernanceDecision& decision,
                                  uint64_t now_ms) override;
  ncos::core::contracts::CompanionSnapshot snapshot() const override;

 private:
  static ncos::core::contracts::CompanionPresenceMode presence_from_runtime(
      const ncos::core::contracts::CompanionRuntimeSignal& runtime);

  ncos::core::contracts::CompanionSnapshot snapshot_{};
};

}  // namespace ncos::core::state
