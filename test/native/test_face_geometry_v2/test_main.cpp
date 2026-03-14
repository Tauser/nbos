#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_frame_composer.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_frame_composer.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_geometry_v2_profiles_are_semantic_and_valid() {
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

void test_face_geometry_v2_keeps_incremental_compatibility_with_render_state() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.version = ncos::core::contracts::FaceRenderStateVersion::kV1;

  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(state));

  state.version = ncos::core::contracts::FaceRenderStateVersion::kV2;
  state.geometry =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kCuriousTall);

  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(state));
}

void test_face_geometry_v2_changes_silhouette_without_renderer_semantics() {
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
  TEST_ASSERT_TRUE(wide_frame.mouth_w > tall_frame.mouth_w);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_geometry_v2_profiles_are_semantic_and_valid);
  RUN_TEST(test_face_geometry_v2_keeps_incremental_compatibility_with_render_state);
  RUN_TEST(test_face_geometry_v2_changes_silhouette_without_renderer_semantics);
  return UNITY_END();
}
