#include <unity.h>

#include "services/face/face_clip_player.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_compositor.cpp"
#include "services/face/face_clip_player.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

constexpr ncos::models::face::FaceClipKeyframe kClipFrames[] = {
    {0, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kRight, 70, 90}},
    {180, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kLeft, 64, 82}},
    {320, {ncos::models::face::GazeAnchor::kUser, ncos::models::face::GazeDirection::kCenter, 50, 94}},
};

constexpr ncos::models::face::FaceClip kClip = {
    2001,
    "test_signature",
    kClipFrames,
    sizeof(kClipFrames) / sizeof(kClipFrames[0]),
    360,
    140,
    8,
    500,
    220,
};

}  // namespace

void test_face_clip_player_plays_and_recovers_to_baseline() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  ncos::services::face::FaceClipPlayer player{33};

  TEST_ASSERT_TRUE(compositor.bind_state(&state));
  TEST_ASSERT_TRUE(player.play(kClip, &compositor, &state, 1000));

  TEST_ASSERT_TRUE(player.active());
  TEST_ASSERT_EQUAL_UINT32(2001, player.active_clip_id());

  TEST_ASSERT_TRUE(player.tick(1120, &compositor, &state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeDirection::kRight),
                        static_cast<int>(state.eyes.direction));

  TEST_ASSERT_TRUE(player.tick(1220, &compositor, &state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeDirection::kLeft),
                        static_cast<int>(state.eyes.direction));

  TEST_ASSERT_TRUE(player.tick(1365, &compositor, &state));
  TEST_ASSERT_TRUE(player.recovering());

  TEST_ASSERT_TRUE(player.tick(1520, &compositor, &state));
  TEST_ASSERT_FALSE(player.active());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeDirection::kCenter),
                        static_cast<int>(state.eyes.direction));
  TEST_ASSERT_FALSE(compositor.can_write(ncos::models::face::FaceLayer::kClip, 33));
}

void test_face_clip_player_cancel_starts_recovery() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  ncos::services::face::FaceClipPlayer player{33};

  TEST_ASSERT_TRUE(compositor.bind_state(&state));
  TEST_ASSERT_TRUE(player.play(kClip, &compositor, &state, 2000));
  TEST_ASSERT_TRUE(player.tick(2120, &compositor, &state));

  TEST_ASSERT_TRUE(player.cancel(2140, &state));
  TEST_ASSERT_TRUE(player.recovering());

  TEST_ASSERT_TRUE(player.tick(2300, &compositor, &state));
  TEST_ASSERT_FALSE(player.active());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeDirection::kCenter),
                        static_cast<int>(state.eyes.direction));
}

void test_face_clip_player_stops_when_preempted_by_other_clip_owner() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  ncos::services::face::FaceClipPlayer player{33};

  TEST_ASSERT_TRUE(compositor.bind_state(&state));
  TEST_ASSERT_TRUE(player.play(kClip, &compositor, &state, 3000));

  ncos::services::face::FaceLayerRequest preempt{};
  preempt.layer = ncos::models::face::FaceLayer::kClip;
  preempt.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kClipOwner;
  preempt.requester_service = 99;
  preempt.priority = 9;

  TEST_ASSERT_TRUE(compositor.request_layer(preempt, 3200).granted);

  TEST_ASSERT_FALSE(player.tick(3210, &compositor, &state));
  TEST_ASSERT_FALSE(player.active());
  TEST_ASSERT_FALSE(compositor.can_write(ncos::models::face::FaceLayer::kClip, 33));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_clip_player_plays_and_recovers_to_baseline);
  RUN_TEST(test_face_clip_player_cancel_starts_recovery);
  RUN_TEST(test_face_clip_player_stops_when_preempted_by_other_clip_owner);
  return UNITY_END();
}
