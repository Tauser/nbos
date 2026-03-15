#include "services/face/face_service.hpp"

namespace ncos::services::face {

bool FaceService::initialize(uint64_t now_ms) {
  return pipeline_.initialize(now_ms);
}

void FaceService::tick(uint64_t now_ms, const ncos::core::contracts::FaceMultimodalInput& multimodal) {
  pipeline_.tick(now_ms, multimodal);
}

size_t FaceService::export_preview_json(char* out_buffer, size_t out_buffer_size) const {
  return pipeline_.export_preview_json(out_buffer, out_buffer_size);
}

}  // namespace ncos::services::face
