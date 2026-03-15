#pragma once

#include <stdint.h>

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_clip_player.hpp"
#include "services/face/face_compositor.hpp"
#include "services/face/face_display_renderer.hpp"
#include "services/face/face_frame_composer.hpp"
#include "services/face/face_gaze_controller.hpp"
#include "services/face/face_preset_library.hpp"

namespace ncos::services::face {

class FaceGraphicsPipeline final {
 public:
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms);
  bool initialized() const;

 private:
  static constexpr uint16_t kPresetOwnerServiceId = 31;
  static constexpr uint16_t kGazeOwnerServiceId = 32;
  static constexpr uint16_t kClipOwnerServiceId = 33;
  static constexpr uint32_t kRenderPeriodMs = 120;

  bool initialized_ = false;
  uint64_t next_render_ms_ = 0;
  uint64_t next_gaze_target_ms_ = 0;
  uint64_t next_clip_start_ms_ = 0;
  bool gaze_left_ = false;

  ncos::core::contracts::FaceRenderState state_{};
  FaceExploratoryPresetId exploratory_preset_ = FaceExploratoryPresetId::kClarityNeutral;
  FaceCompositor compositor_{};
  FaceClipPlayer clip_player_{kClipOwnerServiceId};
  FaceGazeController gaze_controller_{kGazeOwnerServiceId};
  FaceFrameComposer composer_{};
  FaceDisplayRenderer renderer_{};
};

}  // namespace ncos::services::face
