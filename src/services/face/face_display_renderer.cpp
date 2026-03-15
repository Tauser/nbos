#include "services/face/face_display_renderer.hpp"

namespace {

constexpr uint16_t kOrientationDotColor = 0xF800;
constexpr int16_t kEyeDirtyPadding = 16;

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

bool requires_full_redraw(const ncos::services::face::FaceFrame& prev,
                          const ncos::services::face::FaceFrame& next) {
  return prev.background != next.background || prev.face_color != next.face_color ||
         prev.head_x != next.head_x || prev.head_y != next.head_y || prev.head_w != next.head_w ||
         prev.head_h != next.head_h || prev.head_radius != next.head_radius;
}

EyeRect make_eye_rect(const ncos::services::face::FaceFrame& frame, bool left_eye) {
  const int16_t eye_x = left_eye ? frame.left_eye_x : frame.right_eye_x;
  const int16_t eye_y = left_eye ? frame.left_eye_y : frame.right_eye_y;

  EyeRect rect{};
  rect.w = static_cast<int16_t>(frame.eye_w + kEyeDirtyPadding);
  rect.h = static_cast<int16_t>(frame.eye_h + kEyeDirtyPadding);
  rect.x = static_cast<int16_t>(eye_x - rect.w / 2);
  rect.y = static_cast<int16_t>(eye_y - rect.h / 2);
  return rect;
}

void clear_dirty_eye_region(ncos::drivers::display::DisplayDriver* display,
                            const ncos::services::face::FaceFrame& previous_frame,
                            const ncos::services::face::FaceFrame& current_frame) {
  const EyeRect prev_left = make_eye_rect(previous_frame, true);
  const EyeRect prev_right = make_eye_rect(previous_frame, false);
  const EyeRect next_left = make_eye_rect(current_frame, true);
  const EyeRect next_right = make_eye_rect(current_frame, false);

  int16_t x0 = min_i16(min_i16(prev_left.x, prev_right.x), min_i16(next_left.x, next_right.x));
  int16_t y0 = min_i16(min_i16(prev_left.y, prev_right.y), min_i16(next_left.y, next_right.y));
  int16_t x1 = max_i16(max_i16(static_cast<int16_t>(prev_left.x + prev_left.w),
                               static_cast<int16_t>(prev_right.x + prev_right.w)),
                       max_i16(static_cast<int16_t>(next_left.x + next_left.w),
                               static_cast<int16_t>(next_right.x + next_right.w)));
  int16_t y1 = max_i16(max_i16(static_cast<int16_t>(prev_left.y + prev_left.h),
                               static_cast<int16_t>(prev_right.y + prev_right.h)),
                       max_i16(static_cast<int16_t>(next_left.y + next_left.h),
                               static_cast<int16_t>(next_right.y + next_right.h)));

  x0 = x0 < 0 ? 0 : x0;
  y0 = y0 < 0 ? 0 : y0;
  x1 = x1 > display->width() ? display->width() : x1;
  y1 = y1 > display->height() ? display->height() : y1;

  if (x1 > x0 && y1 > y0) {
    const int16_t dirty_w = static_cast<int16_t>(x1 - x0);
    const int16_t dirty_h = static_cast<int16_t>(y1 - y0);
    display->fillRect(x0, y0, dirty_w, dirty_h, current_frame.face_color);
  }
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

  display_->setRotation(1);
  display_->startWrite();

  const bool full_redraw = !has_previous_frame_ || requires_full_redraw(previous_frame_, frame);

  if (full_redraw) {
    display_->fillScreen(frame.background);

    if (frame.face_color != frame.background && frame.head_w > 0 && frame.head_h > 0) {
      display_->fillRoundRect(frame.head_x, frame.head_y, frame.head_w, frame.head_h,
                              frame.head_radius, frame.face_color);
    }
  } else {
    clear_dirty_eye_region(display_, previous_frame_, frame);
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
  display_->fillCircle(orientation_x, orientation_y, 3, kOrientationDotColor);
  display_->endWrite();

  previous_frame_ = frame;
  has_previous_frame_ = true;
  return true;
}

}  // namespace ncos::services::face
