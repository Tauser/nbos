#include "services/face/face_frame_composer.hpp"

namespace {
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

  frame.left_eye_x = static_cast<int16_t>(80 + dx);
  frame.left_eye_y = static_cast<int16_t>(112 + dy);
  frame.right_eye_x = static_cast<int16_t>(160 + dx);
  frame.right_eye_y = static_cast<int16_t>(112 + dy);

  // Minimal blink mapping keeps renderer semantic-free.
  const uint8_t openness = state.lids.openness_percent;
  frame.eye_radius = openness > 80 ? 14 : (openness > 40 ? 8 : 3);

  frame.mouth_x = 88;
  frame.mouth_y = 176;
  frame.mouth_w = 64;
  frame.mouth_h = state.mouth.openness_percent > 25 ? 8 : 3;

  *out_frame = frame;
  return true;
}

}  // namespace ncos::services::face
