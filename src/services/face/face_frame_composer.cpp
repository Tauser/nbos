#include "services/face/face_frame_composer.hpp"

namespace {

int16_t clamp_i16(int32_t value, int16_t min_v, int16_t max_v) {
  if (value < min_v) {
    return min_v;
  }
  if (value > max_v) {
    return max_v;
  }
  return static_cast<int16_t>(value);
}

int16_t gaze_dx(ncos::models::face::GazeDirection direction) {
  switch (direction) {
    case ncos::models::face::GazeDirection::kLeft:
    case ncos::models::face::GazeDirection::kUpLeft:
    case ncos::models::face::GazeDirection::kDownLeft:
      return -8;
    case ncos::models::face::GazeDirection::kRight:
    case ncos::models::face::GazeDirection::kUpRight:
    case ncos::models::face::GazeDirection::kDownRight:
      return 8;
    default:
      return 0;
  }
}

int16_t gaze_dy(ncos::models::face::GazeDirection direction) {
  switch (direction) {
    case ncos::models::face::GazeDirection::kUp:
    case ncos::models::face::GazeDirection::kUpLeft:
    case ncos::models::face::GazeDirection::kUpRight:
      return -6;
    case ncos::models::face::GazeDirection::kDown:
    case ncos::models::face::GazeDirection::kDownLeft:
    case ncos::models::face::GazeDirection::kDownRight:
      return 6;
    default:
      return 0;
  }
}

}  // namespace

namespace ncos::services::face {

bool FaceFrameComposer::compose(const ncos::core::contracts::FaceRenderState& state,
                                FaceFrame* out_frame) const {
  if (out_frame == nullptr || !ncos::core::contracts::is_valid(state)) {
    return false;
  }

  FaceFrame frame{};
  frame.background = 0x0000;   // black
  frame.eye_color = 0xFFFF;    // white
  frame.mouth_color = 0xFFFF;  // white

  const int16_t dx = gaze_dx(state.eyes.direction);
  const int16_t dy = gaze_dy(state.eyes.direction);

  const int16_t eye_spacing = clamp_i16(56 + (static_cast<int32_t>(state.geometry.eye_spacing_percent) - 50) / 2,
                                        42, 72);
  const int16_t eye_center_x = 120;
  const int16_t eye_line_y = clamp_i16(112 + (static_cast<int32_t>(state.geometry.eye_line_height_percent) - 50) / 2,
                                       92, 132);

  frame.left_eye_x = static_cast<int16_t>(eye_center_x - eye_spacing / 2 + dx);
  frame.left_eye_y = static_cast<int16_t>(eye_line_y + dy);
  frame.right_eye_x = static_cast<int16_t>(eye_center_x + eye_spacing / 2 + dx);
  frame.right_eye_y = static_cast<int16_t>(eye_line_y + dy);

  const int16_t base_eye_radius = clamp_i16(12 + (static_cast<int32_t>(state.geometry.eye_size_percent) - 50) / 4,
                                            8, 18);
  const uint8_t openness = state.lids.openness_percent;
  frame.eye_radius = openness > 80 ? base_eye_radius : (openness > 40 ? base_eye_radius / 2 : 3);

  frame.mouth_x = static_cast<int16_t>(120 - clamp_i16(52 + (static_cast<int32_t>(state.geometry.mouth_width_percent) - 50) / 2,
                                                       34, 74) /
                                       2);
  frame.mouth_y = clamp_i16(178 + (50 - static_cast<int32_t>(state.geometry.brow_height_percent)) / 3, 156,
                            194);
  frame.mouth_w = clamp_i16(52 + (static_cast<int32_t>(state.geometry.mouth_width_percent) - 50) / 2, 34, 74);

  const int16_t mouth_base_h = clamp_i16(4 + (static_cast<int32_t>(state.geometry.mouth_height_percent) - 50) / 8,
                                         3, 9);
  frame.mouth_h = state.mouth.openness_percent > 25 ? mouth_base_h + 4 : mouth_base_h;

  *out_frame = frame;
  return true;
}

}  // namespace ncos::services::face
