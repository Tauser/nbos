#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config/system_config.hpp"
#include "core/contracts/face_multimodal_contracts.hpp"
#include "core/contracts/face_render_state_contracts.hpp"
#include "core/contracts/motion_runtime_contracts.hpp"
#include "services/display/display_diagnostics.hpp"
#include "services/face/face_clip_player.hpp"
#include "services/face/face_compositor.hpp"
#include "services/face/face_display_renderer.hpp"
#include "services/face/face_frame_composer.hpp"
#include "services/face/face_gaze_controller.hpp"
#include "services/face/face_multimodal_sync.hpp"
#include "services/face/face_preset_library.hpp"
#include "services/face/face_tooling.hpp"

namespace ncos::services::face {

class FaceGraphicsPipeline final {
 public:
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms, const ncos::core::contracts::FaceMultimodalInput& multimodal);
  bool initialized() const;
  size_t export_preview_json(char* out_buffer, size_t out_buffer_size) const;
  ncos::core::contracts::MotionFaceSignal motion_signal() const;
  FaceRenderStats render_stats() const;

 private:
  void schedule_next_render(uint64_t now_ms);

  static constexpr uint16_t PresetOwnerServiceId = 31;
  static constexpr uint16_t GazeOwnerServiceId = 32;
  static constexpr uint16_t ClipOwnerServiceId = 33;
  static constexpr uint16_t ModulationOwnerServiceId = 34;
  static constexpr uint32_t RenderPeriodMs = 120;

  bool initialized_ = false;
  uint64_t next_render_ms_ = 0;
  uint64_t next_gaze_target_ms_ = 0;
  uint64_t next_clip_start_ms_ = 0;
  bool gaze_left_ = false;

  ncos::core::contracts::FaceRenderState state_{};
  FaceOfficialPresetId official_preset_ = FaceOfficialPresetId::kCoreNeutral;
  FaceCompositor compositor_{};
  FaceClipPlayer clip_player_{ClipOwnerServiceId};
  FaceGazeController gaze_controller_{GazeOwnerServiceId};
  FaceMultimodalSync multimodal_sync_{};
  FacePreviewSnapshot preview_snapshot_{};
  FaceTuningTelemetry tuning_{};
  ncos::core::contracts::MotionFaceSignal motion_signal_{};
  FaceFrameComposer composer_{};
  FaceDisplayRenderer renderer_{};
  ncos::services::display::DisplayDiagnosticsRunner diagnostics_runner_{};
  ncos::config::DisplayDiagnosticsMode diagnostics_mode_ = ncos::config::DisplayDiagnosticsMode::kOff;
};

}  // namespace ncos::services::face
