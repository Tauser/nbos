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

void test_companion_state_exposes_personality_from_central_snapshot() {
  ncos::core::state::CompanionStateStore store;

  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));
  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_EQUAL_STRING("companion_core", snap.personality.profile_name);
  TEST_ASSERT_EQUAL_UINT8(68, snap.personality.warmth_percent);
  TEST_ASSERT_EQUAL_UINT8(58, snap.personality.curiosity_percent);
  TEST_ASSERT_EQUAL_INT8(0, snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_EQUAL_INT8(0, snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_EQUAL_INT16(0, snap.personality.adaptive_continuity_window_bias_ms);
  TEST_ASSERT_EQUAL_UINT16(190, snap.personality.reengagement_ttl_ms);
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

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1080));

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
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kAttendUser),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionPresenceMode::kAttending),
                        static_cast<int>(snap.runtime.presence_mode));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kUserTrigger),
                        static_cast<int>(snap.runtime.last_transition_cause));
  TEST_ASSERT_GREATER_THAN_UINT64(1110, snap.runtime.state_hold_until_ms);
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
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kEnergyProtect),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kEnergyGuard),
                        static_cast<int>(snap.runtime.last_transition_cause));
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

void test_companion_state_tracks_boot_idle_responding_and_recovery() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kRuntimeStarted),
                        static_cast<int>(snap.runtime.last_transition_cause));
  TEST_ASSERT_EQUAL_UINT64(0, snap.runtime.state_hold_until_ms);

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  interaction.session_active = true;
  interaction.response_pending = true;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1200));

  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kResponding),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kCompanionResponding),
                        static_cast<int>(snap.runtime.last_transition_cause));

  interaction.phase = ncos::core::contracts::InteractionPhase::kIdle;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kNone;
  interaction.session_active = false;
  interaction.response_pending = false;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1400));

  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kResponding),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kCompanionResponding),
                        static_cast<int>(snap.runtime.last_transition_cause));

  TEST_ASSERT_TRUE(store.ingest_runtime(runtime,
                                        ncos::core::contracts::CompanionStateWriter::kRuntimeCore,
                                        2200));
  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kRecoveryToIdle),
                        static_cast<int>(snap.runtime.last_transition_cause));
  TEST_ASSERT_EQUAL_UINT32(3, snap.runtime.state_transition_total);
}

void test_companion_state_marks_alert_scan_from_world_attention() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kStimulus;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 68;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1250));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kAlertScan),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kStimulusObserved),
                        static_cast<int>(snap.runtime.last_transition_cause));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionPresenceMode::kAttending),
                        static_cast<int>(snap.runtime.presence_mode));
}

void test_companion_state_decays_to_sleep_after_stable_idle() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));

  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 13250));
  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kSleep),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kIdleDecayToSleep),
                        static_cast<int>(snap.runtime.last_transition_cause));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionPresenceMode::kIdle),
                        static_cast<int>(snap.runtime.presence_mode));
}

void test_companion_state_keeps_attend_user_during_hold_and_then_recovers() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 70;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 2000));

  auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kAttendUser),
                        static_cast<int>(snap.runtime.product_state));

  TEST_ASSERT_TRUE(store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 2805));
  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kUserRelease),
                        static_cast<int>(snap.runtime.last_transition_cause));
}

void test_companion_state_keeps_responding_during_hold_and_then_recovers() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  interaction.session_active = true;
  interaction.response_pending = true;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1200));

  interaction.phase = ncos::core::contracts::InteractionPhase::kIdle;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kNone;
  interaction.session_active = false;
  interaction.response_pending = false;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1800));

  auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kResponding),
                        static_cast<int>(snap.runtime.product_state));

  TEST_ASSERT_TRUE(store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 2150));
  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kRecoveryToIdle),
                        static_cast<int>(snap.runtime.last_transition_cause));
}

void test_companion_state_wakes_from_sleep_on_user_trigger_and_returns_to_idle() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 13250));

  auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kSleep),
                        static_cast<int>(snap.runtime.product_state));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 72;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 13320));

  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kAttendUser),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kUserTrigger),
                        static_cast<int>(snap.runtime.last_transition_cause));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 14000));

  TEST_ASSERT_TRUE(store.ingest_runtime(runtime,
                                        ncos::core::contracts::CompanionStateWriter::kRuntimeCore,
                                        14750));
  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kUserRelease),
                        static_cast<int>(snap.runtime.last_transition_cause));
  TEST_ASSERT_EQUAL_UINT32(4, snap.runtime.state_transition_total);
}

