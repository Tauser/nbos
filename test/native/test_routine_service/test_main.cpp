#include <unity.h>

#include "services/routine/routine_service.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "services/routine/routine_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_routine_service_emits_ambient_idle_routine() {
  ncos::services::routine::RoutineService service;
  TEST_ASSERT_TRUE(service.initialize(62, 1000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  ncos::core::contracts::BehaviorRuntimeState behavior_state{};
  behavior_state.initialized = true;

  ncos::core::contracts::RoutineProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior_state, 2100, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IdleRoutine::kAmbientGazeSweep),
                        static_cast<int>(proposal.routine));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionMode::kAmbient),
                        static_cast<int>(proposal.attention_mode));
}

void test_routine_service_switches_to_user_attention_mode() {
  ncos::services::routine::RoutineService service;
  TEST_ASSERT_TRUE(service.initialize(62, 2000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.attentional.focus_confidence_percent = 80;

  ncos::core::contracts::BehaviorRuntimeState behavior_state{};
  behavior_state.initialized = true;

  ncos::core::contracts::RoutineProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior_state, 3100, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IdleRoutine::kUserPresencePulse),
                        static_cast<int>(proposal.routine));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionMode::kUserEngaged),
                        static_cast<int>(proposal.attention_mode));
}

void test_routine_service_suppresses_when_behavior_recently_owned_action() {
  ncos::services::routine::RoutineService service;
  TEST_ASSERT_TRUE(service.initialize(62, 3000));

  ncos::core::contracts::CompanionSnapshot snapshot{};

  ncos::core::contracts::BehaviorRuntimeState behavior_state{};
  behavior_state.initialized = true;
  behavior_state.active_profile = ncos::core::contracts::BehaviorProfile::kAlertScan;
  behavior_state.last_accept_ms = 3400;

  ncos::core::contracts::RoutineProposal proposal{};
  TEST_ASSERT_FALSE(service.tick(snapshot, behavior_state, 3500, &proposal));
  TEST_ASSERT_FALSE(proposal.valid);
}

void test_routine_service_tracks_governance_results() {
  ncos::services::routine::RoutineService service;
  TEST_ASSERT_TRUE(service.initialize(62, 4000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  ncos::core::contracts::BehaviorRuntimeState behavior_state{};
  behavior_state.initialized = true;

  ncos::core::contracts::RoutineProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior_state, 5100, &proposal));

  ncos::core::contracts::GovernanceDecision allow{};
  allow.kind = ncos::core::contracts::GovernanceDecisionKind::kAllow;
  service.on_governance_decision(allow, 5110);

  ncos::core::contracts::GovernanceDecision reject{};
  reject.kind = ncos::core::contracts::GovernanceDecisionKind::kReject;
  service.on_governance_decision(reject, 5120);

  const auto& state = service.state();
  TEST_ASSERT_EQUAL_UINT32(1, state.accepted_total);
  TEST_ASSERT_EQUAL_UINT32(1, state.rejected_total);
}

void test_routine_service_uses_persistent_user_history_for_ambient_initiative() {
  ncos::services::routine::RoutineService service;
  TEST_ASSERT_TRUE(service.initialize(62, 5200));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.personality = ncos::core::contracts::make_companion_personality_state();
  snapshot.personality.persistent_memory_applied = true;
  snapshot.personality.persistent_social_warmth_bias_percent = 5;
  snapshot.personality.persistent_continuity_window_bias_ms = 240;
  snapshot.personality.persistent_reinforced_sessions = 6;
  snapshot.personality.persistent_preferred_attention_channel =
      ncos::core::contracts::AttentionChannel::kTouch;
  snapshot.personality.adaptive_social_warmth_bias_percent = 5;
  snapshot.personality.adaptive_continuity_window_bias_ms = 240;

  ncos::core::contracts::BehaviorRuntimeState behavior_state{};
  behavior_state.initialized = true;

  ncos::core::contracts::RoutineProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior_state, 6300, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IdleRoutine::kUserPresencePulse),
                        static_cast<int>(proposal.routine));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionMode::kAmbient),
                        static_cast<int>(proposal.attention_mode));
  TEST_ASSERT_EQUAL_UINT8(4, proposal.proposal.priority);
  TEST_ASSERT_EQUAL_UINT32(336, proposal.proposal.ttl_ms);
  TEST_ASSERT_EQUAL_STRING("idle_user_affinity_pulse", proposal.rationale);
}

