#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::interfaces::state {

class CompanionStatePort {
 public:
  virtual ~CompanionStatePort() = default;

  virtual void initialize(const ncos::core::contracts::CompanionStructuralState& structural,
                          uint64_t now_ms) = 0;
  virtual void ingest_runtime(const ncos::core::contracts::CompanionRuntimeSignal& runtime,
                              uint64_t now_ms) = 0;
  virtual void ingest_emotional(const ncos::core::contracts::CompanionEmotionalSignal& emotional,
                                uint64_t now_ms) = 0;
  virtual void ingest_attentional(
      const ncos::core::contracts::CompanionAttentionalSignal& attentional, uint64_t now_ms) = 0;
  virtual void ingest_energetic(const ncos::core::contracts::CompanionEnergeticSignal& energetic,
                                uint64_t now_ms) = 0;
  virtual void ingest_interactional(
      const ncos::core::contracts::CompanionInteractionSignal& interactional,
      uint64_t now_ms) = 0;
  virtual void ingest_governance_decision(const ncos::core::contracts::GovernanceDecision& decision,
                                          uint64_t now_ms) = 0;
  virtual ncos::core::contracts::CompanionSnapshot snapshot() const = 0;
};

}  // namespace ncos::interfaces::state
