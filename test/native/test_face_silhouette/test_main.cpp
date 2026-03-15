#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_frame_composer.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_geometry.cpp"
#include "services/face/face_frame_composer.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

struct FrozenSilhouetteFrame {
  ncos::models::face::FaceShapeProfile profile;
  int16_t head_w;
  int16_t head_h;
  int16_t head_radius;
  int16_t eye_span;
};

constexpr FrozenSilhouetteFrame kFrozenFrames[] = {
    {ncos::models::face::FaceShapeProfile::kCompanionBalanced, 145, 149, 20, 66},
    {ncos::models::face::FaceShapeProfile::kHeroicWide, 156, 134, 15, 72},
    {ncos::models::face::FaceShapeProfile::kCuriousTall, 139, 152, 18, 62},
    {ncos::models::face::FaceShapeProfile::kPlayfulRound, 147, 151, 24, 64},
};

ncos::services::face::FaceFrame compose_profile(ncos::models::face::FaceShapeProfile profile) {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.geometry = ncos::core::contracts::make_shape_geometry_profile(profile);

  ncos::services::face::FaceFrame frame{};
  ncos::services::face::FaceFrameComposer composer{};
  TEST_ASSERT_TRUE(composer.compose(state, &frame));
  return frame;
}

}  // namespace

void test_face_silhouette_frozen_keyframes_match_geometry_profiles() {
  for (const auto& expected : kFrozenFrames) {
    const auto frame = compose_profile(expected.profile);

    const int16_t eye_span = static_cast<int16_t>(frame.right_eye_x - frame.left_eye_x);

    TEST_ASSERT_EQUAL_INT16(expected.head_w, frame.head_w);
    TEST_ASSERT_EQUAL_INT16(expected.head_h, frame.head_h);
    TEST_ASSERT_EQUAL_INT16(expected.head_radius, frame.head_radius);
    TEST_ASSERT_EQUAL_INT16(expected.eye_span, eye_span);
    TEST_ASSERT_EQUAL_INT16(0, frame.mouth_w);
  }
}

void test_face_silhouette_profiles_are_pairwise_distinguishable() {
  const auto balanced = compose_profile(ncos::models::face::FaceShapeProfile::kCompanionBalanced);
  const auto heroic = compose_profile(ncos::models::face::FaceShapeProfile::kHeroicWide);
  const auto curious = compose_profile(ncos::models::face::FaceShapeProfile::kCuriousTall);
  const auto playful = compose_profile(ncos::models::face::FaceShapeProfile::kPlayfulRound);

  TEST_ASSERT_TRUE(heroic.head_w > balanced.head_w);
  TEST_ASSERT_TRUE(curious.head_h > balanced.head_h);
  TEST_ASSERT_TRUE(playful.head_radius > balanced.head_radius);
  TEST_ASSERT_TRUE(heroic.head_h < curious.head_h);
}

void test_face_silhouette_renderer_contract_stays_semantic_free() {
  auto baseline = ncos::core::contracts::make_face_render_state_baseline();
  baseline.geometry =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kCompanionBalanced);

  auto variant = baseline;
  variant.geometry =
      ncos::core::contracts::make_shape_geometry_profile(ncos::models::face::FaceShapeProfile::kHeroicWide);

  ncos::services::face::FaceFrame frame_a{};
  ncos::services::face::FaceFrame frame_b{};
  ncos::services::face::FaceFrameComposer composer{};

  TEST_ASSERT_TRUE(composer.compose(baseline, &frame_a));
  TEST_ASSERT_TRUE(composer.compose(variant, &frame_b));

  TEST_ASSERT_EQUAL_INT(static_cast<int>(baseline.composition.mode),
                        static_cast<int>(variant.composition.mode));
  TEST_ASSERT_NOT_EQUAL(frame_a.head_w, frame_b.head_w);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_silhouette_frozen_keyframes_match_geometry_profiles);
  RUN_TEST(test_face_silhouette_profiles_are_pairwise_distinguishable);
  RUN_TEST(test_face_silhouette_renderer_contract_stays_semantic_free);
  return UNITY_END();
}

