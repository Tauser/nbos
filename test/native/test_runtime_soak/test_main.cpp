#include <unity.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/interaction_taxonomy.hpp"
#include "core/state/companion_state_store.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/interaction_taxonomy.cpp"
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "core/state/companion_state_store.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_soak_companion_state_store_keeps_consistent_revision_and_bounds() {
  ncos::core::state::CompanionStateStore store;

  ncos::core::contracts::CompanionStructuralState structural{};
  structural.offline_first = true;
  structural.semantic_taxonomy_version = ncos::core::contracts::kSemanticTaxonomyVersion;
  structural.board_name = "test_board";

  TEST_ASSERT_TRUE(store.initialize(structural,
                                    ncos::core::contracts::CompanionStateWriter::kBootstrap,
                                    1000));

  constexpr uint32_t kIterations = 6000;
  uint64_t now_ms = 1000;

  for (uint32_t i = 0; i < kIterations; ++i) {
    ncos::core::contracts::CompanionRuntimeSignal runtime{};
    runtime.initialized = true;
    runtime.started = true;
    runtime.safe_mode = false;
    runtime.scheduler_tasks = 2;
    runtime.fault_count = 0;
    runtime.governance_allowed_total = i;
    runtime.governance_preempted_total = i / 4;
    runtime.governance_rejected_total = i / 8;

    ncos::core::contracts::CompanionEmotionalSignal emotional{};
    emotional.tone = ncos::core::contracts::EmotionalTone::kCurious;
    emotional.arousal = ncos::core::contracts::EmotionalArousal::kMedium;
    emotional.vector.valence_percent = static_cast<int8_t>((i % 2U == 0U) ? 120 : -120);
    emotional.vector.arousal_percent = static_cast<uint8_t>(130);
    emotional.vector.social_engagement_percent = static_cast<uint8_t>(140);
    emotional.intensity_percent = static_cast<uint8_t>(150);
    emotional.stability_percent = static_cast<uint8_t>(140);

    ncos::core::contracts::CompanionAttentionalSignal attentional{};
    attentional.target = (i % 3U == 0U) ? ncos::core::contracts::AttentionTarget::kUser
                                         : ncos::core::contracts::AttentionTarget::kStimulus;
    attentional.channel = ncos::core::contracts::AttentionChannel::kMultimodal;
    attentional.focus_confidence_percent = static_cast<uint8_t>(90);
    attentional.lock_active = (i % 4U) != 0U;

    ncos::core::contracts::CompanionEnergeticSignal energetic{};
    energetic.mode = (i % 5U == 0U) ? ncos::core::contracts::EnergyMode::kConstrained
                                     : ncos::core::contracts::EnergyMode::kNominal;
    energetic.battery_percent = static_cast<uint8_t>(50 + (i % 50U));
    energetic.thermal_load_percent = static_cast<uint8_t>(20 + (i % 40U));
    energetic.external_power = (i % 9U) == 0U;

    ncos::core::contracts::CompanionInteractionSignal interaction{};
    interaction.phase = (i % 2U == 0U) ? ncos::core::contracts::InteractionPhase::kListening
                                        : ncos::core::contracts::InteractionPhase::kActing;
    interaction.turn_owner = (i % 2U == 0U) ? ncos::core::contracts::TurnOwner::kUser
                                             : ncos::core::contracts::TurnOwner::kCompanion;
    interaction.session_active = true;
    interaction.response_pending = (i % 7U) == 0U;

    ncos::core::contracts::GovernanceDecision governance{};
    governance.kind = ncos::core::contracts::GovernanceDecisionKind::kAllow;
    governance.proposal_trace_id = i + 1;
    governance.domain = ncos::core::contracts::ActionDomain::kFace;
    governance.owner_service = 61;

    TEST_ASSERT_TRUE(store.ingest_runtime(runtime,
                                          ncos::core::contracts::CompanionStateWriter::kRuntimeCore,
                                          now_ms));
    TEST_ASSERT_TRUE(store.ingest_emotional(emotional,
                                            ncos::core::contracts::CompanionStateWriter::kEmotionService,
                                            now_ms));
    TEST_ASSERT_TRUE(store.ingest_attentional(attentional,
                                              ncos::core::contracts::CompanionStateWriter::kAttentionService,
                                              now_ms));
    TEST_ASSERT_TRUE(store.ingest_energetic(energetic,
                                            ncos::core::contracts::CompanionStateWriter::kPowerService,
                                            now_ms));
    TEST_ASSERT_TRUE(store.ingest_interactional(interaction,
                                                ncos::core::contracts::CompanionStateWriter::kInteractionService,
                                                now_ms));
    TEST_ASSERT_TRUE(store.ingest_governance_decision(
        governance, ncos::core::contracts::CompanionStateWriter::kGovernanceCore, now_ms));

    now_ms += 50;
  }

  const auto snapshot =
      store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  const uint32_t expected_revision = 1U + (kIterations * 6U);
  TEST_ASSERT_EQUAL_UINT32(expected_revision, snapshot.revision);

  TEST_ASSERT_TRUE(snapshot.runtime.initialized);
  TEST_ASSERT_TRUE(snapshot.runtime.started);
  TEST_ASSERT_FALSE(snapshot.runtime.safe_mode);
  TEST_ASSERT_EQUAL_UINT32(2, snapshot.runtime.scheduler_tasks);

  TEST_ASSERT_TRUE(snapshot.emotional.vector.valence_percent <= 100);
  TEST_ASSERT_TRUE(snapshot.emotional.vector.valence_percent >= -100);
  TEST_ASSERT_TRUE(snapshot.emotional.vector.arousal_percent <= 100);
  TEST_ASSERT_TRUE(snapshot.emotional.vector.social_engagement_percent <= 100);

  TEST_ASSERT_TRUE(snapshot.transient.has_active_trace);
  TEST_ASSERT_EQUAL_UINT16(61, snapshot.transient.active_owner_service);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_soak_companion_state_store_keeps_consistent_revision_and_bounds);
  return UNITY_END();
}
