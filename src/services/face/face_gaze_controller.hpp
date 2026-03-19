#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "models/face/face_gaze_target.hpp"

namespace ncos::services::face {

enum class FaceGazeMotionPhase : uint8_t {
  kIdle = 0,
  kSaccade = 1,
  kOvershoot = 2,
  kSettle = 3,
  kFixation = 4,
};

class FaceGazeController final {
 public:
  explicit FaceGazeController(uint16_t owner_service = 0);

  void bind_owner(uint16_t owner_service);
  bool set_target(const ncos::models::face::FaceGazeTarget& target, uint64_t now_ms);
  void clear_target();

  bool tick(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state);

 private:
  bool owner_can_write(const ncos::core::contracts::FaceRenderState& state) const;

  bool active_target_ = false;
  uint16_t owner_service_ = 0;

  ncos::models::face::FaceGazeTarget target_{};
  ncos::models::face::GazeDirection current_direction_ = ncos::models::face::GazeDirection::kCenter;

  ncos::models::face::GazeDirection start_direction_ = ncos::models::face::GazeDirection::kCenter;
  ncos::models::face::GazeDirection overshoot_direction_ = ncos::models::face::GazeDirection::kCenter;

  FaceGazeMotionPhase phase_ = FaceGazeMotionPhase::kIdle;
  uint64_t saccade_start_ms_ = 0;
  uint64_t saccade_end_ms_ = 0;
  uint64_t overshoot_end_ms_ = 0;
  uint64_t settle_end_ms_ = 0;
  uint64_t fixation_end_ms_ = 0;
};

}  // namespace ncos::services::face
