#pragma once

#include <stdint.h>

namespace ncos::services::face {

struct FaceFrame {
  uint16_t background = 0;
  uint16_t eye_color = 0;
  uint16_t mouth_color = 0;

  int16_t left_eye_x = 0;
  int16_t left_eye_y = 0;
  int16_t right_eye_x = 0;
  int16_t right_eye_y = 0;
  int16_t eye_radius = 0;

  int16_t mouth_x = 0;
  int16_t mouth_y = 0;
  int16_t mouth_w = 0;
  int16_t mouth_h = 0;
};

}  // namespace ncos::services::face
