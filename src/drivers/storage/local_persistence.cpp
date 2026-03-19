#include "drivers/storage/local_persistence.hpp"

#include <string.h>

#include "drivers/storage/storage_platform_bsp.hpp"

#if defined(ESP_PLATFORM)
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#endif

namespace {

#if !defined(ESP_PLATFORM)
struct HostPersistenceSlot {
  bool occupied = false;
  char ns[16] = {};
  char key[16] = {};
  size_t size = 0;
  uint8_t data[128] = {};
};

HostPersistenceSlot s_host_slot{};
#endif

bool copy_token(const char* source, char* dest, size_t capacity) {
  if (source == nullptr || dest == nullptr || capacity == 0) {
    return false;
  }

  const size_t len = strlen(source);
  if (len + 1 > capacity) {
    return false;
  }

  memcpy(dest, source, len + 1);
  return true;
}

}  // namespace

namespace ncos::drivers::storage {

bool LocalPersistence::initialize() {
  if (initialized_) {
    return available_;
  }

#if defined(ESP_PLATFORM)
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    if (active_storage_platform_bsp().persistence.erase_corrupt_records &&
        (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
      if (nvs_flash_erase() == ESP_OK) {
        err = nvs_flash_init();
      }
    }
  }
  available_ = err == ESP_OK;
#else
  available_ = true;
#endif

  initialized_ = true;
  return available_;
}

LocalPersistenceStatus LocalPersistence::read_blob(const char* ns,
                                                   const char* key,
                                                   void* out_data,
                                                   size_t out_capacity,
                                                   size_t* out_size) {
  if (ns == nullptr || key == nullptr || out_data == nullptr || out_capacity == 0) {
    return LocalPersistenceStatus::kInvalidArgument;
  }
  if (!initialize()) {
    return LocalPersistenceStatus::kUnavailable;
  }

#if defined(ESP_PLATFORM)
  nvs_handle_t handle = 0;
  esp_err_t err = nvs_open(ns, NVS_READONLY, &handle);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    return LocalPersistenceStatus::kNotFound;
  }
  if (err != ESP_OK) {
    return LocalPersistenceStatus::kIoError;
  }

  size_t required = 0;
  err = nvs_get_blob(handle, key, nullptr, &required);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(handle);
    return LocalPersistenceStatus::kNotFound;
  }
  if (err != ESP_OK || required > out_capacity) {
    nvs_close(handle);
    return LocalPersistenceStatus::kIoError;
  }

  err = nvs_get_blob(handle, key, out_data, &required);
  nvs_close(handle);
  if (err != ESP_OK) {
    return LocalPersistenceStatus::kIoError;
  }

  if (out_size != nullptr) {
    *out_size = required;
  }
  return LocalPersistenceStatus::kOk;
#else
  if (!s_host_slot.occupied || strcmp(s_host_slot.ns, ns) != 0 || strcmp(s_host_slot.key, key) != 0) {
    return LocalPersistenceStatus::kNotFound;
  }
  if (s_host_slot.size > out_capacity) {
    return LocalPersistenceStatus::kIoError;
  }

  memcpy(out_data, s_host_slot.data, s_host_slot.size);
  if (out_size != nullptr) {
    *out_size = s_host_slot.size;
  }
  return LocalPersistenceStatus::kOk;
#endif
}

LocalPersistenceStatus LocalPersistence::write_blob(const char* ns,
                                                    const char* key,
                                                    const void* data,
                                                    size_t size) {
  if (ns == nullptr || key == nullptr || data == nullptr || size == 0 ||
      size > active_storage_platform_bsp().persistence.max_record_bytes) {
    return LocalPersistenceStatus::kInvalidArgument;
  }
  if (!initialize()) {
    return LocalPersistenceStatus::kUnavailable;
  }

#if defined(ESP_PLATFORM)
  nvs_handle_t handle = 0;
  esp_err_t err = nvs_open(ns, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    return LocalPersistenceStatus::kIoError;
  }

  err = nvs_set_blob(handle, key, data, size);
  if (err == ESP_OK) {
    err = nvs_commit(handle);
  }
  nvs_close(handle);
  return err == ESP_OK ? LocalPersistenceStatus::kOk : LocalPersistenceStatus::kIoError;
#else
  memset(&s_host_slot, 0, sizeof(s_host_slot));
  s_host_slot.occupied = copy_token(ns, s_host_slot.ns, sizeof(s_host_slot.ns)) &&
                         copy_token(key, s_host_slot.key, sizeof(s_host_slot.key));
  if (!s_host_slot.occupied) {
    memset(&s_host_slot, 0, sizeof(s_host_slot));
    return LocalPersistenceStatus::kIoError;
  }
  memcpy(s_host_slot.data, data, size);
  s_host_slot.size = size;
  return LocalPersistenceStatus::kOk;
#endif
}

LocalPersistenceStatus LocalPersistence::erase_key(const char* ns, const char* key) {
  if (ns == nullptr || key == nullptr) {
    return LocalPersistenceStatus::kInvalidArgument;
  }
  if (!initialize()) {
    return LocalPersistenceStatus::kUnavailable;
  }

#if defined(ESP_PLATFORM)
  nvs_handle_t handle = 0;
  esp_err_t err = nvs_open(ns, NVS_READWRITE, &handle);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    return LocalPersistenceStatus::kNotFound;
  }
  if (err != ESP_OK) {
    return LocalPersistenceStatus::kIoError;
  }

  err = nvs_erase_key(handle, key);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(handle);
    return LocalPersistenceStatus::kNotFound;
  }
  if (err == ESP_OK) {
    err = nvs_commit(handle);
  }
  nvs_close(handle);
  return err == ESP_OK ? LocalPersistenceStatus::kOk : LocalPersistenceStatus::kIoError;
#else
  if (!s_host_slot.occupied || strcmp(s_host_slot.ns, ns) != 0 || strcmp(s_host_slot.key, key) != 0) {
    return LocalPersistenceStatus::kNotFound;
  }
  memset(&s_host_slot, 0, sizeof(s_host_slot));
  return LocalPersistenceStatus::kOk;
#endif
}

}  // namespace ncos::drivers::storage
