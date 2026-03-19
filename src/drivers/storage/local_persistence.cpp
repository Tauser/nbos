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

constexpr size_t HostPersistenceSlotCount = 8;
HostPersistenceSlot s_host_slots[HostPersistenceSlotCount]{};
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

#if !defined(ESP_PLATFORM)
HostPersistenceSlot* find_host_slot(const char* ns, const char* key) {
  for (auto& slot : s_host_slots) {
    if (slot.occupied && strcmp(slot.ns, ns) == 0 && strcmp(slot.key, key) == 0) {
      return &slot;
    }
  }
  return nullptr;
}

HostPersistenceSlot* find_free_host_slot() {
  for (auto& slot : s_host_slots) {
    if (!slot.occupied) {
      return &slot;
    }
  }
  return nullptr;
}
#endif

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
  const auto* slot = find_host_slot(ns, key);
  if (slot == nullptr) {
    return LocalPersistenceStatus::kNotFound;
  }
  if (slot->size > out_capacity) {
    return LocalPersistenceStatus::kIoError;
  }

  memcpy(out_data, slot->data, slot->size);
  if (out_size != nullptr) {
    *out_size = slot->size;
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
  auto* slot = find_host_slot(ns, key);
  if (slot == nullptr) {
    slot = find_free_host_slot();
  }
  if (slot == nullptr) {
    return LocalPersistenceStatus::kIoError;
  }

  memset(slot, 0, sizeof(*slot));
  slot->occupied = copy_token(ns, slot->ns, sizeof(slot->ns)) &&
                   copy_token(key, slot->key, sizeof(slot->key));
  if (!slot->occupied) {
    memset(slot, 0, sizeof(*slot));
    return LocalPersistenceStatus::kIoError;
  }
  memcpy(slot->data, data, size);
  slot->size = size;
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
  auto* slot = find_host_slot(ns, key);
  if (slot == nullptr) {
    return LocalPersistenceStatus::kNotFound;
  }
  memset(slot, 0, sizeof(*slot));
  return LocalPersistenceStatus::kOk;
#endif
}

}  // namespace ncos::drivers::storage
