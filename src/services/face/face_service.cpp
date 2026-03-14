#include "services/face/face_service.hpp"

namespace ncos::services::face {

bool FaceService::initialize(uint64_t now_ms) {
  return pipeline_.initialize(now_ms);
}

void FaceService::tick(uint64_t now_ms) {
  pipeline_.tick(now_ms);
}

}  // namespace ncos::services::face
