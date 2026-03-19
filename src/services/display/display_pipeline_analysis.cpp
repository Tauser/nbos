#include "services/display/display_pipeline_analysis.hpp"

namespace {

constexpr int16_t EyeDirtyPadding = 10;

int16_t min_i16(int16_t a, int16_t b) {
  return a < b ? a : b;
}

int16_t max_i16(int16_t a, int16_t b) {
  return a > b ? a : b;
}

struct EyeRect {
  int16_t x = 0;
  int16_t y = 0;
  int16_t w = 0;
  int16_t h = 0;
};

EyeRect make_eye_rect(const ncos::services::face::FaceFrame& frame, bool left_eye) {
  const int16_t eye_x = left_eye ? frame.left_eye_x : frame.right_eye_x;
  const int16_t eye_y = left_eye ? frame.left_eye_y : frame.right_eye_y;

  EyeRect rect{};
  rect.w = static_cast<int16_t>(frame.eye_w + EyeDirtyPadding);
  rect.h = static_cast<int16_t>(frame.eye_h + EyeDirtyPadding);
  rect.x = static_cast<int16_t>(eye_x - rect.w / 2);
  rect.y = static_cast<int16_t>(eye_y - rect.h / 2);
  return rect;
}

ncos::services::display::DirtyRect clip_rect(EyeRect rect, int16_t display_width, int16_t display_height) {
  ncos::services::display::DirtyRect clipped{};
  const int16_t unclipped_x0 = rect.x;
  const int16_t unclipped_y0 = rect.y;
  const int16_t unclipped_x1 = static_cast<int16_t>(rect.x + rect.w);
  const int16_t unclipped_y1 = static_cast<int16_t>(rect.y + rect.h);

  int16_t x0 = rect.x;
  int16_t y0 = rect.y;
  int16_t x1 = unclipped_x1;
  int16_t y1 = unclipped_y1;

  if (x0 < 0) {
    x0 = 0;
  }
  if (y0 < 0) {
    y0 = 0;
  }
  if (x1 > display_width) {
    x1 = display_width;
  }
  if (y1 > display_height) {
    y1 = display_height;
  }

  clipped.clipped = unclipped_x0 != x0 || unclipped_y0 != y0 || unclipped_x1 != x1 ||
                    unclipped_y1 != y1;
  clipped.valid = x1 > x0 && y1 > y0;
  clipped.x = x0;
  clipped.y = y0;
  clipped.w = clipped.valid ? static_cast<int16_t>(x1 - x0) : 0;
  clipped.h = clipped.valid ? static_cast<int16_t>(y1 - y0) : 0;
  return clipped;
}

bool rects_overlap_or_touch(const ncos::services::display::DirtyRect& a,
                            const ncos::services::display::DirtyRect& b) {
  if (!a.valid || !b.valid) {
    return false;
  }

  const int16_t ax1 = static_cast<int16_t>(a.x + a.w);
  const int16_t ay1 = static_cast<int16_t>(a.y + a.h);
  const int16_t bx1 = static_cast<int16_t>(b.x + b.w);
  const int16_t by1 = static_cast<int16_t>(b.y + b.h);

  return !(ax1 < b.x || bx1 < a.x || ay1 < b.y || by1 < a.y);
}

ncos::services::display::DirtyRect merge_rects(const ncos::services::display::DirtyRect& a,
                                                const ncos::services::display::DirtyRect& b) {
  if (!a.valid) {
    return b;
  }
  if (!b.valid) {
    return a;
  }

  ncos::services::display::DirtyRect merged{};
  const int16_t x0 = min_i16(a.x, b.x);
  const int16_t y0 = min_i16(a.y, b.y);
  const int16_t x1 = max_i16(static_cast<int16_t>(a.x + a.w), static_cast<int16_t>(b.x + b.w));
  const int16_t y1 = max_i16(static_cast<int16_t>(a.y + a.h), static_cast<int16_t>(b.y + b.h));

  merged.valid = true;
  merged.clipped = a.clipped || b.clipped;
  merged.x = x0;
  merged.y = y0;
  merged.w = static_cast<int16_t>(x1 - x0);
  merged.h = static_cast<int16_t>(y1 - y0);
  return merged;
}

bool high_contrast_frame(const ncos::services::face::FaceFrame& frame) {
  return frame.background != frame.face_color || frame.face_color != frame.eye_color;
}

bool motion_present(const ncos::services::face::FaceFrame& previous,
                    const ncos::services::face::FaceFrame& current) {
  return previous.left_eye_x != current.left_eye_x || previous.left_eye_y != current.left_eye_y ||
         previous.right_eye_x != current.right_eye_x || previous.right_eye_y != current.right_eye_y ||
         previous.head_x != current.head_x || previous.head_y != current.head_y;
}

ncos::services::display::FullRedrawReason detect_full_redraw_reason(
    const ncos::services::face::FaceFrame& previous,
    const ncos::services::face::FaceFrame& current) {
  using ncos::services::display::FullRedrawReason;

  FullRedrawReason reason = FullRedrawReason::kNone;
  if (previous.background != current.background) {
    reason = reason | FullRedrawReason::kBackgroundChanged;
  }
  if (previous.face_color != current.face_color) {
    reason = reason | FullRedrawReason::kFaceColorChanged;
  }
  if (previous.head_x != current.head_x || previous.head_y != current.head_y ||
      previous.head_w != current.head_w || previous.head_h != current.head_h ||
      previous.head_radius != current.head_radius) {
    reason = reason | FullRedrawReason::kHeadGeometryChanged;
  }

  return reason;
}

}  // namespace

