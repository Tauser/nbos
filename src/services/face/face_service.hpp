#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/face_multimodal_contracts.hpp"
#include "services/face/face_graphics_pipeline.hpp"

namespace ncos::services::face {

class FaceService final {
 public:
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms, const ncos::core::contracts::FaceMultimodalInput& multimodal);
  size_t export_preview_json(char* out_buffer, size_t out_buffer_size) const;

 private:
  FaceGraphicsPipeline pipeline_{};
};

}  // namespace ncos::services::face
