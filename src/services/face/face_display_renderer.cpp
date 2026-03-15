#include "services/face/face_display_renderer.hpp"

namespace {

bool requires_full_redraw(const ncos::services::face::FaceFrame& prev,
                          const ncos::services::face::FaceFrame& next) {
  return prev.background != next.background || prev.face_color != next.face_color ||
         prev.head_x != next.head_x || prev.head_y != next.head_y || prev.head_w != next.head_w ||
         prev.head_h != next.head_h || prev.head_radius != next.head_radius;
}

void erase_eye(ncos::drivers::display::DisplayDriver* display,
               const ncos::services::face::FaceFrame& frame,
               int16_t eye_x,
               int16_t eye_y) {
  const int16_t erase_w = static_cast<int16_t>(frame.eye_w + 8);
  const int16_t erase_h = static_cast<int16_t>(frame.eye_h + 8);
  const int16_t erase_x = static_cast<int16_t>(eye_x - erase_w / 2);
  const int16_t erase_y = static_cast<int16_t>(eye_y - erase_h / 2);
  const int16_t erase_corner = static_cast<int16_t>((frame.eye_corner + 4) > 16 ? 16 : (frame.eye_corner + 4));
  display->fillRoundRect(erase_x, erase_y, erase_w, erase_h, erase_corner, frame.face_color);
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

    if (frame.face_color != frame.background && frame.head_w > 0 && frame.head_h > 0) {
      display_->fillRoundRect(frame.head_x, frame.head_y, frame.head_w, frame.head_h,
                              frame.head_radius, frame.face_color);
    }
  } else {
    // Erase only dynamic visual elements to keep frame stable and avoid full-screen flicker.
    erase_eye(display_, frame, previous_frame_.left_eye_x, previous_frame_.left_eye_y);
    erase_eye(display_, frame, previous_frame_.right_eye_x, previous_frame_.right_eye_y);
    display_->fillRoundRect(previous_frame_.mouth_x - 2, previous_frame_.mouth_y - 2,
                            previous_frame_.mouth_w + 4, previous_frame_.mouth_h + 4,
                            previous_frame_.mouth_corner + 2, frame.face_color);
  }

  const int16_t left_eye_x = static_cast<int16_t>(frame.left_eye_x - frame.eye_w / 2);
  const int16_t left_eye_y = static_cast<int16_t>(frame.left_eye_y - frame.eye_h / 2);
  const int16_t right_eye_x = static_cast<int16_t>(frame.right_eye_x - frame.eye_w / 2);
  const int16_t right_eye_y = static_cast<int16_t>(frame.right_eye_y - frame.eye_h / 2);

  display_->fillRoundRect(left_eye_x, left_eye_y, frame.eye_w, frame.eye_h, frame.eye_corner,
                          frame.eye_color);
  display_->fillRoundRect(right_eye_x, right_eye_y, frame.eye_w, frame.eye_h, frame.eye_corner,
                          frame.eye_color);

  if (frame.pupil_radius > 0) {
    display_->fillCircle(frame.left_pupil_x, frame.left_pupil_y, frame.pupil_radius,
                         frame.pupil_color);
    display_->fillCircle(frame.right_pupil_x, frame.right_pupil_y, frame.pupil_radius,
                         frame.pupil_color);
  }

  display_->fillRoundRect(frame.mouth_x, frame.mouth_y, frame.mouth_w, frame.mouth_h,
                          frame.mouth_corner, frame.mouth_color);

  previous_frame_ = frame;
  has_previous_frame_ = true;
  return true;
}

}  // namespace ncos::services::face