namespace ncos::services::display {

DirtyRect compute_eye_dirty_rect(const ncos::services::face::FaceFrame& previous,
                                 const ncos::services::face::FaceFrame& current,
                                 int16_t display_width,
                                 int16_t display_height) {
  const EyeRect prev_left = make_eye_rect(previous, true);
  const EyeRect prev_right = make_eye_rect(previous, false);
  const EyeRect next_left = make_eye_rect(current, true);
  const EyeRect next_right = make_eye_rect(current, false);

  DirtyRect left_rect = clip_rect(
      {min_i16(prev_left.x, next_left.x), min_i16(prev_left.y, next_left.y),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_left.x + prev_left.w),
                                    static_cast<int16_t>(next_left.x + next_left.w)) -
                            min_i16(prev_left.x, next_left.x)),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_left.y + prev_left.h),
                                    static_cast<int16_t>(next_left.y + next_left.h)) -
                            min_i16(prev_left.y, next_left.y))},
      display_width, display_height);
  DirtyRect right_rect = clip_rect(
      {min_i16(prev_right.x, next_right.x), min_i16(prev_right.y, next_right.y),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_right.x + prev_right.w),
                                    static_cast<int16_t>(next_right.x + next_right.w)) -
                            min_i16(prev_right.x, next_right.x)),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_right.y + prev_right.h),
                                    static_cast<int16_t>(next_right.y + next_right.h)) -
                            min_i16(prev_right.y, next_right.y))},
      display_width, display_height);

  return rects_overlap_or_touch(left_rect, right_rect) ? merge_rects(left_rect, right_rect) : left_rect;
}

DisplayRenderPlan analyze_render_plan(const ncos::services::face::FaceFrame* previous,
                                      const ncos::services::face::FaceFrame& current,
                                      int16_t display_width,
                                      int16_t display_height,
                                      DisplayRenderMode requested_mode,
                                      const ncos::drivers::display::DisplayPanelCapabilityProfile& panel_profile) {
  DisplayRenderPlan plan{};
  plan.requested_mode = requested_mode;
  plan.effective_mode = requested_mode;
  plan.recommended_flush_path = ncos::drivers::display::DisplayFlushPath::kDirectPrimitives;

  if (previous == nullptr) {
    plan.full_redraw = true;
    plan.full_redraw_reason = FullRedrawReason::kNoPreviousFrame;
    plan.effective_mode = DisplayRenderMode::kForceFullRedraw;
    plan.high_contrast_motion = false;
    return plan;
  }

  plan.high_contrast_motion = motion_present(*previous, current) && high_contrast_frame(current);

  const FullRedrawReason transition_reason = detect_full_redraw_reason(*previous, current);
  if (transition_reason != FullRedrawReason::kNone) {
    plan.full_redraw = true;
    plan.full_redraw_reason = transition_reason;
    plan.effective_mode = DisplayRenderMode::kForceFullRedraw;
    return plan;
  }

  if (requested_mode == DisplayRenderMode::kForceFullRedraw) {
    plan.full_redraw = true;
    plan.full_redraw_reason = FullRedrawReason::kForced;
    plan.effective_mode = DisplayRenderMode::kForceFullRedraw;
    return plan;
  }

  plan.full_redraw = false;
  plan.full_redraw_reason = FullRedrawReason::kNone;
  plan.effective_mode = DisplayRenderMode::kForceDirtyRect;

  const EyeRect prev_left = make_eye_rect(*previous, true);
  const EyeRect prev_right = make_eye_rect(*previous, false);
  const EyeRect next_left = make_eye_rect(current, true);
  const EyeRect next_right = make_eye_rect(current, false);

  plan.dirty_rect = clip_rect(
      {min_i16(prev_left.x, next_left.x), min_i16(prev_left.y, next_left.y),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_left.x + prev_left.w),
                                    static_cast<int16_t>(next_left.x + next_left.w)) -
                            min_i16(prev_left.x, next_left.x)),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_left.y + prev_left.h),
                                    static_cast<int16_t>(next_left.y + next_left.h)) -
                            min_i16(prev_left.y, next_left.y))},
      display_width, display_height);
  plan.dirty_rect_secondary = clip_rect(
      {min_i16(prev_right.x, next_right.x), min_i16(prev_right.y, next_right.y),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_right.x + prev_right.w),
                                    static_cast<int16_t>(next_right.x + next_right.w)) -
                            min_i16(prev_right.x, next_right.x)),
       static_cast<int16_t>(max_i16(static_cast<int16_t>(prev_right.y + prev_right.h),
                                    static_cast<int16_t>(next_right.y + next_right.h)) -
                            min_i16(prev_right.y, next_right.y))},
      display_width, display_height);

  if (rects_overlap_or_touch(plan.dirty_rect, plan.dirty_rect_secondary)) {
    plan.dirty_rect = merge_rects(plan.dirty_rect, plan.dirty_rect_secondary);
    plan.dirty_rect_secondary = DirtyRect{};
  }

  if (!ncos::drivers::display::flush_path_recommended(
          panel_profile, ncos::drivers::display::DisplayFlushPath::kDirectPrimitives,
          plan.high_contrast_motion)) {
    plan.recommended_flush_path = ncos::drivers::display::DisplayFlushPath::kSpriteWindow;
  }

  return plan;
}

}  // namespace ncos::services::display
