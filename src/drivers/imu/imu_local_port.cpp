#include "drivers/imu/imu_local_port.hpp"

namespace ncos::drivers::imu {

bool ImuLocalPort::ensure_ready() {
  return bringup_.init();
}

bool ImuLocalPort::read_sample(ncos::interfaces::sensing::ImuSampleRaw* out_sample) {
  if (!ensure_ready() || out_sample == nullptr) {
    return false;
  }

  ncos::drivers::imu::ImuSample sample{};
  if (!bringup_.read_sample(&sample)) {
    return false;
  }

  out_sample->ax = sample.ax;
  out_sample->ay = sample.ay;
  out_sample->az = sample.az;
  out_sample->gx = sample.gx;
  out_sample->gy = sample.gy;
  out_sample->gz = sample.gz;
  return true;
}

ncos::interfaces::sensing::ImuPort* acquire_shared_imu_port() {
  static ImuLocalPort shared{};
  return &shared;
}

}  // namespace ncos::drivers::imu
