#pragma once

#include <stdint.h>

#include "services/face/face_graphics_pipeline.hpp"

namespace ncos::services::face {

class FaceService final {
 public:
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms);

 private:
  FaceGraphicsPipeline pipeline_{};
};

}  // namespace ncos::services::face