void test_companion_state_energy_protect_preempts_active_state_and_returns_to_idle() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  interaction.session_active = true;
  interaction.response_pending = true;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1200));

  ncos::core::contracts::CompanionEnergeticSignal energetic{};
  energetic.mode = ncos::core::contracts::EnergyMode::kCritical;
  energetic.battery_percent = 10;
  energetic.external_power = false;
  TEST_ASSERT_TRUE(store.ingest_energetic(
      energetic, ncos::core::contracts::CompanionStateWriter::kPowerService, 1250));

  auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kEnergyProtect),
                        static_cast<int>(snap.runtime.product_state));

  interaction.phase = ncos::core::contracts::InteractionPhase::kIdle;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kNone;
  interaction.session_active = false;
  interaction.response_pending = false;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1300));

  energetic.mode = ncos::core::contracts::EnergyMode::kNominal;
  energetic.battery_percent = 64;
  energetic.external_power = true;
  TEST_ASSERT_TRUE(store.ingest_energetic(
      energetic, ncos::core::contracts::CompanionStateWriter::kPowerService, 1700));

  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionStateTransitionCause::kRecoveryToIdle),
                        static_cast<int>(snap.runtime.last_transition_cause));
}

void test_companion_state_keeps_short_session_memory_separate_from_instant_idle() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 78;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 2600));

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kIdle;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kNone;
  interaction.session_active = false;
  interaction.response_pending = false;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 2605));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 2805));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_TRUE(snap.session.warm);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(snap.session.anchor_target));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kTouch),
                        static_cast<int>(snap.session.anchor_channel));
  TEST_ASSERT_EQUAL_UINT16(1, snap.session.user_trigger_count);
  TEST_ASSERT_EQUAL_UINT16(0, snap.session.companion_response_count);
  TEST_ASSERT_EQUAL_UINT64(1200, snap.session.last_user_trigger_ms);
  TEST_ASSERT_GREATER_THAN_UINT64(2805, snap.session.retention_until_ms);
}

void test_companion_state_refreshes_short_session_memory_across_close_interactions() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 76;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 2700));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 2900));

  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 82;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 4300));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 5200));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 5905));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_TRUE(snap.session.warm);
  TEST_ASSERT_TRUE(snap.session.user_trigger_count >= 2);
  TEST_ASSERT_TRUE(snap.session.last_activity_ms >= 5200);
  TEST_ASSERT_EQUAL_UINT64(4300, snap.session.last_user_trigger_ms);
  TEST_ASSERT_GREATER_THAN_UINT64(5905, snap.session.retention_until_ms);
}

void test_companion_state_short_session_memory_expires_after_retention_window() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 80;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 2600));

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kIdle;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kNone;
  interaction.session_active = false;
  interaction.response_pending = false;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 2605));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 22000));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_FALSE(snap.session.warm);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kNone),
                        static_cast<int>(snap.session.anchor_target));
  TEST_ASSERT_EQUAL_UINT16(0, snap.session.user_trigger_count);
  TEST_ASSERT_EQUAL_UINT64(0, snap.session.last_user_trigger_ms);
  TEST_ASSERT_EQUAL_UINT64(0, snap.session.retention_until_ms);
}

void test_companion_state_redacts_short_session_memory_for_cloud_reader() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 84;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  const auto runtime_view = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  const auto cloud_view = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kCloudBridge);

  TEST_ASSERT_TRUE(runtime_view.session.warm);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(runtime_view.session.recent_stimulus.target));
  TEST_ASSERT_FALSE(cloud_view.session.warm);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kNone),
                        static_cast<int>(cloud_view.session.anchor_target));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kNone),
                        static_cast<int>(cloud_view.session.recent_stimulus.target));
  TEST_ASSERT_EQUAL_UINT8(0, cloud_view.session.engagement_recent_percent);
  TEST_ASSERT_EQUAL_UINT16(0, cloud_view.session.user_trigger_count);
}

