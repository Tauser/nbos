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

uint8_t clamp_percent_frame(int32_t value) {
  if (value < 0) {
    return 0;
  }
  if (value > 100) {
    return 100;
  }
  return static_cast<uint8_t>(value);
}

int16_t scale_dimension(int16_t base, int8_t delta_percent, int16_t min_v, int16_t max_v) {
  const int32_t scaled = (static_cast<int32_t>(base) * (100 + delta_percent) + 50) / 100;
  return clamp_i16_frame(scaled, min_v, max_v);
}

int16_t resolve_eye_radius(int16_t base_radius, uint8_t openness_percent, int8_t size_delta_percent) {
  const int16_t scaled_base_radius = scale_dimension(base_radius, size_delta_percent, 2, 30);
  if (openness_percent > 80) {
    return scaled_base_radius;
  }
  if (openness_percent > 40) {
    return clamp_i16_frame(scaled_base_radius / 2, 2, 30);
  }
  return 2;
}

int16_t resolve_eye_height(int16_t base_width, uint8_t openness_percent, int8_t size_delta_percent) {
  const int16_t scaled_width = scale_dimension(base_width, size_delta_percent, 40, 96);
  return clamp_i16_frame((static_cast<int32_t>(scaled_width) * openness_percent) / 100, 2, scaled_width);
}

bool is_lateral_direction(ncos::models::face::GazeDirection direction) {
  return direction == ncos::models::face::GazeDirection::kLeft ||
         direction == ncos::models::face::GazeDirection::kRight;
}

int16_t lateral_parallax_shift_px(ncos::models::face::GazeDirection direction, uint8_t focus_percent) {
  if (!is_lateral_direction(direction) || focus_percent < 80) {
    return 0;
  }
  return direction == ncos::models::face::GazeDirection::kLeft ? -1 : 1;
}

int8_t leading_eye_parallax_size_delta_percent(ncos::models::face::GazeDirection direction,
                                                uint8_t focus_percent) {
  if (!is_lateral_direction(direction) || focus_percent < 40) {
    return 0;
  }
  return focus_percent >= 70 ? 4 : 2;
}

int8_t trailing_eye_parallax_size_delta_percent(ncos::models::face::GazeDirection direction,
                                                 uint8_t focus_percent) {
  if (!is_lateral_direction(direction) || focus_percent < 40) {
    return 0;
  }
  return focus_percent >= 70 ? -2 : -1;
}

int8_t compose_size_delta(int8_t manual_delta, int8_t parallax_delta) {
  return static_cast<int8_t>(clamp_i16_frame(static_cast<int16_t>(manual_delta) + parallax_delta, -20, 20));
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

  const auto& left_adjust = state.eyes.left_adjust;
  const auto& right_adjust = state.eyes.right_adjust;
  const int16_t parallax_shift = lateral_parallax_shift_px(state.eyes.direction, state.eyes.focus_percent);
  const int8_t leading_size_delta =
      leading_eye_parallax_size_delta_percent(state.eyes.direction, state.eyes.focus_percent);
  const int8_t trailing_size_delta =
      trailing_eye_parallax_size_delta_percent(state.eyes.direction, state.eyes.focus_percent);

  const int8_t left_size_delta = compose_size_delta(
      left_adjust.size_delta_percent,
      state.eyes.direction == ncos::models::face::GazeDirection::kLeft ? leading_size_delta :
      state.eyes.direction == ncos::models::face::GazeDirection::kRight ? trailing_size_delta : 0);
  const int8_t right_size_delta = compose_size_delta(
      right_adjust.size_delta_percent,
      state.eyes.direction == ncos::models::face::GazeDirection::kRight ? leading_size_delta :
      state.eyes.direction == ncos::models::face::GazeDirection::kLeft ? trailing_size_delta : 0);

  const uint8_t left_openness =
      clamp_percent_frame(static_cast<int32_t>(state.lids.openness_percent) + left_adjust.openness_delta_percent);
  const uint8_t right_openness =
      clamp_percent_frame(static_cast<int32_t>(state.lids.openness_percent) + right_adjust.openness_delta_percent);

  frame.left_eye_x = static_cast<int16_t>(layout.center_x - layout.eye_spacing / 2 + layout.gaze_dx +
                                          left_adjust.x_offset_px +
                                          (state.eyes.direction == ncos::models::face::GazeDirection::kLeft ?
                                               parallax_shift : 0));
  frame.left_eye_y = static_cast<int16_t>(layout.eye_line_y + layout.gaze_dy + left_adjust.y_offset_px);
  frame.right_eye_x = static_cast<int16_t>(layout.center_x + layout.eye_spacing / 2 + layout.gaze_dx +
                                           right_adjust.x_offset_px +
                                           (state.eyes.direction == ncos::models::face::GazeDirection::kRight ?
                                                parallax_shift : 0));
  frame.right_eye_y = static_cast<int16_t>(layout.eye_line_y + layout.gaze_dy + right_adjust.y_offset_px);

  frame.eye_radius = layout.eye_radius;
  frame.eye_w = layout.eye_w;
  frame.eye_h = layout.eye_h;
  frame.eye_corner = clamp_i16_frame(layout.eye_h / 4, 5, 14);

  frame.left_eye_w = scale_dimension(layout.eye_w, left_size_delta, 40, 96);
  frame.left_eye_h = resolve_eye_height(layout.eye_w, left_openness, left_size_delta);
  frame.left_eye_radius = resolve_eye_radius(layout.eye_radius, left_openness, left_size_delta);
  frame.left_eye_corner = clamp_i16_frame(frame.left_eye_h / 4, 5, 14);

  frame.right_eye_w = scale_dimension(layout.eye_w, right_size_delta, 40, 96);
  frame.right_eye_h = resolve_eye_height(layout.eye_w, right_openness, right_size_delta);
  frame.right_eye_radius = resolve_eye_radius(layout.eye_radius, right_openness, right_size_delta);
  frame.right_eye_corner = clamp_i16_frame(frame.right_eye_h / 4, 5, 14);

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
