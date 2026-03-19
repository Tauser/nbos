#include <unity.h>

#include "services/behavior/behavior_service.hpp"
#include "services/vision/perception_service.hpp"
#include "core/contracts/face_multimodal_contracts.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/action_governance_contracts.cpp"
#include "core/contracts/companion_state_contracts.cpp"
#include "core/contracts/face_multimodal_contracts.cpp"
#include "services/behavior/behavior_service.cpp"
#include "services/vision/perception_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_companion_mvp_slice_uses_touch_as_official_trigger() {
  ncos::services::vision::PerceptionService perception;
  ncos::services::behavior::BehaviorService behavior;

  TEST_ASSERT_TRUE(perception.initialize(65, 1000));
  TEST_ASSERT_TRUE(behavior.initialize(61, 1000));

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::TouchRuntimeState touch{};
  touch.initialized = true;
  touch.last_read_ok = true;
  touch.normalized_level = 780;

  ncos::core::contracts::CameraRuntimeState camera{};
  ncos::core::contracts::CompanionSnapshot companion{};

  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_TRUE(perception.tick(audio, touch, camera, companion, 1120, &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kUser),
                        static_cast<int>(attention.target));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionChannel::kTouch),
                        static_cast<int>(attention.channel));
  TEST_ASSERT_TRUE(attention.lock_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kActing),
                        static_cast<int>(interaction.phase));

  companion.attentional.target = attention.target;
  companion.attentional.channel = attention.channel;
  companion.attentional.focus_confidence_percent = attention.focus_confidence_percent;
  companion.attentional.lock_active = attention.lock_active;
  companion.interactional.phase = interaction.phase;
  companion.interactional.turn_owner = interaction.turn_owner;
  companion.interactional.session_active = interaction.session_active;
  companion.interactional.response_pending = interaction.response_pending;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(behavior.tick(companion, 1320, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::BehaviorProfile::kAttendUser),
                        static_cast<int>(proposal.profile));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::ActionDomain::kFace),
                        static_cast<int>(proposal.proposal.domain));

  ncos::core::contracts::GovernanceDecision allow{};
  allow.kind = ncos::core::contracts::GovernanceDecisionKind::kAllow;
  behavior.on_governance_decision(allow, 1330);

  ncos::core::contracts::ImuRuntimeState imu{};
  const auto face_input =
      ncos::core::contracts::make_face_multimodal_input(audio, touch, imu, companion, behavior.state(), 1360);
  TEST_ASSERT_TRUE(face_input.touch_active);
  TEST_ASSERT_TRUE(face_input.behavior_active);
  TEST_ASSERT_EQUAL_UINT8(78, face_input.touch_intensity_percent);
  TEST_ASSERT_EQUAL_UINT8(58, face_input.behavior_activation_percent);
}

void test_companion_mvp_slice_returns_to_idle_without_new_stimulus() {
  ncos::services::vision::PerceptionService perception;
  ncos::services::behavior::BehaviorService behavior;

  TEST_ASSERT_TRUE(perception.initialize(65, 2000));
  TEST_ASSERT_TRUE(behavior.initialize(61, 2000));

  ncos::core::contracts::CompanionSnapshot companion{};
  companion.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  companion.attentional.channel = ncos::core::contracts::AttentionChannel::kTouch;
  companion.attentional.focus_confidence_percent = 78;
  companion.attentional.lock_active = true;
  companion.interactional.phase = ncos::core::contracts::InteractionPhase::kActing;
  companion.interactional.turn_owner = ncos::core::contracts::TurnOwner::kUser;
  companion.interactional.session_active = true;

  ncos::core::contracts::BehaviorProposal proposal{};
  TEST_ASSERT_TRUE(behavior.tick(companion, 2300, &proposal));
  TEST_ASSERT_TRUE(proposal.valid);

  ncos::core::contracts::GovernanceDecision allow{};
  allow.kind = ncos::core::contracts::GovernanceDecisionKind::kAllow;
  behavior.on_governance_decision(allow, 2310);

  ncos::core::contracts::AudioRuntimeState audio{};
  ncos::core::contracts::TouchRuntimeState touch{};
  touch.initialized = true;
  touch.last_read_ok = true;
  touch.normalized_level = 0;

  ncos::core::contracts::CameraRuntimeState camera{};
  ncos::core::contracts::CompanionAttentionalSignal attention{};
  ncos::core::contracts::CompanionInteractionSignal interaction{};

  TEST_ASSERT_FALSE(perception.tick(audio, touch, camera, ncos::core::contracts::CompanionSnapshot{}, 2600,
                                    &attention, &interaction));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::PerceptionStage::Dormant),
                        static_cast<int>(perception.state().stage));
  TEST_ASSERT_FALSE(perception.state().presence_active);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::AttentionTarget::kNone),
                        static_cast<int>(attention.target));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::InteractionPhase::kIdle),
                        static_cast<int>(interaction.phase));

  ncos::core::contracts::BehaviorProposal idle_proposal{};
  TEST_ASSERT_FALSE(behavior.tick(ncos::core::contracts::CompanionSnapshot{}, 2600, &idle_proposal));
  TEST_ASSERT_FALSE(idle_proposal.valid);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_companion_mvp_slice_uses_touch_as_official_trigger);
  RUN_TEST(test_companion_mvp_slice_returns_to_idle_without_new_stimulus);
  return UNITY_END();
}
