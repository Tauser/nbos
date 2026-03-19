#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_motion_safety.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_motion_safety.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_motion_safety_limits_diagonal_focus_progressively() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.safety_mode = ncos::core::contracts::FaceRenderSafetyMode::kNominal;
  state.eyes.direction = ncos::models::face::GazeDirection::kCenter;
  state.eyes.focus_percent = 30;

  ncos::services::face::FaceMotionSafetyStatus status{};
  ncos::services::face::reset_face_motion_safety_status(&status, state, 1000);

  state.eyes.direction = ncos::models::face::GazeDirection::kUpRight;
  state.eyes.focus_percent = 80;

  ncos::services::face::FaceMotionSafetyResult result{};
  TEST_ASSERT_TRUE(ncos::services::face::apply_face_motion_safety(&state, &status, 1120, &result));
  TEST_ASSERT_TRUE(result.active);
  TEST_ASSERT_TRUE(result.limited_diagonal);
  TEST_ASSERT_TRUE(result.limited_velocity);
  TEST_ASSERT_LESS_THAN_UINT8(80, state.eyes.focus_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(32, state.eyes.focus_percent);
}

void test_face_motion_safety_softens_blink_and_gaze_combination() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.safety_mode = ncos::core::contracts::FaceRenderSafetyMode::kNominal;
  state.eyes.direction = ncos::models::face::GazeDirection::kRight;
  state.eyes.focus_percent = 34;

  ncos::services::face::FaceMotionSafetyStatus status{};
  ncos::services::face::reset_face_motion_safety_status(&status, state, 1000);

  state.eyes.focus_percent = 62;
  state.lids.phase = ncos::models::face::BlinkPhase::kClosing;
  state.lids.openness_percent = 30;

  ncos::services::face::FaceMotionSafetyResult result{};
  TEST_ASSERT_TRUE(ncos::services::face::apply_face_motion_safety(&state, &status, 1120, &result));
  TEST_ASSERT_TRUE(result.limited_blink_interaction);
  TEST_ASSERT_LESS_THAN_UINT8(40, state.eyes.focus_percent);

  state.eyes.focus_percent = 48;
  state.lids.phase = ncos::models::face::BlinkPhase::kClosed;
  state.lids.openness_percent = 0;
  TEST_ASSERT_TRUE(ncos::services::face::apply_face_motion_safety(&state, &status, 1240, &result));
  TEST_ASSERT_EQUAL_UINT8(0, state.eyes.focus_percent);
}

void test_face_motion_safety_rate_limits_abrupt_transition() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.safety_mode = ncos::core::contracts::FaceRenderSafetyMode::kNominal;
  state.eyes.direction = ncos::models::face::GazeDirection::kLeft;
  state.eyes.focus_percent = 52;

  ncos::services::face::FaceMotionSafetyStatus status{};
  ncos::services::face::reset_face_motion_safety_status(&status, state, 1000);

  state.eyes.direction = ncos::models::face::GazeDirection::kDownRight;
  state.eyes.focus_percent = 68;

  ncos::services::face::FaceMotionSafetyResult result{};
  TEST_ASSERT_TRUE(ncos::services::face::apply_face_motion_safety(&state, &status, 1080, &result));
  TEST_ASSERT_TRUE(result.limited_velocity);
  TEST_ASSERT_TRUE(result.limited_diagonal);
  TEST_ASSERT_LESS_THAN_UINT8(68, state.eyes.focus_percent);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT8(36, state.eyes.focus_percent);
}

void test_face_motion_safety_keeps_calm_lateral_motion_expressive() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.safety_mode = ncos::core::contracts::FaceRenderSafetyMode::kNominal;
  state.eyes.direction = ncos::models::face::GazeDirection::kRight;
  state.eyes.focus_percent = 32;

  ncos::services::face::FaceMotionSafetyStatus status{};
  ncos::services::face::reset_face_motion_safety_status(&status, state, 1000);

  ncos::services::face::FaceMotionSafetyResult result{};
  TEST_ASSERT_FALSE(ncos::services::face::apply_face_motion_safety(&state, &status, 1120, &result));
  TEST_ASSERT_FALSE(result.active);
  TEST_ASSERT_EQUAL_UINT8(32, state.eyes.focus_percent);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_motion_safety_limits_diagonal_focus_progressively);
  RUN_TEST(test_face_motion_safety_softens_blink_and_gaze_combination);
  RUN_TEST(test_face_motion_safety_rate_limits_abrupt_transition);
  RUN_TEST(test_face_motion_safety_keeps_calm_lateral_motion_expressive);
  return UNITY_END();
}
