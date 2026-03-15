#include <unity.h>

#include "services/face/face_clip_player.hpp"
#include "services/face/face_compositor.hpp"
#include "services/face/face_preset_library.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_compositor.cpp"
#include "services/face/face_clip_player.cpp"
#include "services/face/face_preset_library.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

constexpr ncos::models::face::FaceClipKeyframe kFreezeValidationClipFrames[] = {
    {0, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kRight, 78, 90}},
    {140, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kLeft, 72, 84}},
    {260, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kCenter, 64, 92}},
};

constexpr ncos::models::face::FaceClip kFreezeValidationClip = {
    3101,
    "freeze_validation_clip",
    kFreezeValidationClipFrames,
    sizeof(kFreezeValidationClipFrames) / sizeof(kFreezeValidationClipFrames[0]),
    300,
    180,
    8,
    620,
    260,
};

}  // namespace

void test_official_preset_coexists_with_compositor_and_clip_recovery() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  ncos::services::face::FaceClipPlayer clip_player{33};

  TEST_ASSERT_TRUE(compositor.bind_state(&state));

  ncos::services::face::FaceLayerRequest base_request{};
  base_request.layer = ncos::models::face::FaceLayer::kBase;
  base_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  base_request.requester_service = 31;
  base_request.priority = 3;
  TEST_ASSERT_TRUE(compositor.request_layer(base_request, 1000).granted);

  ncos::services::face::FaceLayerRequest gaze_request{};
  gaze_request.layer = ncos::models::face::FaceLayer::kGaze;
  gaze_request.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze_request.requester_service = 32;
  gaze_request.priority = 5;
  TEST_ASSERT_TRUE(compositor.request_layer(gaze_request, 1000).granted);

  TEST_ASSERT_TRUE(ncos::services::face::apply_official_face_preset(
      ncos::services::face::FaceOfficialPresetId::kCoreAttend, &state, 1010));

  const auto baseline_direction = state.eyes.direction;
  const auto baseline_focus = state.eyes.focus_percent;
  const auto baseline_lids = state.lids.openness_percent;

  TEST_ASSERT_TRUE(clip_player.play(kFreezeValidationClip, &compositor, &state, 1100));
  TEST_ASSERT_TRUE(clip_player.tick(1240, &compositor, &state));
  TEST_ASSERT_NOT_EQUAL(static_cast<int>(baseline_direction), static_cast<int>(state.eyes.direction));

  TEST_ASSERT_TRUE(clip_player.tick(1430, &compositor, &state));
  TEST_ASSERT_TRUE(clip_player.recovering());

  TEST_ASSERT_TRUE(clip_player.tick(1650, &compositor, &state));
  TEST_ASSERT_FALSE(clip_player.active());

  TEST_ASSERT_EQUAL_INT(static_cast<int>(baseline_direction), static_cast<int>(state.eyes.direction));
  TEST_ASSERT_EQUAL_UINT8(baseline_focus, state.eyes.focus_percent);
  TEST_ASSERT_EQUAL_UINT8(baseline_lids, state.lids.openness_percent);
  TEST_ASSERT_FALSE(compositor.can_write(ncos::models::face::FaceLayer::kClip, 33));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_official_preset_coexists_with_compositor_and_clip_recovery);
  return UNITY_END();
}
