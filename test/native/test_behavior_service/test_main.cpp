#include <unity.h>

#include "services/behavior/behavior_service.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "services/behavior/behavior_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_behavior_service_emits_energy_protect_when_critical() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 1000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.energetic.mode = ncos::core::contracts::EnergyMode::kCritical;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, 1300, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kEnergyProtect),
                        static_cast<int>(proposal.profile));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::ActionDomain::kPower),
                        static_cast<int>(proposal.proposal.domain));
  TEST_ASSERT_EQUAL_UINT8(10, proposal.proposal.priority);
  TEST_ASSERT_EQUAL_UINT32(420, proposal.proposal.ttl_ms);
}

void test_behavior_service_prefers_alert_scan_over_attend_user() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 2000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.attentional.focus_confidence_percent = 90;
  snapshot.emotional.phase = ncos::models::emotion::EmotionPhase::kAlerting;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, 2300, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kAlertScan),
                        static_cast<int>(proposal.profile));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::ActionDomain::kMotion),
                        static_cast<int>(proposal.proposal.domain));
  TEST_ASSERT_EQUAL_UINT32(220, proposal.proposal.ttl_ms);
}

void test_behavior_service_respects_cooldown() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 3000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.attentional.focus_confidence_percent = 80;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, 3300, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);

  ncos::core::contracts::BehaviorProposal second{};
  TEST_ASSERT_FALSE(service.tick(snapshot, 3400, &second));
  TEST_ASSERT_FALSE(second.valid);
}

void test_behavior_service_tracks_governance_preemption_and_rejection() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 4000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.attentional.focus_confidence_percent = 80;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, 4300, &proposal));

  ncos::core::contracts::GovernanceDecision preempt{};
  preempt.kind = ncos::core::contracts::GovernanceDecisionKind::kPreemptAndAllow;
  service.on_governance_decision(preempt, 4310);

  ncos::core::contracts::GovernanceDecision reject{};
  reject.kind = ncos::core::contracts::GovernanceDecisionKind::kReject;
  service.on_governance_decision(reject, 4320);

  const auto& state = service.state();
  TEST_ASSERT_EQUAL_UINT32(1, state.accepted_total);
  TEST_ASSERT_EQUAL_UINT32(1, state.preempted_total);
  TEST_ASSERT_EQUAL_UINT32(1, state.rejected_total);
}

void test_behavior_service_raises_attend_priority_on_voice_trigger_context() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 5000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.attentional.channel = ncos::core::contracts::AttentionChannel::kAuditory;
  snapshot.attentional.focus_confidence_percent = 70;
  snapshot.interactional.response_pending = true;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, 5300, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kAttendUser),
                        static_cast<int>(proposal.profile));
  TEST_ASSERT_EQUAL_UINT8(7, proposal.proposal.priority);
  TEST_ASSERT_EQUAL_UINT32(220, proposal.proposal.ttl_ms);
}

void test_behavior_service_uses_short_context_to_avoid_cold_reengagement() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 5600));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.runtime.product_state = ncos::core::contracts::CompanionProductState::kIdleObserve;
  snapshot.session.warm = true;
  snapshot.session.last_activity_ms = 5900;
  snapshot.session.anchor_target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.session.recent_stimulus.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.session.recent_interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  snapshot.session.recent_interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  snapshot.session.engagement_recent_percent = 72;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, 6100, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kAttendUser),
                        static_cast<int>(proposal.profile));
  TEST_ASSERT_EQUAL_UINT8(5, proposal.proposal.priority);
  TEST_ASSERT_EQUAL_UINT32(190, proposal.proposal.ttl_ms);
  TEST_ASSERT_EQUAL_STRING("attend_user_continuity", proposal.rationale);
}

