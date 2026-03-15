#include <unity.h>

#include "services/face/face_compositor.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_compositor.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_compositor_grants_initial_ownership_and_hold() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  TEST_ASSERT_TRUE(compositor.bind_state(&state));

  ncos::services::face::FaceLayerRequest request{};
  request.layer = ncos::models::face::FaceLayer::kGaze;
  request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  request.requester_service = 32;
  request.priority = 5;
  request.hold_ms = 300;

  const auto decision = compositor.request_layer(request, 1000);
  TEST_ASSERT_TRUE(decision.granted);
  TEST_ASSERT_EQUAL_UINT16(32, decision.active_owner_service);
  TEST_ASSERT_EQUAL_UINT64(1300, decision.hold_until_ms);
  TEST_ASSERT_TRUE(compositor.can_write(ncos::models::face::FaceLayer::kGaze, 32));
}

void test_face_compositor_blocks_equal_priority_takeover_during_hold() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  TEST_ASSERT_TRUE(compositor.bind_state(&state));

  ncos::services::face::FaceLayerRequest owner{};
  owner.layer = ncos::models::face::FaceLayer::kTransient;
  owner.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kTransientOwner;
  owner.requester_service = 41;
  owner.priority = 7;
  owner.hold_ms = 400;
  TEST_ASSERT_TRUE(compositor.request_layer(owner, 2000).granted);

  auto contender = owner;
  contender.requester_service = 51;
  contender.priority = 7;

  const auto decision = compositor.request_layer(contender, 2200);
  TEST_ASSERT_FALSE(decision.granted);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::services::face::FaceComposeRejectReason::kHoldActive),
                        static_cast<int>(decision.reason));
}

void test_face_compositor_preempts_with_higher_priority_and_applies_cooldown() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  TEST_ASSERT_TRUE(compositor.bind_state(&state));

  ncos::services::face::FaceLayerRequest first{};
  first.layer = ncos::models::face::FaceLayer::kGaze;
  first.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  first.requester_service = 32;
  first.priority = 5;
  first.hold_ms = 500;
  first.cooldown_ms = 220;
  TEST_ASSERT_TRUE(compositor.request_layer(first, 3000).granted);

  auto preempt = first;
  preempt.requester_service = 48;
  preempt.priority = 8;

  const auto preempt_decision = compositor.request_layer(preempt, 3200);
  TEST_ASSERT_TRUE(preempt_decision.granted);
  TEST_ASSERT_EQUAL_UINT16(48, preempt_decision.active_owner_service);
  TEST_ASSERT_TRUE(preempt_decision.cooldown_until_ms >= 3420);

  const auto blocked_decision = compositor.request_layer(first, 3300);
  TEST_ASSERT_FALSE(blocked_decision.granted);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::services::face::FaceComposeRejectReason::kCooldownActive),
                        static_cast<int>(blocked_decision.reason));
}

void test_face_compositor_allows_parallel_ownership_across_layers() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  TEST_ASSERT_TRUE(compositor.bind_state(&state));

  ncos::services::face::FaceLayerRequest base{};
  base.layer = ncos::models::face::FaceLayer::kBase;
  base.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  base.requester_service = 31;
  base.priority = 3;

  ncos::services::face::FaceLayerRequest gaze{};
  gaze.layer = ncos::models::face::FaceLayer::kGaze;
  gaze.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze.requester_service = 32;
  gaze.priority = 5;

  TEST_ASSERT_TRUE(compositor.request_layer(base, 4000).granted);
  TEST_ASSERT_TRUE(compositor.request_layer(gaze, 4010).granted);
  TEST_ASSERT_TRUE(compositor.can_write(ncos::models::face::FaceLayer::kBase, 31));
  TEST_ASSERT_TRUE(compositor.can_write(ncos::models::face::FaceLayer::kGaze, 32));
}

void test_face_compositor_rejects_wrong_role_for_layer() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  TEST_ASSERT_TRUE(compositor.bind_state(&state));

  ncos::services::face::FaceLayerRequest invalid{};
  invalid.layer = ncos::models::face::FaceLayer::kClip;
  invalid.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBlinkOwner;
  invalid.requester_service = 77;
  invalid.priority = 9;

  const auto decision = compositor.request_layer(invalid, 5000);
  TEST_ASSERT_FALSE(decision.granted);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::services::face::FaceComposeRejectReason::kOwnershipMismatch),
                        static_cast<int>(decision.reason));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_compositor_grants_initial_ownership_and_hold);
  RUN_TEST(test_face_compositor_blocks_equal_priority_takeover_during_hold);
  RUN_TEST(test_face_compositor_preempts_with_higher_priority_and_applies_cooldown);
  RUN_TEST(test_face_compositor_allows_parallel_ownership_across_layers);
  RUN_TEST(test_face_compositor_rejects_wrong_role_for_layer);
  return UNITY_END();
}
