#include "drivers/update/update_local_port.hpp"

#ifndef NCOS_NATIVE_TESTS
#include <string.h>

#include "esp_ota_ops.h"
#endif

namespace ncos::drivers::update {

bool UpdateLocalPort::ensure_ready() {
  return true;
}

bool UpdateLocalPort::read_boot_info(ncos::interfaces::update::OtaBootInfo* out_info) {
  if (out_info == nullptr) {
    return false;
  }

  ncos::interfaces::update::OtaBootInfo info{};

#ifdef NCOS_NATIVE_TESTS
  info.component_available = false;
  info.rollback_supported = false;
  info.pending_verify = false;
  info.running_slot = ncos::core::contracts::OtaRunningSlot::kFactory;
#else
  const esp_partition_t* running = esp_ota_get_running_partition();
  info.component_available = (running != nullptr);
  info.rollback_supported = esp_ota_check_rollback_is_possible();

  if (running != nullptr) {
    if (strcmp(running->label, "factory") == 0) {
      info.running_slot = ncos::core::contracts::OtaRunningSlot::kFactory;
    } else if (strcmp(running->label, "ota_0") == 0) {
      info.running_slot = ncos::core::contracts::OtaRunningSlot::kSlotA;
    } else if (strcmp(running->label, "ota_1") == 0) {
      info.running_slot = ncos::core::contracts::OtaRunningSlot::kSlotB;
    }

    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
      info.pending_verify = (state == ESP_OTA_IMG_PENDING_VERIFY);
    }
  }
#endif

  *out_info = info;
  return true;
}

bool UpdateLocalPort::confirm_running_image() {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  return esp_ota_mark_app_valid_cancel_rollback() == ESP_OK;
#endif
}

bool UpdateLocalPort::request_rollback() {
#ifdef NCOS_NATIVE_TESTS
  return true;
#else
  // NOTE: this call may reboot immediately if rollback is accepted by bootloader.
  const esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();
  return err == ESP_OK;
#endif
}

ncos::interfaces::update::UpdatePort* acquire_shared_update_port() {
  static UpdateLocalPort instance;
  return &instance;
}

}  // namespace ncos::drivers::update
