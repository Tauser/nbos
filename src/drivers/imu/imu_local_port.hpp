#pragma once

#include "drivers/imu/mpu6050_bringup.hpp"
#include "interfaces/sensing/imu_port.hpp"

namespace ncos::drivers::imu {

class ImuLocalPort final : public ncos::interfaces::sensing::ImuPort {
 public:
  bool ensure_ready() override;
  bool read_sample(ncos::interfaces::sensing::ImuSampleRaw* out_sample) override;

 private:
  Mpu6050Bringup bringup_{};
};

ncos::interfaces::sensing::ImuPort* acquire_shared_imu_port();

}  // namespace ncos::drivers::imu