void test_behavior_service_requires_identity_continuity_threshold_for_warm_reengagement() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 6400));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.runtime.product_state = ncos::core::contracts::CompanionProductState::kIdleObserve;
  snapshot.session.warm = true;
  snapshot.session.last_activity_ms = 6600;
  snapshot.session.anchor_target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.session.recent_stimulus.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.session.recent_interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  snapshot.session.recent_interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  snapshot.session.engagement_recent_percent = 55;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_FALSE(service.tick(snapshot, 6800, &proposal));
  TEST_ASSERT_FALSE(proposal.valid);
}

void test_behavior_service_does_not_hold_warm_reengagement_after_window() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 7000));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.runtime.product_state = ncos::core::contracts::CompanionProductState::kIdleObserve;
  snapshot.session.warm = true;
  snapshot.session.last_activity_ms = 7100;
  snapshot.session.anchor_target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.session.recent_stimulus.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.session.recent_interaction.phase = ncos::core::contracts::InteractionPhase::kResponding;
  snapshot.session.recent_interaction.turn_owner = ncos::core::contracts::TurnOwner::kCompanion;
  snapshot.session.engagement_recent_percent = 72;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_FALSE(service.tick(snapshot, 10350, &proposal));
  TEST_ASSERT_FALSE(proposal.valid);
}

void test_behavior_service_reflects_adaptation_in_attend_priority_and_ttl() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 7400));

  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.attentional.focus_confidence_percent = 72;
  snapshot.personality = ncos::core::contracts::make_companion_personality_state();
  snapshot.personality.adaptive_social_warmth_bias_percent = 8;
  snapshot.personality.adaptive_response_energy_bias_percent = 6;
  snapshot.personality.adaptive_continuity_window_bias_ms = 400;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(snapshot, 7700, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kAttendUser),
                        static_cast<int>(proposal.profile));
  TEST_ASSERT_EQUAL_UINT8(8, proposal.proposal.priority);
  TEST_ASSERT_EQUAL_UINT32(288, proposal.proposal.ttl_ms);
}

void test_behavior_service_returns_to_idle_profile_when_slice_goes_idle() {
  ncos::services::behavior::BehaviorService service;
  TEST_ASSERT_TRUE(service.initialize(61, 6000));

  ncos::core::contracts::CompanionSnapshot attend_snapshot{};
  attend_snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  attend_snapshot.attentional.focus_confidence_percent = 80;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(service.tick(attend_snapshot, 6300, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);

  ncos::core::contracts::GovernanceDecision allow{};
  allow.kind = ncos::core::contracts::GovernanceDecisionKind::kAllow;
  service.on_governance_decision(allow, 6310);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kAttendUser),
                        static_cast<int>(service.state().active_profile));

  ncos::core::contracts::CompanionSnapshot idle_snapshot{};
  idle_snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kNone;
  idle_snapshot.interactional.phase = ncos::core::contracts::InteractionPhase::kIdle;
  idle_snapshot.interactional.session_active = false;

  ncos::core::contracts::BehaviorProposal idle_proposal{};
  TEST_ASSERT_FALSE(service.tick(idle_snapshot, 6600, &idle_proposal));
  TEST_ASSERT_FALSE(idle_proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kIdleObserve),
                        static_cast<int>(service.state().active_profile));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_behavior_service_emits_energy_protect_when_critical);
  RUN_TEST(test_behavior_service_prefers_alert_scan_over_attend_user);
  RUN_TEST(test_behavior_service_respects_cooldown);
  RUN_TEST(test_behavior_service_tracks_governance_preemption_and_rejection);
  RUN_TEST(test_behavior_service_raises_attend_priority_on_voice_trigger_context);
  RUN_TEST(test_behavior_service_uses_short_context_to_avoid_cold_reengagement);
  RUN_TEST(test_behavior_service_requires_identity_continuity_threshold_for_warm_reengagement);
  RUN_TEST(test_behavior_service_does_not_hold_warm_reengagement_after_window);
  RUN_TEST(test_behavior_service_reflects_adaptation_in_attend_priority_and_ttl);
  RUN_TEST(test_behavior_service_returns_to_idle_profile_when_slice_goes_idle);
  return UNITY_END();
}




