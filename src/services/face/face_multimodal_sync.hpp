#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/face_multimodal_contracts.hpp"
#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_compositor.hpp"

namespace ncos::services::face {

struct FaceMultimodalSyncResult {
  bool applied = false;
  uint8_t target_focus_percent = 0;
  uint8_t target_lid_open_percent = 100;
  uint8_t target_brow_intensity_percent = 0;
  ncos::models::face::BlinkPhase target_blink_phase = ncos::models::face::BlinkPhase::kOpen;
};

class FaceMultimodalSync final {
 public:
  bool apply(const ncos::core::contracts::FaceMultimodalInput& input,
             FaceCompositor* compositor,
             ncos::core::contracts::FaceRenderState* state,
             uint16_t owner_service,
             uint64_t now_ms,
             FaceMultimodalSyncResult* out_result = nullptr);
};

}  // namespace ncos::services::face
