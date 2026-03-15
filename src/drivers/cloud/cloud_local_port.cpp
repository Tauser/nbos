#include "drivers/cloud/cloud_local_port.hpp"

namespace ncos::drivers::cloud {

bool CloudLocalPort::ensure_ready() {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  return false;
#endif
}

bool CloudLocalPort::send_packet(const ncos::core::contracts::CloudSyncPacket&) {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  return false;
#endif
}

ncos::interfaces::cloud::CloudSyncPort* acquire_shared_cloud_port() {
  static CloudLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::cloud
