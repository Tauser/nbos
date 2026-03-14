#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::drivers::ttlinker {

struct TtlinkerProbeResult {
  bool tx_ok = false;
  size_t bytes_written = 0;
  size_t bytes_read = 0;
  uint8_t first_byte = 0;
};

class TtlinkerBringup final {
 public:
  bool can_run_probe_with_console() const;
  bool init();
  bool run_probe(int read_window_ms, TtlinkerProbeResult* out_result) const;
  void deinit();

 private:
  bool ready_ = false;
};

}  // namespace ncos::drivers::ttlinker
