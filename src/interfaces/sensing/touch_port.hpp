#pragma once

#include <stdint.h>

namespace ncos::interfaces::sensing {

class TouchPort {
 public:
  virtual ~TouchPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool calibrate_idle() = 0;
  virtual bool read_raw(uint32_t* out_raw) = 0;
  virtual uint32_t baseline_raw() const = 0;
  virtual uint32_t trigger_delta() const = 0;
};

}  // namespace ncos::interfaces::sensing
