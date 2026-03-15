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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_routine_service_emits_ambient_idle_routine);
  RUN_TEST(test_routine_service_switches_to_user_attention_mode);
  RUN_TEST(test_routine_service_suppresses_when_behavior_recently_owned_action);
  RUN_TEST(test_routine_service_tracks_governance_results);
  return UNITY_END();
}