void test_companion_state_redacts_personality_for_cloud_reader() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  const auto runtime_view = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  const auto cloud_view = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kCloudBridge);

  TEST_ASSERT_EQUAL_STRING("companion_core", runtime_view.personality.profile_name);
  TEST_ASSERT_EQUAL_UINT8(68, runtime_view.personality.warmth_percent);
  TEST_ASSERT_EQUAL_INT8(0, runtime_view.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_NULL(cloud_view.personality.profile_name);
  TEST_ASSERT_EQUAL_UINT8(0, cloud_view.personality.warmth_percent);
  TEST_ASSERT_EQUAL_UINT8(0, cloud_view.personality.curiosity_percent);
  TEST_ASSERT_EQUAL_INT8(0, cloud_view.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_EQUAL_INT8(0, cloud_view.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_EQUAL_INT16(0, cloud_view.personality.adaptive_continuity_window_bias_ms);
  TEST_ASSERT_EQUAL_UINT16(0, cloud_view.personality.reengagement_ttl_ms);
  TEST_ASSERT_EQUAL_UINT64(0, cloud_view.personality.user_continuity_window_ms);
}

void test_companion_state_keeps_recent_stimulus_after_alert_scan_recovers_to_idle() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kStimulus;
  attentional.channel = ncos::core::contracts::AttentionChannel::kAuditory;
  attentional.focus_confidence_percent = 67;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 3000));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 3005));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_TRUE(snap.session.warm);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kStimulus),
                        static_cast<int>(snap.session.recent_stimulus.target));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kAuditory),
                        static_cast<int>(snap.session.recent_stimulus.channel));
  TEST_ASSERT_EQUAL_UINT8(67, snap.session.recent_stimulus.confidence_percent);
  TEST_ASSERT_EQUAL_UINT64(1200, snap.session.recent_stimulus.observed_at_ms);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kAlertScan),
                        static_cast<int>(snap.session.anchor_state));
}

void test_companion_state_keeps_recent_engagement_and_interaction_context() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  interaction.session_active = true;
  interaction.response_pending = true;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 1400));

  interaction.phase = ncos::core::contracts::InteractionPhase::kIdle;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kNone;
  interaction.session_active = false;
  interaction.response_pending = false;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 2200));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 2505));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::CompanionProductState::kIdleObserve),
                        static_cast<int>(snap.runtime.product_state));
  TEST_ASSERT_TRUE(snap.session.warm);
  TEST_ASSERT_EQUAL_UINT8(72, snap.session.engagement_recent_percent);
  TEST_ASSERT_EQUAL_UINT64(1400, snap.session.last_engagement_ms);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kResponding),
                        static_cast<int>(snap.session.recent_interaction.phase));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::TurnOwner::kCompanion),
                        static_cast<int>(snap.session.recent_interaction.turn_owner));
  TEST_ASSERT_TRUE(snap.session.recent_interaction.response_pending);
  TEST_ASSERT_EQUAL_UINT64(1400, snap.session.recent_interaction.updated_at_ms);
  TEST_ASSERT_EQUAL_UINT16(1, snap.session.companion_response_count);
}

