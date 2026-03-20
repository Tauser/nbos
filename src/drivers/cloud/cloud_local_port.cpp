#include "drivers/cloud/cloud_local_port.hpp"

#include "hal/platform/runtime_build_target.hpp"

namespace ncos::drivers::cloud {

bool CloudLocalPort::ensure_ready() {
  return ncos::hal::platform::running_native_tests();
}

bool CloudLocalPort::send_packet(const ncos::core::contracts::CloudSyncPacket&) {
  return ncos::hal::platform::running_native_tests();
}

ncos::interfaces::cloud::CloudSyncPort* acquire_shared_cloud_port() {
  static CloudLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::cloud
