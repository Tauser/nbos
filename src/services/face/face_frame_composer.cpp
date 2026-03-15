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
  frame.face_color = 0x0000;   // hidden mask
  frame.eye_color = 0x061F;    // cyan
  frame.pupil_color = 0x0000;
  frame.mouth_color = 0x0000;

  frame.head_w = layout.head_w;
  frame.head_h = layout.head_h;
  frame.head_radius = layout.head_radius;
  frame.head_x = static_cast<int16_t>(layout.center_x - layout.head_w / 2);
  frame.head_y = static_cast<int16_t>(layout.center_y - layout.head_h / 2);

  frame.left_eye_x = static_cast<int16_t>(layout.center_x - layout.eye_spacing / 2 + layout.gaze_dx);
  frame.left_eye_y = static_cast<int16_t>(layout.eye_line_y + layout.gaze_dy);
  frame.right_eye_x = static_cast<int16_t>(layout.center_x + layout.eye_spacing / 2 + layout.gaze_dx);
  frame.right_eye_y = static_cast<int16_t>(layout.eye_line_y + layout.gaze_dy);
  frame.eye_radius = layout.eye_radius;
  frame.eye_w = layout.eye_w;
  frame.eye_h = layout.eye_h;
  frame.eye_corner = clamp_i16_frame(layout.eye_h / 4, 5, 14);

  frame.left_pupil_x = frame.left_eye_x;
  frame.left_pupil_y = frame.left_eye_y;
  frame.right_pupil_x = frame.right_eye_x;
  frame.right_pupil_y = frame.right_eye_y;
  frame.pupil_radius = 0;

  frame.mouth_w = 0;
  frame.mouth_h = 0;
  frame.mouth_y = 0;
  frame.mouth_x = 0;
  frame.mouth_corner = 0;

  *out_frame = frame;
  return true;
}

}  // namespace ncos::services::face
