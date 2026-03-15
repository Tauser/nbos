#include "services/face/face_geometry.hpp"

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
      return -6;
    case ncos::models::face::GazeDirection::kRight:
    case ncos::models::face::GazeDirection::kUpRight:
    case ncos::models::face::GazeDirection::kDownRight:
      return 6;
    default:
      return 0;
  }
}

int16_t gaze_dy(ncos::models::face::GazeDirection direction) {
  switch (direction) {
    case ncos::models::face::GazeDirection::kUp:
    case ncos::models::face::GazeDirection::kUpLeft:
    case ncos::models::face::GazeDirection::kUpRight:
      return -4;
    case ncos::models::face::GazeDirection::kDown:
    case ncos::models::face::GazeDirection::kDownLeft:
    case ncos::models::face::GazeDirection::kDownRight:
      return 4;
    default:
      return 0;
  }
}

}  // namespace

namespace ncos::services::face {

bool make_face_geometry_layout(const ncos::core::contracts::FaceRenderState& state,
                               FaceGeometryLayout* out_layout) {
  if (out_layout == nullptr || !ncos::core::contracts::is_valid(state)) {
    return false;
  }

  FaceGeometryLayout layout{};

  layout.head_w = clamp_i16(146 + (static_cast<int32_t>(state.geometry.jaw_width_percent) - 50) * 3 / 5,
                            122, 182);
  layout.head_h = clamp_i16(
      146 + (50 - static_cast<int32_t>(state.geometry.jaw_width_percent)) / 2 +
          (static_cast<int32_t>(state.geometry.silhouette_roundness_percent) - 50) / 4,
      132, 184);
  layout.head_radius =
      clamp_i16(18 + (static_cast<int32_t>(state.geometry.silhouette_roundness_percent) - 50) / 4,
                8, 30);

  layout.eye_spacing =
      clamp_i16(56 + (static_cast<int32_t>(state.geometry.eye_spacing_percent) - 50) / 2, 42, 72);
  layout.eye_line_y =
      clamp_i16(112 + (static_cast<int32_t>(state.geometry.eye_line_height_percent) - 50) / 2, 92, 132);

  const int16_t base_eye_radius =
      clamp_i16(12 + (static_cast<int32_t>(state.geometry.eye_size_percent) - 50) / 4, 8, 18);
  const uint8_t openness = state.lids.openness_percent;
  layout.eye_radius = openness > 80 ? base_eye_radius : (openness > 40 ? base_eye_radius / 2 : 3);

  layout.eye_w = clamp_i16(30 + (static_cast<int32_t>(state.geometry.eye_size_percent) - 50) / 2, 24, 42);
  const int16_t base_eye_h = clamp_i16(22 + (static_cast<int32_t>(state.geometry.eye_size_percent) - 50) / 3,
                                       16, 30);
  layout.eye_h = clamp_i16(base_eye_h * static_cast<int32_t>(openness) / 100, 3, base_eye_h);

  layout.pupil_dx = gaze_dx(state.eyes.direction);
  layout.pupil_dy = gaze_dy(state.eyes.direction);
  layout.pupil_radius = clamp_i16(layout.eye_h / 4 + 2, 2, 7);

  layout.mouth_w =
      clamp_i16(52 + (static_cast<int32_t>(state.geometry.mouth_width_percent) - 50) / 2, 34, 74);

  const int16_t mouth_base_h =
      clamp_i16(3 + (static_cast<int32_t>(state.geometry.mouth_height_percent) - 50) / 10, 2, 6);
  layout.mouth_h = state.mouth.openness_percent > 25
                       ? clamp_i16(mouth_base_h + 3, 4, 10)
                       : mouth_base_h;

  layout.mouth_y =
      clamp_i16(178 + (50 - static_cast<int32_t>(state.geometry.brow_height_percent)) / 3, 156, 194);

  *out_layout = layout;
  return true;
}

}  // namespace ncos::services::face
