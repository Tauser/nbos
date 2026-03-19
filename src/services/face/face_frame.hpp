#pragma once

#include <stdint.h>

namespace ncos::services::face {

struct FaceFrame {
  uint16_t background = 0;
  uint16_t face_color = 0;
  uint16_t eye_color = 0;
  uint16_t pupil_color = 0;
  uint16_t mouth_color = 0;

  int16_t head_x = 0;
  int16_t head_y = 0;
  int16_t head_w = 0;
  int16_t head_h = 0;
  int16_t head_radius = 0;

  int16_t left_eye_x = 0;
  int16_t left_eye_y = 0;
  int16_t right_eye_x = 0;
  int16_t right_eye_y = 0;
  int16_t eye_radius = 0;
  int16_t eye_w = 0;
  int16_t eye_h = 0;
  int16_t eye_corner = 0;
  int16_t left_eye_radius = 0;
  int16_t left_eye_w = 0;
  int16_t left_eye_h = 0;
  int16_t left_eye_corner = 0;
  int16_t right_eye_radius = 0;
  int16_t right_eye_w = 0;
  int16_t right_eye_h = 0;
  int16_t right_eye_corner = 0;

  int16_t left_pupil_x = 0;
  int16_t left_pupil_y = 0;
  int16_t right_pupil_x = 0;
  int16_t right_pupil_y = 0;
  int16_t pupil_radius = 0;

  int16_t mouth_x = 0;
  int16_t mouth_y = 0;
  int16_t mouth_w = 0;
  int16_t mouth_h = 0;
  int16_t mouth_corner = 0;
};

}  // namespace ncos::services::face
