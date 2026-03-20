#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "interfaces/state/companion_state_port.hpp"

namespace ncos::core::state {

class CompanionStateStore final : public ncos::interfaces::state::CompanionStatePort {
 public:
  bool initialize(const ncos::core::contracts::CompanionStructuralState& structural,
                  ncos::core::contracts::CompanionStateWriter writer,
                  uint64_t now_ms) override;
  bool ingest_persistent_memory_signal(
      const ncos::core::contracts::CompanionPersistentMemorySignal& signal,
      ncos::core::contracts::CompanionStateWriter writer,
      uint64_t now_ms);
  bool ingest_runtime(const ncos::core::contracts::CompanionRuntimeSignal& runtime,
                      ncos::core::contracts::CompanionStateWriter writer,
                      uint64_t now_ms) override;
  bool ingest_emotional(const ncos::core::contracts::CompanionEmotionalSignal& emotional,
                        ncos::core::contracts::CompanionStateWriter writer,
                        uint64_t now_ms) override;
  bool ingest_attentional(const ncos::core::contracts::CompanionAttentionalSignal& attentional,
                          ncos::core::contracts::CompanionStateWriter writer,
                          uint64_t now_ms) override;
  bool ingest_energetic(const ncos::core::contracts::CompanionEnergeticSignal& energetic,
                        ncos::core::contracts::CompanionStateWriter writer,
                        uint64_t now_ms) override;
  bool ingest_interactional(const ncos::core::contracts::CompanionInteractionSignal& interactional,
                            ncos::core::contracts::CompanionStateWriter writer,
                            uint64_t now_ms) override;
  bool ingest_governance_decision(const ncos::core::contracts::GovernanceDecision& decision,
                                  ncos::core::contracts::CompanionStateWriter writer,
                                  uint64_t now_ms) override;
  ncos::core::contracts::CompanionSnapshot snapshot_for(
      ncos::core::contracts::CompanionStateReader reader) const override;

 private:
  static bool energy_protect_active(const ncos::core::contracts::CompanionSnapshot& snapshot);
  static bool user_attention_active(const ncos::core::contracts::CompanionSnapshot& snapshot);
  static bool stimulus_attention_active(const ncos::core::contracts::CompanionSnapshot& snapshot);
  static bool response_active(const ncos::core::contracts::CompanionSnapshot& snapshot);
  static ncos::core::contracts::CompanionPresenceMode derive_presence_mode(
      const ncos::core::contracts::CompanionSnapshot& snapshot);
  static ncos::core::contracts::CompanionProductState derive_product_state(
      const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);
  static ncos::core::contracts::CompanionStateTransitionCause derive_transition_cause(
      ncos::core::contracts::CompanionProductState previous_state,
      ncos::core::contracts::CompanionProductState next_state);
  static uint64_t hold_duration_for_state(ncos::core::contracts::CompanionProductState state);
  static bool session_activity_present(const ncos::core::contracts::CompanionSnapshot& snapshot);
  static uint8_t derive_session_engagement_percent(
      const ncos::core::contracts::CompanionSnapshot& snapshot, bool user_now, bool stimulus_now,
      bool response_now);
  static bool user_session_continuity_active(
      const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);
  static bool stimulus_session_continuity_active(
      const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);
  static int8_t derive_persistent_social_warmth_bias_percent(
      const ncos::core::contracts::CompanionPersistentMemorySignal& signal);
  static int8_t derive_persistent_response_energy_bias_percent(
      const ncos::core::contracts::CompanionPersistentMemorySignal& signal);
  static int16_t derive_persistent_continuity_window_bias_ms(
      const ncos::core::contracts::CompanionPersistentMemorySignal& signal);
  static int8_t derive_target_social_warmth_bias_percent(
      const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);
  static int8_t derive_target_response_energy_bias_percent(
      const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);
  static int16_t derive_target_continuity_window_bias_ms(
      const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);
  void refresh_session_memory(uint64_t now_ms,
                              ncos::core::contracts::CompanionProductState previous_state,
                              ncos::core::contracts::CompanionProductState next_state,
                              bool user_now,
                              bool stimulus_now,
                              bool response_now);

  void refresh_adaptive_personality(uint64_t now_ms);
  void refresh_derived_runtime_state(uint64_t now_ms);
  bool authorize_write(ncos::core::contracts::CompanionStateWriter writer,
                       ncos::core::contracts::CompanionStateDomain domain) const;

  ncos::core::contracts::CompanionSnapshot snapshot_{};
};

}  // namespace ncos::core::state
