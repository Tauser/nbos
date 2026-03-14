#pragma once

#include <stdint.h>

namespace ncos::interfaces::sensing {

struct ImuSampleRaw {
  int16_t ax = 0;
  int16_t ay = 0;
  int16_t az = 0;
  int16_t gx = 0;
  int16_t gy = 0;
  int16_t gz = 0;
};

class ImuPort {
 public:
  virtual ~ImuPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool read_sample(ImuSampleRaw* out_sample) = 0;
};

}  // namespace ncos::interfaces::sensing
