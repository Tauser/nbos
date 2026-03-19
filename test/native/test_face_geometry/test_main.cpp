#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_frame_composer.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_geometry.cpp"
#include "services/face/face_frame_composer.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_geometry_profiles_are_semantic_and_valid() {
  const auto balanced =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kCompanionBalanced);
  const auto heroic =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kHeroicWide);
  const auto playful =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kPlayfulRound);

  TEST_ASSERT_TRUE(balanced.eye_size_percent > 0);
  TEST_ASSERT_TRUE(heroic.jaw_width_percent > 0);
  TEST_ASSERT_TRUE(heroic.jaw_width_percent > balanced.jaw_width_percent);
  TEST_ASSERT_TRUE(playful.silhouette_roundness_percent > heroic.silhouette_roundness_percent);
}

void test_face_geometry_keeps_incremental_compatibility_with_render_state() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.version = ncos::core::contracts::FaceRenderStateVersion::kV1;

  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(state));

  state.version = ncos::core::contracts::FaceRenderStateVersion::kV2;
  state.geometry =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kCuriousTall);

  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(state));
}

void test_face_geometry_changes_silhouette_without_renderer_semantics() {
  auto wide_state = ncos::core::contracts::make_face_render_state_baseline();
  wide_state.geometry =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kHeroicWide);

  auto tall_state = ncos::core::contracts::make_face_render_state_baseline();
  tall_state.geometry =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kCuriousTall);

  ncos::services::face::FaceFrameComposer composer{};
  ncos::services::face::FaceFrame wide_frame{};
  ncos::services::face::FaceFrame tall_frame{};

  TEST_ASSERT_TRUE(composer.compose(wide_state, &wide_frame));
  TEST_ASSERT_TRUE(composer.compose(tall_state, &tall_frame));

  const int16_t wide_eye_span = wide_frame.right_eye_x - wide_frame.left_eye_x;
  const int16_t tall_eye_span = tall_frame.right_eye_x - tall_frame.left_eye_x;

  TEST_ASSERT_TRUE(wide_eye_span > tall_eye_span);
  TEST_ASSERT_TRUE(wide_frame.head_w > tall_frame.head_w);
  TEST_ASSERT_TRUE(tall_frame.head_h > wide_frame.head_h);
  TEST_ASSERT_EQUAL_INT16(0, wide_frame.mouth_w);
  TEST_ASSERT_EQUAL_INT16(0, tall_frame.mouth_w);
}

void test_face_geometry_scales_gaze_displacement_with_focus_percent() {
  auto low_focus_state = ncos::core::contracts::make_face_render_state_baseline();
  low_focus_state.eyes.direction = ncos::models::face::GazeDirection::kUpRight;
  low_focus_state.eyes.focus_percent = 20;

  auto high_focus_state = low_focus_state;
  high_focus_state.eyes.focus_percent = 100;

  ncos::services::face::FaceGeometryLayout low_layout{};
  ncos::services::face::FaceGeometryLayout high_layout{};

  TEST_ASSERT_TRUE(ncos::services::face::make_face_geometry_layout(low_focus_state, &low_layout));
  TEST_ASSERT_TRUE(ncos::services::face::make_face_geometry_layout(high_focus_state, &high_layout));

  TEST_ASSERT_TRUE(high_layout.gaze_dx > low_layout.gaze_dx);
  TEST_ASSERT_TRUE(low_layout.gaze_dy < 0);
  TEST_ASSERT_TRUE(high_layout.gaze_dy < low_layout.gaze_dy);
}

void test_face_geometry_keeps_diagonal_displacement_under_lateral_envelope() {
  auto lateral_state = ncos::core::contracts::make_face_render_state_baseline();
  lateral_state.eyes.direction = ncos::models::face::GazeDirection::kRight;
  lateral_state.eyes.focus_percent = 100;

  auto diagonal_state = lateral_state;
  diagonal_state.eyes.direction = ncos::models::face::GazeDirection::kUpRight;

  ncos::services::face::FaceGeometryLayout lateral_layout{};
  ncos::services::face::FaceGeometryLayout diagonal_layout{};

  TEST_ASSERT_TRUE(ncos::services::face::make_face_geometry_layout(lateral_state, &lateral_layout));
  TEST_ASSERT_TRUE(ncos::services::face::make_face_geometry_layout(diagonal_state, &diagonal_layout));

  TEST_ASSERT_TRUE(diagonal_layout.gaze_dx < lateral_layout.gaze_dx);
  TEST_ASSERT_TRUE(diagonal_layout.gaze_dy < 0);
  TEST_ASSERT_TRUE(-diagonal_layout.gaze_dy < lateral_layout.gaze_dx);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_geometry_profiles_are_semantic_and_valid);
  RUN_TEST(test_face_geometry_keeps_incremental_compatibility_with_render_state);
  RUN_TEST(test_face_geometry_changes_silhouette_without_renderer_semantics);
  RUN_TEST(test_face_geometry_scales_gaze_displacement_with_focus_percent);
  RUN_TEST(test_face_geometry_keeps_diagonal_displacement_under_lateral_envelope);
  return UNITY_END();
}
