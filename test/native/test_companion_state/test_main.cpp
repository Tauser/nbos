#include <unity.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/state/companion_state_store.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "core/state/companion_state_store.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_companion_state_keeps_structural_source_of_truth() {
  ncos::core::state::CompanionStateStore store;

  ncos::core::contracts::CompanionStructuralState structural{};
  structural.offline_first = true;
  structural.semantic_taxonomy_version = 1;
  structural.board_name = "esp32s3_dev";

  TEST_ASSERT_TRUE(store.initialize(structural, ncos::core::contracts::CompanionStateWriter::kBootstrap,
                                    1000));
  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_TRUE(snap.structural.offline_first);
  TEST_ASSERT_EQUAL_UINT16(1, snap.structural.semantic_taxonomy_version);
  TEST_ASSERT_EQUAL_STRING("esp32s3_dev", snap.structural.board_name);
}

void test_companion_state_rejects_unauthorized_write() {
  ncos::core::state::CompanionStateStore store;

  ncos::core::contracts::CompanionStructuralState structural{};
  structural.board_name = "nao_deve_aplicar";

  TEST_ASSERT_FALSE(
      store.initialize(structural, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1000));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_STRING("unknown", snap.structural.board_name);
}

void test_companion_state_updates_emotional_domain() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionEmotionalSignal emotional{};
  emotional.tone = ncos::core::contracts::EmotionalTone::kCurious;
  emotional.arousal = ncos::core::contracts::EmotionalArousal::kMedium;
  emotional.intensity_percent = 62;
  emotional.stability_percent = 74;

  TEST_ASSERT_TRUE(
      store.ingest_emotional(emotional, ncos::core::contracts::CompanionStateWriter::kEmotionService,
                             1100));
  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EmotionalTone::kCurious),
                        static_cast<int>(snap.emotional.tone));
  TEST_ASSERT_EQUAL_UINT8(62, snap.emotional.intensity_percent);
  TEST_ASSERT_EQUAL_INT8(10, snap.emotional.vector.valence_percent);
  TEST_ASSERT_EQUAL_UINT8(58, snap.emotional.vector.social_engagement_percent);
}

void test_companion_state_normalizes_authoritative_emotion_vector() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionEmotionalSignal emotional{};
  emotional.vector_authoritative = true;
  emotional.vector.valence_percent = 85;
  emotional.vector.arousal_percent = 48;
  emotional.vector.social_engagement_percent = 88;
  emotional.intensity_percent = 64;
  emotional.stability_percent = 76;

  TEST_ASSERT_TRUE(
      store.ingest_emotional(emotional, ncos::core::contracts::CompanionStateWriter::kEmotionService,
                             1115));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EmotionalTone::kAffiliative),
                        static_cast<int>(snap.emotional.tone));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EmotionalArousal::kMedium),
                        static_cast<int>(snap.emotional.arousal));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::emotion::EmotionPhase::kEngaged),
                        static_cast<int>(snap.emotional.phase));
  TEST_ASSERT_EQUAL_INT8(85, snap.emotional.vector.valence_percent);
  TEST_ASSERT_EQUAL_UINT8(48, snap.emotional.vector.arousal_percent);
  TEST_ASSERT_EQUAL_UINT8(88, snap.emotional.vector.social_engagement_percent);
}
void test_companion_state_updates_attentional_and_interactional_link() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kMultimodal;
  attentional.focus_confidence_percent = 85;
  attentional.lock_active = true;

  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1110));
  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(snap.attentional.target));
  TEST_ASSERT_TRUE(snap.interactional.session_active);
}

void test_companion_state_updates_energetic_domain() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionEnergeticSignal energetic{};
  energetic.mode = ncos::core::contracts::EnergyMode::kConstrained;
  energetic.battery_percent = 28;
  energetic.thermal_load_percent = 66;
  energetic.external_power = false;

  TEST_ASSERT_TRUE(
      store.ingest_energetic(energetic, ncos::core::contracts::CompanionStateWriter::kPowerService,
                             1120));
  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kConstrained),
                        static_cast<int>(snap.energetic.mode));
  TEST_ASSERT_EQUAL_UINT8(28, snap.energetic.battery_percent);
}

void test_companion_state_runtime_safe_mode_constrains_interaction_and_energy() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  interaction.session_active = true;
  interaction.response_pending = true;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1130));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.safe_mode = true;
  runtime.scheduler_tasks = 2;
  runtime.governance_rejected_total = 4;

  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1140));
  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kCritical),
                        static_cast<int>(snap.energetic.mode));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kIdle),
                        static_cast<int>(snap.interactional.phase));
  TEST_ASSERT_FALSE(snap.interactional.response_pending);
}

void test_companion_state_tracks_transient_governance_transition() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::GovernanceDecision decision{};
  decision.kind = ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow;
  decision.proposal_trace_id = 777;
  decision.domain = ncos::core::contracts::ActionDomain::kMotion;
  decision.owner_service = 42;

  TEST_ASSERT_TRUE(store.ingest_governance_decision(
      decision, ncos::core::contracts::CompanionStateWriter::kGovernanceCore, 1150));
  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_TRUE(snap.transient.has_active_trace);
  TEST_ASSERT_EQUAL_UINT32(777, snap.transient.active_trace_id);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::ActionDomain::kMotion),
                        static_cast<int>(snap.transient.active_domain));
  TEST_ASSERT_EQUAL_UINT16(42, snap.transient.active_owner_service);
}

void test_companion_state_redacts_by_reader_profile() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionEmotionalSignal emotional{};
  emotional.vector_authoritative = true;
  emotional.vector.valence_percent = -30;
  emotional.vector.arousal_percent = 82;
  emotional.vector.social_engagement_percent = 32;
  emotional.intensity_percent = 72;
  TEST_ASSERT_TRUE(
      store.ingest_emotional(emotional, ncos::core::contracts::CompanionStateWriter::kEmotionService,
                             1100));

  ncos::core::contracts::CompanionEnergeticSignal energetic{};
  energetic.mode = ncos::core::contracts::EnergyMode::kConstrained;
  energetic.battery_percent = 25;
  TEST_ASSERT_TRUE(
      store.ingest_energetic(energetic, ncos::core::contracts::CompanionStateWriter::kPowerService,
                             1110));

  const auto face_view = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kFaceService);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EmotionalTone::kAlert),
                        static_cast<int>(face_view.emotional.tone));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kNominal),
                        static_cast<int>(face_view.energetic.mode));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_companion_state_keeps_structural_source_of_truth);
  RUN_TEST(test_companion_state_rejects_unauthorized_write);
  RUN_TEST(test_companion_state_updates_emotional_domain);
  RUN_TEST(test_companion_state_normalizes_authoritative_emotion_vector);
  RUN_TEST(test_companion_state_updates_attentional_and_interactional_link);
  RUN_TEST(test_companion_state_updates_energetic_domain);
  RUN_TEST(test_companion_state_runtime_safe_mode_constrains_interaction_and_energy);
  RUN_TEST(test_companion_state_tracks_transient_governance_transition);
  RUN_TEST(test_companion_state_redacts_by_reader_profile);
  return UNITY_END();
}






