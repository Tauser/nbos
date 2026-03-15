#pragma once

#include "interfaces/power/power_port.hpp"

namespace ncos::drivers::power {

class PowerLocalPort final : public ncos::interfaces::power::PowerPort {
 public:
  bool ensure_ready() override;
  bool read_sample(ncos::interfaces::power::PowerSampleRaw* out_sample) override;
};

ncos::interfaces::power::PowerPort* acquire_shared_power_port();

}  // namespace ncos::drivers::power