void test_routine_service_uses_persistent_stimulus_history_for_curious_ambient_scan() {
  ncos::services::routine::RoutineService service;
  TEST_ASSERT_TRUE(service.initialize(62, 6400));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.personality = ncos::core::contracts::make_companion_personality_state();
  snapshot.personality.persistent_memory_applied = true;
  snapshot.personality.persistent_response_energy_bias_percent = 4;
  snapshot.personality.persistent_preferred_attention_channel =
      ncos::core::contracts::AttentionChannel::kAuditory;
  snapshot.personality.adaptive_response_energy_bias_percent = 4;
  snapshot.emotional.tone = ncos::core::contracts::EmotionalTone::kCurious;

  ncos::core::contracts::BehaviorRuntimeState behavior_state{};
  behavior_state.initialized = true;

  ncos::core::contracts::RoutineProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior_state, 7500, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IdleRoutine::kStimulusScanNudge),
                        static_cast<int>(proposal.routine));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionMode::kAmbient),
                        static_cast<int>(proposal.attention_mode));
  TEST_ASSERT_EQUAL_UINT8(4, proposal.proposal.priority);
  TEST_ASSERT_EQUAL_UINT32(304, proposal.proposal.ttl_ms);
  TEST_ASSERT_EQUAL_STRING("idle_stimulus_affinity", proposal.rationale);
}

void test_routine_service_shortens_initiative_cooldown_when_user_history_is_reinforced() {
  ncos::services::routine::RoutineService service;
  TEST_ASSERT_TRUE(service.initialize(62, 7600));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.personality = ncos::core::contracts::make_companion_personality_state();
  snapshot.personality.persistent_memory_applied = true;
  snapshot.personality.persistent_social_warmth_bias_percent = 5;
  snapshot.personality.persistent_continuity_window_bias_ms = 240;
  snapshot.personality.persistent_reinforced_sessions = 6;
  snapshot.personality.persistent_preferred_attention_channel =
      ncos::core::contracts::AttentionChannel::kTouch;
  snapshot.personality.adaptive_social_warmth_bias_percent = 5;
  snapshot.personality.adaptive_continuity_window_bias_ms = 240;

  ncos::core::contracts::BehaviorRuntimeState behavior_state{};
  behavior_state.initialized = true;

  ncos::core::contracts::RoutineProposal first{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior_state, 8700, &first));
  TEST_ASSERT_TRUE(first.valid);

  ncos::core::contracts::RoutineProposal second{};
  TEST_ASSERT_TRUE(service.tick(snapshot, behavior_state, 9470, &second));
  TEST_ASSERT_TRUE(second.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::IdleRoutine::kUserPresencePulse),
                        static_cast<int>(second.routine));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_routine_service_emits_ambient_idle_routine);
  RUN_TEST(test_routine_service_switches_to_user_attention_mode);
  RUN_TEST(test_routine_service_suppresses_when_behavior_recently_owned_action);
  RUN_TEST(test_routine_service_tracks_governance_results);
  RUN_TEST(test_routine_service_uses_persistent_user_history_for_ambient_initiative);
  RUN_TEST(test_routine_service_uses_persistent_stimulus_history_for_curious_ambient_scan);
  RUN_TEST(test_routine_service_shortens_initiative_cooldown_when_user_history_is_reinforced);
  return UNITY_END();
}

