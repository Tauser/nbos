#pragma once

#include "core/contracts/face_render_state_contracts.hpp"

namespace ncos::services::face {

struct FaceGeometryLayout {
  int16_t center_x = 120;
  int16_t center_y = 120;

  int16_t head_w = 146;
  int16_t head_h = 146;
  int16_t head_radius = 18;

  int16_t eye_spacing = 56;
  int16_t eye_line_y = 112;
  int16_t eye_radius = 12;
  int16_t eye_w = 34;
  int16_t eye_h = 24;

  int16_t pupil_dx = 0;
  int16_t pupil_dy = 0;
  int16_t pupil_radius = 6;

  int16_t mouth_w = 52;
  int16_t mouth_h = 4;
  int16_t mouth_y = 178;
};

bool make_face_geometry_layout(const ncos::core::contracts::FaceRenderState& state,
                               FaceGeometryLayout* out_layout);

}  // namespace ncos::services::face
