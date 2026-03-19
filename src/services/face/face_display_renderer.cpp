#include "services/face/face_display_renderer.hpp"

#include <chrono>

namespace {

constexpr uint16_t OrientationDotColor = 0xF800;

uint64_t monotonic_time_us() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

bool frames_equal(const ncos::services::face::FaceFrame& lhs, const ncos::services::face::FaceFrame& rhs) {
  return lhs.background == rhs.background && lhs.face_color == rhs.face_color &&
         lhs.eye_color == rhs.eye_color && lhs.pupil_color == rhs.pupil_color &&
         lhs.mouth_color == rhs.mouth_color && lhs.head_x == rhs.head_x && lhs.head_y == rhs.head_y &&
         lhs.head_w == rhs.head_w && lhs.head_h == rhs.head_h && lhs.head_radius == rhs.head_radius &&
         lhs.left_eye_x == rhs.left_eye_x && lhs.left_eye_y == rhs.left_eye_y &&
         lhs.right_eye_x == rhs.right_eye_x && lhs.right_eye_y == rhs.right_eye_y &&
         lhs.eye_radius == rhs.eye_radius && lhs.eye_w == rhs.eye_w && lhs.eye_h == rhs.eye_h &&
         lhs.eye_corner == rhs.eye_corner && lhs.left_pupil_x == rhs.left_pupil_x &&
         lhs.left_pupil_y == rhs.left_pupil_y && lhs.right_pupil_x == rhs.right_pupil_x &&
         lhs.right_pupil_y == rhs.right_pupil_y && lhs.pupil_radius == rhs.pupil_radius &&
         lhs.mouth_x == rhs.mouth_x && lhs.mouth_y == rhs.mouth_y && lhs.mouth_w == rhs.mouth_w &&
         lhs.mouth_h == rhs.mouth_h && lhs.mouth_corner == rhs.mouth_corner;
}

}  // namespace

namespace ncos::services::face {

bool FaceDisplayRenderer::bind(ncos::drivers::display::DisplayDriver* display) {
  display_ = display;
  has_previous_frame_ = false;
  previous_frame_ = FaceFrame{};
  last_render_plan_ = ncos::services::display::DisplayRenderPlan{};
  render_stats_ = FaceRenderStats{};
  return display_ != nullptr;
}

bool FaceDisplayRenderer::render(const FaceFrame& frame) {
  if (display_ == nullptr) {
    return false;
  }

  const FaceFrame* previous = has_previous_frame_ ? &previous_frame_ : nullptr;
  last_render_plan_ = ncos::services::display::analyze_render_plan(
      previous, frame, static_cast<int16_t>(display_->width()), static_cast<int16_t>(display_->height()),
      render_mode_, display_->capability_profile());

  if (previous != nullptr && frames_equal(*previous, frame)) {
    render_stats_.last_frame_time_us = 0;
    render_stats_.last_dirty_area_px = 0;
    render_stats_.last_frame_skipped = true;
    render_stats_.last_frame_full_redraw = last_render_plan_.full_redraw;
    ++render_stats_.skipped_duplicate_frames;
    return true;
  }

  const uint64_t frame_start_us = monotonic_time_us();

  display_->setRotation(1);
  display_->startWrite();

  if (last_render_plan_.full_redraw) {
    display_->fillScreen(frame.background);

    if (frame.face_color != frame.background && frame.head_w > 0 && frame.head_h > 0) {
      display_->fillRoundRect(frame.head_x, frame.head_y, frame.head_w, frame.head_h,
                              frame.head_radius, frame.face_color);
    }
  } else {
    if (last_render_plan_.dirty_rect.valid) {
      display_->fillRect(last_render_plan_.dirty_rect.x, last_render_plan_.dirty_rect.y,
                         last_render_plan_.dirty_rect.w, last_render_plan_.dirty_rect.h,
                         frame.face_color);
    }
    if (last_render_plan_.dirty_rect_secondary.valid) {
      display_->fillRect(last_render_plan_.dirty_rect_secondary.x,
                         last_render_plan_.dirty_rect_secondary.y,
                         last_render_plan_.dirty_rect_secondary.w,
                         last_render_plan_.dirty_rect_secondary.h,
                         frame.face_color);
    }
  }

  const int16_t left_eye_x = static_cast<int16_t>(frame.left_eye_x - frame.eye_w / 2);
  const int16_t left_eye_y = static_cast<int16_t>(frame.left_eye_y - frame.eye_h / 2);
  const int16_t right_eye_x = static_cast<int16_t>(frame.right_eye_x - frame.eye_w / 2);
  const int16_t right_eye_y = static_cast<int16_t>(frame.right_eye_y - frame.eye_h / 2);

  display_->fillRoundRect(left_eye_x, left_eye_y, frame.eye_w, frame.eye_h, frame.eye_corner,
                          frame.eye_color);
  display_->fillRoundRect(right_eye_x, right_eye_y, frame.eye_w, frame.eye_h, frame.eye_corner,
                          frame.eye_color);

  const int16_t orientation_x = static_cast<int16_t>(display_->width() / 2);
  const int16_t orientation_y = static_cast<int16_t>(display_->height() - 8);
  display_->fillCircle(orientation_x, orientation_y, 3, OrientationDotColor);
  display_->endWrite();

  const uint64_t frame_end_us = monotonic_time_us();
  const uint32_t frame_time_us = static_cast<uint32_t>(frame_end_us - frame_start_us);
  const uint32_t primary_dirty_area_px = last_render_plan_.dirty_rect.valid
                                          ? static_cast<uint32_t>(last_render_plan_.dirty_rect.w) *
                                                static_cast<uint32_t>(last_render_plan_.dirty_rect.h)
                                          : 0;
  const uint32_t secondary_dirty_area_px = last_render_plan_.dirty_rect_secondary.valid
                                            ? static_cast<uint32_t>(last_render_plan_.dirty_rect_secondary.w) *
                                                  static_cast<uint32_t>(last_render_plan_.dirty_rect_secondary.h)
                                            : 0;
  const uint32_t dirty_area_px = primary_dirty_area_px + secondary_dirty_area_px;

  render_stats_.last_frame_time_us = frame_time_us;
  render_stats_.last_dirty_area_px = dirty_area_px;
  render_stats_.last_frame_skipped = false;
  render_stats_.last_frame_full_redraw = last_render_plan_.full_redraw;
  render_stats_.peak_frame_time_us =
      frame_time_us > render_stats_.peak_frame_time_us ? frame_time_us : render_stats_.peak_frame_time_us;
  render_stats_.avg_frame_time_us = render_stats_.rendered_frames == 0
                                        ? frame_time_us
                                        : static_cast<uint32_t>((render_stats_.avg_frame_time_us * 7u + frame_time_us) / 8u);
  ++render_stats_.rendered_frames;

  previous_frame_ = frame;
  has_previous_frame_ = true;
  return true;
}

void FaceDisplayRenderer::set_render_mode(ncos::services::display::DisplayRenderMode mode) {
  render_mode_ = mode;
}

ncos::services::display::DisplayRenderPlan FaceDisplayRenderer::last_render_plan() const {
  return last_render_plan_;
}

FaceRenderStats FaceDisplayRenderer::render_stats() const {
  return render_stats_;
}

}  // namespace ncos::services::face