void test_companion_state_bounds_adaptation_during_hot_user_session() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 84;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_GREATER_THAN_INT8(0, snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_GREATER_THAN_INT8(0, snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_GREATER_THAN_INT16(0, snap.personality.adaptive_continuity_window_bias_ms);
  TEST_ASSERT_LESS_OR_EQUAL_INT8(
      ncos::core::contracts::personality_adaptive_social_warmth_bias_max_percent(),
      snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_LESS_OR_EQUAL_INT8(
      ncos::core::contracts::personality_adaptive_response_energy_bias_max_percent(),
      snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_LESS_OR_EQUAL_INT16(
      ncos::core::contracts::personality_adaptive_continuity_window_bias_max_ms(),
      snap.personality.adaptive_continuity_window_bias_ms);
}

void test_companion_state_adaptation_decays_back_to_neutral_after_context_cools() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attentional{};
  attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attentional.focus_confidence_percent = 84;
  attentional.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  attentional.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attentional.focus_confidence_percent = 0;
  attentional.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attentional, ncos::core::contracts::CompanionStateWriter::kAttentionService, 2600));

  ncos::core::contracts::CompanionInteractionSignal interaction{};
  interaction.phase = ncos::core::contracts::InteractionPhase::kIdle;
  interaction.turn_owner = ncos::core::contracts::TurnOwner::kNone;
  interaction.session_active = false;
  interaction.response_pending = false;
  TEST_ASSERT_TRUE(store.ingest_interactional(
      interaction, ncos::core::contracts::CompanionStateWriter::kInteractionService, 2605));

  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 7000));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 7200));
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 7400));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_EQUAL_INT8(0, snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_EQUAL_INT8(0, snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_EQUAL_INT16(0, snap.personality.adaptive_continuity_window_bias_ms);
}

void test_companion_state_adaptation_distinguishes_user_and_stimulus_profiles() {
  ncos::core::state::CompanionStateStore user_store;
  TEST_ASSERT_TRUE(user_store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      user_store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal user_attention{};
  user_attention.target = ncos::core::contracts::AttentionTarget::kUser;
  user_attention.channel = ncos::core::contracts::AttentionChannel::kTouch;
  user_attention.focus_confidence_percent = 84;
  user_attention.lock_active = true;
  TEST_ASSERT_TRUE(user_store.ingest_attentional(
      user_attention, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  const auto user_snap = user_store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  ncos::core::state::CompanionStateStore stimulus_store;
  TEST_ASSERT_TRUE(stimulus_store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));
  TEST_ASSERT_TRUE(stimulus_store.ingest_runtime(
      runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal stimulus_attention{};
  stimulus_attention.target = ncos::core::contracts::AttentionTarget::kStimulus;
  stimulus_attention.channel = ncos::core::contracts::AttentionChannel::kVisual;
  stimulus_attention.focus_confidence_percent = 84;
  stimulus_attention.lock_active = false;
  TEST_ASSERT_TRUE(stimulus_store.ingest_attentional(
      stimulus_attention, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  const auto stimulus_snap =
      stimulus_store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);

  TEST_ASSERT_GREATER_THAN_INT8(stimulus_snap.personality.adaptive_social_warmth_bias_percent,
                                user_snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_GREATER_THAN_INT8(0, user_snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_EQUAL_INT8(0, stimulus_snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_GREATER_THAN_INT8(0, stimulus_snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_GREATER_THAN_INT16(0, stimulus_snap.personality.adaptive_continuity_window_bias_ms);
}

void test_companion_state_adaptation_switches_profiles_without_overshoot() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  attention.target = ncos::core::contracts::AttentionTarget::kUser;
  attention.channel = ncos::core::contracts::AttentionChannel::kTouch;
  attention.focus_confidence_percent = 84;
  attention.lock_active = true;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attention, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1200));

  auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  const int8_t social_before = snap.personality.adaptive_social_warmth_bias_percent;
  const int8_t response_before = snap.personality.adaptive_response_energy_bias_percent;
  const int16_t continuity_before = snap.personality.adaptive_continuity_window_bias_ms;

  attention.target = ncos::core::contracts::AttentionTarget::kStimulus;
  attention.channel = ncos::core::contracts::AttentionChannel::kVisual;
  attention.focus_confidence_percent = 82;
  attention.lock_active = false;
  TEST_ASSERT_TRUE(store.ingest_attentional(
      attention, ncos::core::contracts::CompanionStateWriter::kAttentionService, 1400));

  snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_GREATER_OR_EQUAL_INT8(static_cast<int8_t>(social_before - 2),
                                    snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_LESS_OR_EQUAL_INT8(static_cast<int8_t>(response_before + 2),
                                 snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_LESS_OR_EQUAL_INT16(static_cast<int16_t>(continuity_before + 120),
                                  snap.personality.adaptive_continuity_window_bias_ms);
  TEST_ASSERT_GREATER_OR_EQUAL_INT16(static_cast<int16_t>(continuity_before - 120),
                                     snap.personality.adaptive_continuity_window_bias_ms);
}
void test_companion_state_energy_protect_forces_conservative_adaptation() {
  ncos::core::state::CompanionStateStore store;
  TEST_ASSERT_TRUE(store.initialize({}, ncos::core::contracts::CompanionStateWriter::kBootstrap, 1000));

  ncos::core::contracts::CompanionRuntimeSignal runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.scheduler_tasks = 2;
  TEST_ASSERT_TRUE(
      store.ingest_runtime(runtime, ncos::core::contracts::CompanionStateWriter::kRuntimeCore, 1100));

  ncos::core::contracts::CompanionEnergeticSignal energetic{};
  energetic.mode = ncos::core::contracts::EnergyMode::kCritical;
  energetic.battery_percent = 8;
  energetic.thermal_load_percent = 55;
  energetic.external_power = false;
  TEST_ASSERT_TRUE(
      store.ingest_energetic(energetic, ncos::core::contracts::CompanionStateWriter::kPowerService,
                             1300));

  const auto snap = store.snapshot_for(ncos::core::contracts::CompanionStateReader::kRuntimeCore);
  TEST_ASSERT_LESS_THAN_INT8(0, snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_LESS_THAN_INT8(0, snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_LESS_THAN_INT16(0, snap.personality.adaptive_continuity_window_bias_ms);
  TEST_ASSERT_GREATER_OR_EQUAL_INT8(
      ncos::core::contracts::personality_adaptive_social_warmth_bias_min_percent(),
      snap.personality.adaptive_social_warmth_bias_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_INT8(
      ncos::core::contracts::personality_adaptive_response_energy_bias_min_percent(),
      snap.personality.adaptive_response_energy_bias_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_INT16(
      ncos::core::contracts::personality_adaptive_continuity_window_bias_min_ms(),
      snap.personality.adaptive_continuity_window_bias_ms);
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
  RUN_TEST(test_companion_state_exposes_personality_from_central_snapshot);
  RUN_TEST(test_companion_state_rejects_unauthorized_write);
  RUN_TEST(test_companion_state_updates_emotional_domain);
  RUN_TEST(test_companion_state_normalizes_authoritative_emotion_vector);
  RUN_TEST(test_companion_state_updates_attentional_and_interactional_link);
  RUN_TEST(test_companion_state_updates_energetic_domain);
  RUN_TEST(test_companion_state_runtime_safe_mode_constrains_interaction_and_energy);
  RUN_TEST(test_companion_state_tracks_transient_governance_transition);
  RUN_TEST(test_companion_state_tracks_boot_idle_responding_and_recovery);
  RUN_TEST(test_companion_state_marks_alert_scan_from_world_attention);
  RUN_TEST(test_companion_state_decays_to_sleep_after_stable_idle);
  RUN_TEST(test_companion_state_keeps_attend_user_during_hold_and_then_recovers);
  RUN_TEST(test_companion_state_keeps_responding_during_hold_and_then_recovers);
  RUN_TEST(test_companion_state_wakes_from_sleep_on_user_trigger_and_returns_to_idle);
  RUN_TEST(test_companion_state_energy_protect_preempts_active_state_and_returns_to_idle);
  RUN_TEST(test_companion_state_keeps_short_session_memory_separate_from_instant_idle);
  RUN_TEST(test_companion_state_refreshes_short_session_memory_across_close_interactions);
  RUN_TEST(test_companion_state_short_session_memory_expires_after_retention_window);
  RUN_TEST(test_companion_state_keeps_recent_stimulus_after_alert_scan_recovers_to_idle);
  RUN_TEST(test_companion_state_keeps_recent_engagement_and_interaction_context);
  RUN_TEST(test_companion_state_redacts_short_session_memory_for_cloud_reader);
  RUN_TEST(test_companion_state_redacts_personality_for_cloud_reader);
  RUN_TEST(test_companion_state_bounds_adaptation_during_hot_user_session);
  RUN_TEST(test_companion_state_adaptation_decays_back_to_neutral_after_context_cools);
  RUN_TEST(test_companion_state_adaptation_distinguishes_user_and_stimulus_profiles);
  RUN_TEST(test_companion_state_adaptation_switches_profiles_without_overshoot);
  RUN_TEST(test_companion_state_energy_protect_forces_conservative_adaptation);
  RUN_TEST(test_companion_state_redacts_by_reader_profile);
  return UNITY_END();
}
