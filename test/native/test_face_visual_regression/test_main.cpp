#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"
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

namespace {

struct FaceVisualSignature {
  int16_t left_eye_x = 0;
  int16_t left_eye_y = 0;
  int16_t right_eye_x = 0;
  int16_t right_eye_y = 0;
  int16_t eye_radius = 0;
  int16_t eye_w = 0;
  int16_t eye_h = 0;
  int16_t eye_corner = 0;
  int16_t left_eye_w = 0;
  int16_t left_eye_h = 0;
  int16_t right_eye_w = 0;
  int16_t right_eye_h = 0;
  int16_t head_w = 0;
  int16_t head_h = 0;
};

FaceVisualSignature compose_signature(const ncos::core::contracts::FaceRenderState& state) {
  ncos::services::face::FaceFrameComposer composer{};
  ncos::services::face::FaceFrame frame{};
  TEST_ASSERT_TRUE(composer.compose(state, &frame));

  return {
      frame.left_eye_x,
      frame.left_eye_y,
      frame.right_eye_x,
      frame.right_eye_y,
      frame.eye_radius,
      frame.eye_w,
      frame.eye_h,
      frame.eye_corner,
      frame.left_eye_w,
      frame.left_eye_h,
      frame.right_eye_w,
      frame.right_eye_h,
      frame.head_w,
      frame.head_h,
  };
}

void assert_signature(const FaceVisualSignature& actual, const FaceVisualSignature& expected) {
  TEST_ASSERT_EQUAL_INT16(expected.left_eye_x, actual.left_eye_x);
  TEST_ASSERT_EQUAL_INT16(expected.left_eye_y, actual.left_eye_y);
  TEST_ASSERT_EQUAL_INT16(expected.right_eye_x, actual.right_eye_x);
  TEST_ASSERT_EQUAL_INT16(expected.right_eye_y, actual.right_eye_y);
  TEST_ASSERT_EQUAL_INT16(expected.eye_radius, actual.eye_radius);
  TEST_ASSERT_EQUAL_INT16(expected.eye_w, actual.eye_w);
  TEST_ASSERT_EQUAL_INT16(expected.eye_h, actual.eye_h);
  TEST_ASSERT_EQUAL_INT16(expected.eye_corner, actual.eye_corner);
  TEST_ASSERT_EQUAL_INT16(expected.left_eye_w, actual.left_eye_w);
  TEST_ASSERT_EQUAL_INT16(expected.left_eye_h, actual.left_eye_h);
  TEST_ASSERT_EQUAL_INT16(expected.right_eye_w, actual.right_eye_w);
  TEST_ASSERT_EQUAL_INT16(expected.right_eye_h, actual.right_eye_h);
  TEST_ASSERT_EQUAL_INT16(expected.head_w, actual.head_w);
  TEST_ASSERT_EQUAL_INT16(expected.head_h, actual.head_h);
}

}  // namespace

void test_face_visual_regression_freezes_key_gaze_signatures() {
  auto center_state = ncos::core::contracts::make_face_render_state_baseline();
  center_state.eyes.focus_percent = 100;

  auto left_state = center_state;
  left_state.eyes.direction = ncos::models::face::GazeDirection::kLeft;

  auto right_state = center_state;
  right_state.eyes.direction = ncos::models::face::GazeDirection::kRight;

  auto up_right_state = center_state;
  up_right_state.eyes.direction = ncos::models::face::GazeDirection::kUpRight;

  assert_signature(compose_signature(center_state), {109, 112, 211, 112, 16, 67, 67, 14, 67, 67, 67, 67, 145, 149});
  assert_signature(compose_signature(left_state), {100, 112, 203, 112, 16, 67, 67, 14, 70, 70, 66, 66, 145, 149});
  assert_signature(compose_signature(right_state), {117, 112, 220, 112, 16, 67, 67, 14, 66, 66, 70, 70, 145, 149});
  assert_signature(compose_signature(up_right_state), {115, 108, 217, 108, 16, 67, 67, 14, 67, 67, 67, 67, 145, 149});
}

void test_face_visual_regression_freezes_blink_aperture_signatures() {
  auto open_state = ncos::core::contracts::make_face_render_state_baseline();

  auto closing_state = open_state;
  closing_state.lids.phase = ncos::models::face::BlinkPhase::kClosing;
  closing_state.lids.openness_percent = 50;

  auto closed_state = open_state;
  closed_state.lids.phase = ncos::models::face::BlinkPhase::kClosed;
  closed_state.lids.openness_percent = 0;

  const auto open_signature = compose_signature(open_state);
  const auto closing_signature = compose_signature(closing_state);
  const auto closed_signature = compose_signature(closed_state);

  assert_signature(open_signature, {109, 112, 211, 112, 16, 67, 67, 14, 67, 67, 67, 67, 145, 149});
  assert_signature(closing_signature, {109, 112, 211, 112, 8, 67, 33, 8, 67, 33, 67, 33, 145, 149});
  assert_signature(closed_signature, {109, 112, 211, 112, 2, 67, 2, 5, 67, 2, 67, 2, 145, 149});
}

void test_face_visual_regression_freezes_official_preset_signatures() {
  struct PresetExpectation {
    ncos::services::face::FaceOfficialPresetId preset;
    FaceVisualSignature signature;
  };

  constexpr PresetExpectation kExpectations[] = {
      {ncos::services::face::FaceOfficialPresetId::kCoreNeutral,
       {107, 112, 213, 112, 17, 70, 70, 14, 70, 70, 70, 70, 145, 149}},
      {ncos::services::face::FaceOfficialPresetId::kCoreAttend,
       {112, 111, 224, 111, 16, 68, 59, 14, 67, 58, 71, 62, 156, 134}},
      {ncos::services::face::FaceOfficialPresetId::kCoreCalm,
       {109, 116, 211, 116, 8, 64, 48, 12, 64, 48, 64, 48, 145, 149}},
      {ncos::services::face::FaceOfficialPresetId::kCoreCurious,
       {104, 106, 206, 106, 18, 72, 69, 14, 72, 69, 72, 69, 139, 152}},
      {ncos::services::face::FaceOfficialPresetId::kCoreLock,
       {94, 112, 209, 112, 16, 67, 54, 13, 70, 57, 66, 54, 156, 134}},
  };

  for (const auto& expectation : kExpectations) {
    auto state = ncos::core::contracts::make_face_render_state_baseline();
    TEST_ASSERT_TRUE(ncos::services::face::apply_official_face_preset(expectation.preset, &state, 1200));
    assert_signature(compose_signature(state), expectation.signature);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_visual_regression_freezes_key_gaze_signatures);
  RUN_TEST(test_face_visual_regression_freezes_blink_aperture_signatures);
  RUN_TEST(test_face_visual_regression_freezes_official_preset_signatures);
  return UNITY_END();
}
