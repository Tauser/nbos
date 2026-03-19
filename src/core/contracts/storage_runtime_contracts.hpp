#pragma once

#include <stdint.h>

namespace ncos::core::contracts {

enum class PersistedRuntimeConfigSchemaVersion : uint8_t {
  kV1 = 1,
};

struct PersistedRuntimeConfigRecord {
  PersistedRuntimeConfigSchemaVersion schema_version = PersistedRuntimeConfigSchemaVersion::kV1;
  bool diagnostics_enabled = true;
  bool cloud_sync_enabled = false;
  bool cloud_bridge_enabled = false;
  bool cloud_extension_enabled = false;
  bool telemetry_enabled = false;
  bool telemetry_export_off_device = false;
  uint32_t cloud_sync_interval_ms = 2500;
  uint32_t telemetry_interval_ms = 8000;
};

PersistedRuntimeConfigRecord make_default_persisted_runtime_config();
bool sanitize_persisted_runtime_config(PersistedRuntimeConfigRecord* record);
bool is_valid_persisted_runtime_config(const PersistedRuntimeConfigRecord& record);

}  // namespace ncos::core::contracts
