#include "services/face/face_geometry.hpp"

namespace {

uint8_t clamp_percent(uint8_t value) {
  return value > 100 ? 100 : value;
}

int16_t clamp_i16(int32_t value, int16_t min_v, int16_t max_v) {
  if (value < min_v) {
    return min_v;
  }
  if (value > max_v) {
    return max_v;
  }
  return static_cast<int16_t>(value);
}

int16_t gaze_scale_percent(uint8_t focus_percent) {
  const uint8_t focus = clamp_percent(focus_percent);
  if (focus <= 20) {
    return static_cast<int16_t>(focus * 2);
  }
  if (focus <= 50) {
    return static_cast<int16_t>(40 + (static_cast<int32_t>(focus - 20) * 44) / 30);
  }
  return static_cast<int16_t>(84 + (static_cast<int32_t>(focus - 50) * 8) / 50);
}

int16_t scale_component(int16_t base, uint8_t focus_percent, bool diagonal) {
  if (base == 0) {
    return 0;
  }

  int32_t scale = gaze_scale_percent(focus_percent);
  if (diagonal) {
    scale = (scale * 70) / 100;
  }

  const int32_t magnitude = base < 0 ? -base : base;
  int32_t scaled = (magnitude * scale + 50) / 100;
  if (scaled == 0) {
    scaled = 1;
  }

  return static_cast<int16_t>(base < 0 ? -scaled : scaled);
}

int16_t gaze_dx(ncos::models::face::GazeDirection direction, uint8_t focus_percent) {
  switch (direction) {
    case ncos::models::face::GazeDirection::kLeft:
      return scale_component(-8, focus_percent, false);
    case ncos::models::face::GazeDirection::kRight:
      return scale_component(8, focus_percent, false);
    case ncos::models::face::GazeDirection::kUpLeft:
    case ncos::models::face::GazeDirection::kDownLeft:
      return scale_component(-8, focus_percent, true);
    case ncos::models::face::GazeDirection::kUpRight:
    case ncos::models::face::GazeDirection::kDownRight:
      return scale_component(8, focus_percent, true);
    default:
      return 0;
  }
}

int16_t gaze_dy(ncos::models::face::GazeDirection direction, uint8_t focus_percent) {
  switch (direction) {
    case ncos::models::face::GazeDirection::kUp:
      return scale_component(-5, focus_percent, false);
    case ncos::models::face::GazeDirection::kDown:
      return scale_component(5, focus_percent, false);
    case ncos::models::face::GazeDirection::kUpLeft:
    case ncos::models::face::GazeDirection::kUpRight:
      return scale_component(-5, focus_percent, true);
    case ncos::models::face::GazeDirection::kDownLeft:
    case ncos::models::face::GazeDirection::kDownRight:
      return scale_component(5, focus_percent, true);
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
      clamp_i16(102 + (static_cast<int32_t>(state.geometry.eye_spacing_percent) - 50) * 3 / 4, 82, 136);
  layout.eye_line_y =
      clamp_i16(112 + (static_cast<int32_t>(state.geometry.eye_line_height_percent) - 50) / 2, 92, 132);

  const int16_t base_eye_radius =
      clamp_i16(16 + (static_cast<int32_t>(state.geometry.eye_size_percent) - 50) / 5, 10, 22);
  const uint8_t openness = state.lids.openness_percent;
  layout.eye_radius = openness > 80 ? base_eye_radius : (openness > 40 ? base_eye_radius / 2 : 2);

  layout.eye_w = clamp_i16(66 + (static_cast<int32_t>(state.geometry.eye_size_percent) - 50) / 2, 50, 90);
  const int16_t base_eye_h = layout.eye_w;
  layout.eye_h = clamp_i16(base_eye_h * static_cast<int32_t>(openness) / 100, 2, base_eye_h);

  layout.gaze_dx = gaze_dx(state.eyes.direction, state.eyes.focus_percent);
  layout.gaze_dy = gaze_dy(state.eyes.direction, state.eyes.focus_percent);

  layout.mouth_w = 0;
  layout.mouth_h = 0;
  layout.mouth_y = 0;

  *out_layout = layout;
  return true;
}

}  // namespace ncos::services::face
