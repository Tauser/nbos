#include "services/face/face_frame_composer.hpp"

#include "services/face/face_geometry.hpp"

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
  frame.face_color = 0x0841;   // dark gray-blue
  frame.eye_color = 0xFFFF;    // white
  frame.mouth_color = 0xFFFF;  // white

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

  frame.mouth_w = layout.mouth_w;
  frame.mouth_h = layout.mouth_h;
  frame.mouth_y = layout.mouth_y;
  frame.mouth_x = static_cast<int16_t>(layout.center_x - layout.mouth_w / 2);

  *out_frame = frame;
  return true;
}

}  // namespace ncos::services::face
