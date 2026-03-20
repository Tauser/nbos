#include "drivers/update/update_local_port.hpp"

#include "hal/platform/ota_runtime.hpp"

namespace ncos::drivers::update {

bool UpdateLocalPort::ensure_ready() {
  return true;
}

bool UpdateLocalPort::read_boot_info(ncos::interfaces::update::OtaBootInfo* out_info) {
  return ncos::hal::platform::read_ota_boot_info(out_info);
}

bool UpdateLocalPort::confirm_running_image() {
  return ncos::hal::platform::confirm_running_image();
}

bool UpdateLocalPort::request_rollback() {
  return ncos::hal::platform::request_rollback();
}

ncos::interfaces::update::UpdatePort* acquire_shared_update_port() {
  static UpdateLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::update
