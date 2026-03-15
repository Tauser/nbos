#include "drivers/cloud/cloud_local_extension_port.hpp"

namespace ncos::drivers::cloud {

bool CloudLocalExtensionPort::ensure_ready() {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  return false;
#endif
}

bool CloudLocalExtensionPort::submit_extension(
    const ncos::core::contracts::CloudExtensionRequest&,
    ncos::core::contracts::CloudExtensionResponse* out_response) {
  if (out_response == nullptr) {
    return false;
  }

#ifdef NCOS_NATIVE_TESTS
  out_response->accepted = true;
  out_response->applied = true;
  out_response->safe_offline_compatible = true;
  out_response->reason = "local_extension_stub_applied";
  return true;
#else
  out_response->accepted = false;
  out_response->applied = false;
  out_response->safe_offline_compatible = true;
  out_response->reason = "extension_not_available_on_device";
  return false;
#endif
}

ncos::interfaces::cloud::CloudExtensionPort* acquire_shared_cloud_extension_port() {
  static CloudLocalExtensionPort instance;
  return &instance;
}

}  // namespace ncos::drivers::cloud
