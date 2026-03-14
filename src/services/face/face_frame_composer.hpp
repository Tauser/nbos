#pragma once

#include "core/contracts/face_render_state_contracts.hpp"
#include "services/face/face_frame.hpp"

namespace ncos::services::face {

class FaceFrameComposer final {
 public:
  bool compose(const ncos::core::contracts::FaceRenderState& state, FaceFrame* out_frame) const;
};

}  // namespace ncos::services::face
