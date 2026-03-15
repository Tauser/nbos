#include "services/face/face_display_renderer.hpp"

namespace {

bool requires_full_redraw(const ncos::services::face::FaceFrame& prev,
                          const ncos::services::face::FaceFrame& next) {
  return prev.background != next.background || prev.face_color != next.face_color ||
         prev.head_x != next.head_x || prev.head_y != next.head_y || prev.head_w != next.head_w ||
         prev.head_h != next.head_h || prev.head_radius != next.head_radius;
}

}  // namespace

namespace ncos::services::face {

bool FaceDisplayRenderer::bind(ncos::drivers::display::DisplayDriver* display) {
  display_ = display;
  has_previous_frame_ = false;
  previous_frame_ = FaceFrame{};
  return display_ != nullptr;
}

bool FaceDisplayRenderer::render(const FaceFrame& frame) {
  if (display_ == nullptr) {
    return false;
  }

  display_->setRotation(0);

  const bool full_redraw = !has_previous_frame_ || requires_full_redraw(previous_frame_, frame);

  if (full_redraw) {
    display_->fillScreen(frame.background);

    if (frame.head_w > 0 && frame.head_h > 0) {
      display_->fillRoundRect(frame.head_x, frame.head_y, frame.head_w, frame.head_h,
                              frame.head_radius, frame.face_color);
    }
  } else {
    // Incremental redraw to avoid panel-wide flicker: erase dynamic elements only.
    display_->fillCircle(previous_frame_.left_eye_x, previous_frame_.left_eye_y,
                         static_cast<int16_t>(previous_frame_.eye_radius + 2), frame.face_color);
    display_->fillCircle(previous_frame_.right_eye_x, previous_frame_.right_eye_y,
                         static_cast<int16_t>(previous_frame_.eye_radius + 2), frame.face_color);
    display_->fillRoundRect(previous_frame_.mouth_x, previous_frame_.mouth_y, previous_frame_.mouth_w,
                            previous_frame_.mouth_h, 2, frame.face_color);
  }

  display_->fillCircle(frame.left_eye_x, frame.left_eye_y, frame.eye_radius, frame.eye_color);
  display_->fillCircle(frame.right_eye_x, frame.right_eye_y, frame.eye_radius, frame.eye_color);
  display_->fillRoundRect(frame.mouth_x, frame.mouth_y, frame.mouth_w, frame.mouth_h, 2,
                          frame.mouth_color);

  previous_frame_ = frame;
  has_previous_frame_ = true;
  return true;
}

}  // namespace ncos::services::face
