#include "services/face/face_frame_composer.hpp"

#include "services/face/face_geometry.hpp"

namespace {

int16_t clamp_i16_frame(int32_t value, int16_t min_v, int16_t max_v) {
  if (value < min_v) {
    return min_v;
  }
  if (value > max_v) {
    return max_v;
  }
  return static_cast<int16_t>(value);
}

}  // namespace

namespace ncos::services::face {

bool FaceFrameComposer::compose(const ncos::core::contracts::FaceRenderState& state,
                                FaceFrame* out_frame) const {
  if (out_frame == nullptr || !ncos::core::contracts::is_valid(state)) {
    return false;
  }

  FaceGeometryLayout layout{};
  if (!make_face_geometry_layout(state, &layout)) {
    return false;
  }

  FaceFrame frame{};
  frame.background = 0x0000;   // black
  frame.face_color = 0x0000;   // EMO-inspired mask blends with background
  frame.eye_color = 0xFFFF;    // white eyes
  frame.pupil_color = 0x0000;  // black pupils
  frame.mouth_color = 0xFFFF;  // white mouth

  frame.head_w = layout.head_w;
  frame.head_h = layout.head_h;
  frame.head_radius = layout.head_radius;
  frame.head_x = static_cast<int16_t>(layout.center_x - layout.head_w / 2);
  frame.head_y = static_cast<int16_t>(layout.center_y - layout.head_h / 2);

  frame.left_eye_x = static_cast<int16_t>(layout.center_x - layout.eye_spacing / 2);
  frame.left_eye_y = layout.eye_line_y;
  frame.right_eye_x = static_cast<int16_t>(layout.center_x + layout.eye_spacing / 2);
  frame.right_eye_y = layout.eye_line_y;
  frame.eye_radius = layout.eye_radius;
  frame.eye_w = layout.eye_w;
  frame.eye_h = layout.eye_h;
  frame.eye_corner = clamp_i16_frame(layout.eye_h / 2, 1, 14);

  const int16_t max_pupil_dx = clamp_i16_frame(layout.eye_w / 2 - layout.pupil_radius - 2, 0, 16);
  const int16_t max_pupil_dy = clamp_i16_frame(layout.eye_h / 2 - layout.pupil_radius - 2, 0, 12);
  const int16_t pupil_dx = clamp_i16_frame(layout.pupil_dx, -max_pupil_dx, max_pupil_dx);
  const int16_t pupil_dy = clamp_i16_frame(layout.pupil_dy, -max_pupil_dy, max_pupil_dy);

  frame.left_pupil_x = static_cast<int16_t>(frame.left_eye_x + pupil_dx);
  frame.left_pupil_y = static_cast<int16_t>(frame.left_eye_y + pupil_dy);
  frame.right_pupil_x = static_cast<int16_t>(frame.right_eye_x + pupil_dx);
  frame.right_pupil_y = static_cast<int16_t>(frame.right_eye_y + pupil_dy);
  frame.pupil_radius = layout.eye_h <= 4 ? 0 : layout.pupil_radius;

  frame.mouth_w = layout.mouth_w;
  frame.mouth_h = layout.mouth_h;
  frame.mouth_y = layout.mouth_y;
  frame.mouth_x = static_cast<int16_t>(layout.center_x - layout.mouth_w / 2);
  frame.mouth_corner = clamp_i16_frame(layout.mouth_h / 2, 1, 8);

  *out_frame = frame;
  return true;
}

}  // namespace ncos::services::face
