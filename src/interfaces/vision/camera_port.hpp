#pragma once

#include <stdint.h>

namespace ncos::interfaces::vision {

struct CameraFrameMeta {
  uint16_t width = 0;
  uint16_t height = 0;
  uint32_t payload_bytes = 0;
  uint8_t motion_delta_percent = 0;
};

class CameraPort {
 public:
  virtual ~CameraPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool is_component_available() const = 0;
  virtual bool capture_frame(CameraFrameMeta* out_frame) = 0;
};

}  // namespace ncos::interfaces::vision
