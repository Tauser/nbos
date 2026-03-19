#include <unity.h>

#include "services/face/face_frame_composer.hpp"
#include "services/face/face_preset_library.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_geometry.cpp"
#include "services/face/face_frame_composer.cpp"
#define clamp_percent face_preset_library_clamp_percent
#include "services/face/face_preset_library.cpp"
#undef clamp_percent

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_frame_composer_maps_baseline_to_renderable_frame() {
  const auto state = ncos::core::contracts::make_face_render_state_baseline();

  ncos::services::face::FaceFrame frame{};
  ncos::services::face::FaceFrameComposer composer{};

  TEST_ASSERT_TRUE(composer.compose(state, &frame));
  TEST_ASSERT_GREATER_THAN_INT16(0, frame.eye_radius);
  TEST_ASSERT_LESS_THAN_INT16(frame.right_eye_x, frame.left_eye_x);
  TEST_ASSERT_GREATER_THAN_INT16(0, frame.eye_w);
  TEST_ASSERT_GREATER_THAN_INT16(0, frame.eye_h);
  TEST_ASSERT_GREATER_THAN_INT16(0, frame.left_eye_w);
  TEST_ASSERT_GREATER_THAN_INT16(0, frame.left_eye_h);
  TEST_ASSERT_EQUAL_INT16(frame.eye_w, frame.left_eye_w);
  TEST_ASSERT_EQUAL_INT16(frame.eye_w, frame.right_eye_w);
  TEST_ASSERT_GREATER_THAN_INT16(0, frame.head_w);
  TEST_ASSERT_GREATER_THAN_INT16(0, frame.head_h);
  TEST_ASSERT_EQUAL_INT16(0, frame.mouth_w);
  TEST_ASSERT_EQUAL_INT16(0, frame.mouth_h);
}

void test_face_frame_composer_applies_gaze_offset_without_renderer_semantics() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.eyes.direction = ncos::models::face::GazeDirection::kLeft;

  ncos::services::face::FaceFrame frame_left{};
  ncos::services::face::FaceFrameComposer composer{};
  TEST_ASSERT_TRUE(composer.compose(state, &frame_left));

  state.eyes.direction = ncos::models::face::GazeDirection::kRight;
  ncos::services::face::FaceFrame frame_right{};
  TEST_ASSERT_TRUE(composer.compose(state, &frame_right));

  TEST_ASSERT_GREATER_THAN_INT16(frame_left.left_eye_x, frame_right.left_eye_x);
  TEST_ASSERT_GREATER_THAN_INT16(frame_left.right_eye_x, frame_right.right_eye_x);
}

void test_face_frame_composer_supports_controlled_eye_asymmetry() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.eyes.left_adjust.x_offset_px = -3;
  state.eyes.left_adjust.y_offset_px = 2;
  state.eyes.left_adjust.size_delta_percent = -10;
  state.eyes.left_adjust.openness_delta_percent = -20;
  state.eyes.right_adjust.x_offset_px = 4;
  state.eyes.right_adjust.y_offset_px = -1;
  state.eyes.right_adjust.size_delta_percent = 8;
  state.eyes.right_adjust.openness_delta_percent = 10;

  ncos::services::face::FaceFrame frame{};
  ncos::services::face::FaceFrameComposer composer{};

  TEST_ASSERT_TRUE(composer.compose(state, &frame));
  TEST_ASSERT_TRUE(frame.left_eye_x < frame.right_eye_x);
  TEST_ASSERT_TRUE(frame.left_eye_y > frame.right_eye_y);
  TEST_ASSERT_TRUE(frame.left_eye_w < frame.right_eye_w);
  TEST_ASSERT_TRUE(frame.left_eye_h < frame.right_eye_h);
}

void test_face_official_preset_resets_eye_asymmetry_adjustments() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.eyes.left_adjust.x_offset_px = -4;
  state.eyes.right_adjust.size_delta_percent = 12;

  TEST_ASSERT_TRUE(ncos::services::face::apply_official_face_preset(
      ncos::services::face::FaceOfficialPresetId::kCoreAttend, &state, 1500));

  TEST_ASSERT_EQUAL_INT8(0, state.eyes.left_adjust.x_offset_px);
  TEST_ASSERT_EQUAL_INT8(0, state.eyes.left_adjust.size_delta_percent);
  TEST_ASSERT_EQUAL_INT8(0, state.eyes.right_adjust.x_offset_px);
  TEST_ASSERT_EQUAL_INT8(0, state.eyes.right_adjust.size_delta_percent);
}

void test_face_frame_composer_rejects_invalid_render_state() {
  auto invalid_state = ncos::core::contracts::make_face_render_state_baseline();
  invalid_state.composition.layers[0].owner_role = ncos::core::contracts::FaceLayerOwnerRole::kClipOwner;

  ncos::services::face::FaceFrame frame{};
  ncos::services::face::FaceFrameComposer composer{};

  TEST_ASSERT_FALSE(composer.compose(invalid_state, &frame));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_frame_composer_maps_baseline_to_renderable_frame);
  RUN_TEST(test_face_frame_composer_applies_gaze_offset_without_renderer_semantics);
  RUN_TEST(test_face_frame_composer_supports_controlled_eye_asymmetry);
  RUN_TEST(test_face_official_preset_resets_eye_asymmetry_adjustments);
  RUN_TEST(test_face_frame_composer_rejects_invalid_render_state);
  return UNITY_END();
}
