#include "services/face/face_display_renderer.hpp"

namespace {

constexpr uint16_t OrientationDotColor = 0xF800;

}  // namespace

namespace ncos::services::face {

bool FaceDisplayRenderer::bind(ncos::drivers::display::DisplayDriver* display) {
  display_ = display;
  has_previous_frame_ = false;
  previous_frame_ = FaceFrame{};
  last_render_plan_ = ncos::services::display::DisplayRenderPlan{};
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

  display_->setRotation(1);
  display_->startWrite();

  if (last_render_plan_.full_redraw) {
    display_->fillScreen(frame.background);

    if (frame.face_color != frame.background && frame.head_w > 0 && frame.head_h > 0) {
      display_->fillRoundRect(frame.head_x, frame.head_y, frame.head_w, frame.head_h,
                              frame.head_radius, frame.face_color);
    }
  } else if (last_render_plan_.dirty_rect.valid) {
    display_->fillRect(last_render_plan_.dirty_rect.x, last_render_plan_.dirty_rect.y,
                       last_render_plan_.dirty_rect.w, last_render_plan_.dirty_rect.h,
                       frame.face_color);
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

}  // namespace ncos::services::face

