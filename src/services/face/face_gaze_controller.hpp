#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "models/face/face_gaze_target.hpp"

namespace ncos::services::face {

class FaceGazeController final {
 public:
  explicit FaceGazeController(uint16_t owner_service = 0);

  void bind_owner(uint16_t owner_service);
  bool set_target(const ncos::models::face::FaceGazeTarget& target, uint64_t now_ms);
  void clear_target();

  bool tick(uint64_t now_ms, ncos::core::contracts::FaceRenderState* state);

 private:
  bool owner_can_write(const ncos::core::contracts::FaceRenderState& state) const;

  uint16_t owner_service_ = 0;
  bool active_target_ = false;
  uint64_t expires_at_ms_ = 0;
  ncos::models::face::FaceGazeTarget target_{};
};

}  // namespace ncos::services::face
