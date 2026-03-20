#include "drivers/cloud/cloud_local_extension_port.hpp"

#include "hal/platform/runtime_build_target.hpp"

namespace ncos::drivers::cloud {

bool CloudLocalExtensionPort::ensure_ready() {
  return ncos::hal::platform::running_native_tests();
}

bool CloudLocalExtensionPort::submit_extension(
    const ncos::core::contracts::CloudExtensionRequest&,
    ncos::core::contracts::CloudExtensionResponse* out_response) {
  if (out_response == nullptr) {
    return false;
  }

  if (ncos::hal::platform::running_native_tests()) {
    out_response->accepted = true;
    out_response->applied = true;
    out_response->safe_offline_compatible = true;
    out_response->reason = "local_extension_stub_applied";
    return true;
  }

  out_response->accepted = false;
  out_response->applied = false;
  out_response->safe_offline_compatible = true;
  out_response->reason = "extension_not_available_on_device";
  return false;
}

ncos::interfaces::cloud::CloudExtensionPort* acquire_shared_cloud_extension_port() {
  static CloudLocalExtensionPort instance;
  return &instance;
}

}  // namespace ncos::drivers::cloud
