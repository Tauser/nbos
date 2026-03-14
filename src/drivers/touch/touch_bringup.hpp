#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::drivers::touch {

struct TouchStats {
  uint32_t min_raw = 0;
  uint32_t max_raw = 0;
  uint32_t avg_raw = 0;
  uint32_t noise_span = 0;
  uint32_t suggested_delta = 0;
};

class TouchBringup final {
 public:
  bool init();
  bool read_raw(uint32_t* out_raw) const;
  bool sample_idle_stats(size_t samples, int delay_ms, TouchStats* out_stats) const;
  void deinit();

 private:
  bool ready_ = false;
};

}  // namespace ncos::drivers::touch
