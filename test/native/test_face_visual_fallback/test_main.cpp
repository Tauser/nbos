#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_visual_fallback.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_visual_fallback.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_visual_fallback_enters_when_budget_and_dirty_rect_are_unstable() {
  ncos::services::face::FaceTuningTelemetry tuning{};
  tuning.degradation = ncos::services::face::FaceVisualDegradationFlag::kFrameOverBudget |
                       ncos::services::face::FaceVisualDegradationFlag::kLargeDirtyRect |
                       ncos::services::face::FaceVisualDegradationFlag::kDiagonalMotion;

  TEST_ASSERT_TRUE(ncos::services::face::should_enter_face_visual_fallback(tuning));
  TEST_ASSERT_FALSE(ncos::services::face::should_exit_face_visual_fallback(tuning));
}

void test_face_visual_fallback_clamps_diagonal_gaze_but_keeps_lids() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.safety_mode = ncos::core::contracts::FaceRenderSafetyMode::kNominal;
  state.eyes.direction = ncos::models::face::GazeDirection::kUpRight;
  state.eyes.focus_percent = 82;
  state.lids.openness_percent = 41;
  const auto revision_before = state.revision;

  ncos::services::face::apply_face_visual_fallback(&state);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::FaceRenderSafetyMode::kSafeFallback),
                        static_cast<int>(state.safety_mode));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeDirection::kRight),
                        static_cast<int>(state.eyes.direction));
  TEST_ASSERT_EQUAL_UINT8(36, state.eyes.focus_percent);
  TEST_ASSERT_EQUAL_UINT8(41, state.lids.openness_percent);
  TEST_ASSERT_TRUE(state.revision > revision_before);
}

void test_face_visual_fallback_status_tracks_entries_and_exits() {
  ncos::services::face::FaceVisualFallbackStatus status{};

  TEST_ASSERT_TRUE(ncos::services::face::set_face_visual_fallback_active(
      &status, true, 1200,
      ncos::services::face::FaceVisualDegradationFlag::kFrameOverBudget));
  TEST_ASSERT_TRUE(status.active);
  TEST_ASSERT_EQUAL_UINT32(1, status.entry_count);
  TEST_ASSERT_EQUAL_UINT32(0, status.exit_count);
  TEST_ASSERT_EQUAL_UINT64(1200, status.last_change_ms);

  TEST_ASSERT_TRUE(ncos::services::face::set_face_visual_fallback_active(
      &status, false, 1800, ncos::services::face::FaceVisualDegradationFlag::kNone));
  TEST_ASSERT_FALSE(status.active);
  TEST_ASSERT_EQUAL_UINT32(1, status.entry_count);
  TEST_ASSERT_EQUAL_UINT32(1, status.exit_count);
  TEST_ASSERT_EQUAL_UINT64(1800, status.last_change_ms);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_visual_fallback_enters_when_budget_and_dirty_rect_are_unstable);
  RUN_TEST(test_face_visual_fallback_clamps_diagonal_gaze_but_keeps_lids);
  RUN_TEST(test_face_visual_fallback_status_tracks_entries_and_exits);
  return UNITY_END();
}
