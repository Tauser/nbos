#include "services/face/face_display_renderer.hpp"

namespace ncos::services::face {

bool FaceDisplayRenderer::bind(ncos::drivers::display::DisplayDriver* display) {
  display_ = display;
  return display_ != nullptr;
}

bool FaceDisplayRenderer::render(const FaceFrame& frame) const {
  if (display_ == nullptr) {
    return false;
  }

  display_->setRotation(0);
  display_->fillScreen(frame.background);
  display_->fillCircle(frame.left_eye_x, frame.left_eye_y, frame.eye_radius, frame.eye_color);
  display_->fillCircle(frame.right_eye_x, frame.right_eye_y, frame.eye_radius, frame.eye_color);
  display_->fillRoundRect(frame.mouth_x, frame.mouth_y, frame.mouth_w, frame.mouth_h, 2,
                          frame.mouth_color);
  return true;
}

}  // namespace ncos::services::face
