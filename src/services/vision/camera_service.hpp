#pragma once

#include <stdint.h>

#include "core/contracts/vision_runtime_contracts.hpp"
#include "interfaces/vision/camera_port.hpp"

namespace ncos::services::vision {

class CameraService final {
 public:
  void bind_port(ncos::interfaces::vision::CameraPort* port);
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms);
  [[nodiscard]] const ncos::core::contracts::CameraRuntimeState& state() const;

 private:
  ncos::interfaces::vision::CameraPort* port_ = nullptr;
  ncos::core::contracts::CameraRuntimeState state_ =
      ncos::core::contracts::make_camera_runtime_baseline();
  uint64_t next_capture_ms_ = 0;
};

}  // namespace ncos::services::vision
