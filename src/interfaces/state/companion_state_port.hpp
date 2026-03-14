#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::interfaces::state {

class CompanionStatePort {
 public:
  virtual ~CompanionStatePort() = default;

  virtual bool initialize(const ncos::core::contracts::CompanionStructuralState& structural,
                          ncos::core::contracts::CompanionStateWriter writer,
                          uint64_t now_ms) = 0;
  virtual bool ingest_runtime(const ncos::core::contracts::CompanionRuntimeSignal& runtime,
                              ncos::core::contracts::CompanionStateWriter writer,
                              uint64_t now_ms) = 0;
  virtual bool ingest_emotional(const ncos::core::contracts::CompanionEmotionalSignal& emotional,
                                ncos::core::contracts::CompanionStateWriter writer,
                                uint64_t now_ms) = 0;
  virtual bool ingest_attentional(
      const ncos::core::contracts::CompanionAttentionalSignal& attentional,
      ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) = 0;
  virtual bool ingest_energetic(const ncos::core::contracts::CompanionEnergeticSignal& energetic,
                                ncos::core::contracts::CompanionStateWriter writer,
                                uint64_t now_ms) = 0;
  virtual bool ingest_interactional(
      const ncos::core::contracts::CompanionInteractionSignal& interactional,
      ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) = 0;
  virtual bool ingest_governance_decision(
      const ncos::core::contracts::GovernanceDecision& decision,
      ncos::core::contracts::CompanionStateWriter writer, uint64_t now_ms) = 0;
  virtual ncos::core::contracts::CompanionSnapshot snapshot_for(
      ncos::core::contracts::CompanionStateReader reader) const = 0;
};

}  // namespace ncos::interfaces::state
