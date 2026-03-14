#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_display_renderer.hpp"
#include "services/face/face_frame_composer.hpp"
#include "services/face/face_gaze_controller.hpp"

namespace ncos::services::face {

class FaceGraphicsPipeline final {
 public:
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms);
  bool initialized() const;

 private:
  static constexpr uint16_t kFaceServiceId = 31;
  static constexpr uint32_t kRenderPeriodMs = 120;

  bool initialized_ = false;
  uint64_t next_render_ms_ = 0;
  uint64_t next_gaze_target_ms_ = 0;
  bool gaze_left_ = false;

  ncos::core::contracts::FaceRenderState state_{};
  FaceGazeController gaze_controller_{kFaceServiceId};
  FaceFrameComposer composer_{};
  FaceDisplayRenderer renderer_{};
};

}  // namespace ncos::services::face
