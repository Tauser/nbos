#pragma once

#include <stdint.h>

#include "core/contracts/sensing_runtime_contracts.hpp"
#include "interfaces/sensing/imu_port.hpp"

namespace ncos::services::sensing {

class ImuService final {
 public:
  void bind_port(ncos::interfaces::sensing::ImuPort* port);
  bool initialize(uint64_t now_ms);
  void tick(uint64_t now_ms);
  [[nodiscard]] const ncos::core::contracts::ImuRuntimeState& state() const;

 private:
  static int16_t accel_raw_to_mg(int16_t raw);
  static int16_t gyro_raw_to_dps(int16_t raw);

  ncos::interfaces::sensing::ImuPort* port_ = nullptr;
  ncos::core::contracts::ImuRuntimeState state_ = ncos::core::contracts::make_imu_runtime_baseline();
  uint64_t next_probe_ms_ = 0;
};

}  // namespace ncos::services::sensing
