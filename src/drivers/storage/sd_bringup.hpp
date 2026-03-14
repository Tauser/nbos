#pragma once

namespace ncos::drivers::storage {

struct SdProbeResult {
  bool interface_ok = false;
  bool card_init_ok = false;
  int card_size_mb = 0;
};

class SdBringup final {
 public:
  bool run_probe(SdProbeResult* out_result);
};

}  // namespace ncos::drivers::storage
