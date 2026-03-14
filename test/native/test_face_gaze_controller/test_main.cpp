#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_gaze_controller.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_gaze_controller.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

constexpr uint16_t kOwnerService = 31;

ncos::core::contracts::FaceRenderState make_state_with_gaze_owner() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();

  ncos::core::contracts::FaceLayerClaim gaze_claim{};
  gaze_claim.layer = ncos::models::face::FaceLayer::kGaze;
  gaze_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze_claim.requester_service = kOwnerService;
  gaze_claim.priority = 5;

  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, gaze_claim, 100));
  return state;
}

}  // namespace

void test_face_gaze_controller_applies_target_when_ownership_is_valid() {
  auto state = make_state_with_gaze_owner();
  ncos::services::face::FaceGazeController controller{kOwnerService};

  ncos::models::face::FaceGazeTarget target{};
  target.anchor = ncos::models::face::GazeAnchor::kUser;
  target.direction = ncos::models::face::GazeDirection::kUpRight;
  target.focus_percent = 70;
  target.hold_ms = 500;

  TEST_ASSERT_TRUE(controller.set_target(target, 1000));
  TEST_ASSERT_TRUE(controller.tick(1100, &state));

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeAnchor::kUser),
                        static_cast<int>(state.eyes.anchor));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeDirection::kUpRight),
                        static_cast<int>(state.eyes.direction));
  TEST_ASSERT_EQUAL_UINT8(70, state.eyes.focus_percent);
}

void test_face_gaze_controller_refuses_write_without_gaze_ownership() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceGazeController controller{kOwnerService};

  ncos::models::face::FaceGazeTarget target{};
  target.direction = ncos::models::face::GazeDirection::kLeft;
  target.focus_percent = 60;
  target.hold_ms = 300;

  TEST_ASSERT_TRUE(controller.set_target(target, 1000));
  TEST_ASSERT_FALSE(controller.tick(1050, &state));

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeDirection::kCenter),
                        static_cast<int>(state.eyes.direction));
}

void test_face_gaze_controller_expires_target_by_hold_window() {
  auto state = make_state_with_gaze_owner();
  ncos::services::face::FaceGazeController controller{kOwnerService};

  ncos::models::face::FaceGazeTarget target{};
  target.direction = ncos::models::face::GazeDirection::kDown;
  target.focus_percent = 55;
  target.hold_ms = 100;

  TEST_ASSERT_TRUE(controller.set_target(target, 1000));
  TEST_ASSERT_TRUE(controller.tick(1080, &state));
  TEST_ASSERT_FALSE(controller.tick(1201, &state));
}

void test_face_gaze_controller_rejects_zero_hold_target() {
  ncos::services::face::FaceGazeController controller{kOwnerService};
  ncos::models::face::FaceGazeTarget target{};
  target.direction = ncos::models::face::GazeDirection::kRight;
  target.hold_ms = 0;

  TEST_ASSERT_FALSE(controller.set_target(target, 1000));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_gaze_controller_applies_target_when_ownership_is_valid);
  RUN_TEST(test_face_gaze_controller_refuses_write_without_gaze_ownership);
  RUN_TEST(test_face_gaze_controller_expires_target_by_hold_window);
  RUN_TEST(test_face_gaze_controller_rejects_zero_hold_target);
  return UNITY_END();
}
