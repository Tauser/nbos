#include <stdint.h>

#include <unity.h>

#include "drivers/display/panel_capability_profile.hpp"
#include "drivers/display/panel_capability_profile.cpp"
#include "services/display/display_pipeline_analysis.hpp"
#include "services/display/display_pipeline_analysis.cpp"

namespace {

ncos::services::face::FaceFrame make_base_frame() {
  ncos::services::face::FaceFrame frame{};
  frame.background = 0x0000;
  frame.face_color = 0x1111;
  frame.eye_color = 0xFFFF;
  frame.head_x = 24;
  frame.head_y = 32;
  frame.head_w = 180;
  frame.head_h = 180;
  frame.head_radius = 28;
  frame.left_eye_x = 82;
  frame.left_eye_y = 118;
  frame.right_eye_x = 154;
  frame.right_eye_y = 118;
  frame.eye_w = 34;
  frame.eye_h = 52;
  return frame;
}

const ncos::drivers::display::DisplayPanelCapabilityProfile& panel_profile() {
  return ncos::drivers::display::active_panel_capability_profile();
}

void test_display_pipeline_reports_head_change_as_full_redraw_reason() {
  auto previous = make_base_frame();
  auto current = previous;
  current.head_w += 4;

  const auto plan = ncos::services::display::analyze_render_plan(
      &previous, current, 240, 320, ncos::services::display::DisplayRenderMode::kAuto, panel_profile());

  TEST_ASSERT_TRUE(plan.full_redraw);
  TEST_ASSERT_TRUE(ncos::services::display::has_full_redraw_reason(
      plan.full_redraw_reason, ncos::services::display::FullRedrawReason::kHeadGeometryChanged));
}

void test_display_pipeline_reports_background_change_as_full_redraw_reason() {
  auto previous = make_base_frame();
  auto current = previous;
  current.background = 0xFFFF;

  const auto plan = ncos::services::display::analyze_render_plan(
      &previous, current, 240, 320, ncos::services::display::DisplayRenderMode::kAuto, panel_profile());

  TEST_ASSERT_TRUE(plan.full_redraw);
  TEST_ASSERT_TRUE(ncos::services::display::has_full_redraw_reason(
      plan.full_redraw_reason, ncos::services::display::FullRedrawReason::kBackgroundChanged));
}

void test_display_pipeline_uses_dirty_rect_for_eye_motion_and_prefers_regional_composite_flush_path() {
  auto previous = make_base_frame();
  auto current = previous;
  current.left_eye_x += 14;
  current.right_eye_x += 14;

  const auto plan = ncos::services::display::analyze_render_plan(
      &previous, current, 240, 320, ncos::services::display::DisplayRenderMode::kAuto, panel_profile());

  TEST_ASSERT_FALSE(plan.full_redraw);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::services::display::FullRedrawReason::kNone),
                        static_cast<int>(plan.full_redraw_reason));
  TEST_ASSERT_TRUE(plan.dirty_rect.valid);
  TEST_ASSERT_GREATER_THAN(0, plan.dirty_rect.w);
  TEST_ASSERT_GREATER_THAN(0, plan.dirty_rect.h);
  TEST_ASSERT_TRUE(plan.dirty_rect_secondary.valid);
  TEST_ASSERT_GREATER_THAN(0, plan.dirty_rect_secondary.w);
  TEST_ASSERT_GREATER_THAN(0, plan.dirty_rect_secondary.h);
  TEST_ASSERT_LESS_THAN_INT(plan.dirty_rect_secondary.x, plan.dirty_rect.x + plan.dirty_rect.w);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::drivers::display::DisplayFlushPath::kRegionalComposite),
                        static_cast<int>(plan.recommended_flush_path));
}

void test_display_pipeline_marks_dirty_rect_when_clipped_by_bounds() {
  auto previous = make_base_frame();
  auto current = previous;
  previous.left_eye_x = 10;
  previous.right_eye_x = 44;
  current.left_eye_x = -8;
  current.right_eye_x = 20;

  const auto plan = ncos::services::display::analyze_render_plan(
      &previous, current, 240, 320, ncos::services::display::DisplayRenderMode::kForceDirtyRect,
      panel_profile());

  TEST_ASSERT_FALSE(plan.full_redraw);
  TEST_ASSERT_TRUE(plan.dirty_rect.valid);
  TEST_ASSERT_TRUE(plan.dirty_rect.clipped);
  TEST_ASSERT_EQUAL_INT(0, plan.dirty_rect.x);
}

void test_display_pipeline_forces_full_redraw_when_requested() {
  auto previous = make_base_frame();
  auto current = previous;
  current.left_eye_x += 8;

  const auto plan = ncos::services::display::analyze_render_plan(
      &previous, current, 240, 320,
      ncos::services::display::DisplayRenderMode::kForceFullRedraw, panel_profile());

  TEST_ASSERT_TRUE(plan.full_redraw);
  TEST_ASSERT_TRUE(ncos::services::display::has_full_redraw_reason(
      plan.full_redraw_reason, ncos::services::display::FullRedrawReason::kForced));
}

void test_display_pipeline_without_previous_frame_starts_with_full_redraw() {
  const auto current = make_base_frame();

  const auto plan = ncos::services::display::analyze_render_plan(
      nullptr, current, 240, 320, ncos::services::display::DisplayRenderMode::kForceDirtyRect,
      panel_profile());

  TEST_ASSERT_TRUE(plan.full_redraw);
  TEST_ASSERT_TRUE(ncos::services::display::has_full_redraw_reason(
      plan.full_redraw_reason, ncos::services::display::FullRedrawReason::kNoPreviousFrame));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_display_pipeline_reports_head_change_as_full_redraw_reason);
  RUN_TEST(test_display_pipeline_reports_background_change_as_full_redraw_reason);
  RUN_TEST(test_display_pipeline_uses_dirty_rect_for_eye_motion_and_prefers_regional_composite_flush_path);
  RUN_TEST(test_display_pipeline_marks_dirty_rect_when_clipped_by_bounds);
  RUN_TEST(test_display_pipeline_forces_full_redraw_when_requested);
  RUN_TEST(test_display_pipeline_without_previous_frame_starts_with_full_redraw);
  return UNITY_END();
}
