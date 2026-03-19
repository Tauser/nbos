#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::drivers::storage {

enum class StorageBackendId : uint8_t {
  kUnavailable = 0,
  kNvs,
  kSdDeferred,
};

struct StoragePersistenceLimits {
  size_t max_record_bytes = 0;
  bool erase_corrupt_records = false;
  bool allow_runtime_config_persistence = false;
  bool allow_companion_memory_persistence = false;
};

struct StoragePlatformBsp {
  const char* board_name = "unknown";
  StorageBackendId config_backend = StorageBackendId::kUnavailable;
  const char* config_namespace = "";
  const char* runtime_config_key = "";
  const char* persistent_companion_memory_key = "";
  StoragePersistenceLimits persistence{};
};

const StoragePlatformBsp& active_storage_platform_bsp();

}  // namespace ncos::drivers::storage
