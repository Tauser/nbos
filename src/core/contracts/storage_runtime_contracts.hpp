#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::core::contracts {

enum class PersistedRuntimeConfigSchemaVersion : uint8_t {
  kV1 = 1,
};

enum class PersistedStorageEnvelopeVersion : uint8_t {
  kV1 = 1,
};

enum class StorageDataClass : uint8_t {
  kRuntimeConfig = 0,
  kShortSessionMemory,
  kAdaptivePersonality,
  kTelemetryBuffer,
  kFaultHistory,
};

enum class StorageRetentionKind : uint8_t {
  kVolatileOnly = 0,
  kUntilOverwriteOrReset,
  kRollingWindow,
};

enum class StorageTransferMode : uint8_t {
  kBlocked = 0,
  kManualPortable,
};

struct StorageDataPolicy {
  const char* name = "unknown";
  bool persist_locally = false;
  StorageRetentionKind retention = StorageRetentionKind::kVolatileOnly;
  uint16_t retention_days = 0;
  bool integrity_checked = false;
  bool erase_on_schema_mismatch = false;
  bool erase_on_user_reset = false;
  bool erase_on_factory_reset = false;
  StorageTransferMode export_mode = StorageTransferMode::kBlocked;
  StorageTransferMode import_mode = StorageTransferMode::kBlocked;
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

struct PersistedRuntimeConfigEnvelope {
  uint32_t magic = 0;
  PersistedStorageEnvelopeVersion envelope_version = PersistedStorageEnvelopeVersion::kV1;
  uint8_t reserved = 0;
  uint16_t payload_size = 0;
  uint32_t generation = 0;
  uint32_t checksum = 0;
  PersistedRuntimeConfigRecord payload{};
};

PersistedRuntimeConfigRecord make_default_persisted_runtime_config();
bool sanitize_persisted_runtime_config(PersistedRuntimeConfigRecord* record);
bool is_valid_persisted_runtime_config(const PersistedRuntimeConfigRecord& record);
bool is_importable_persisted_runtime_config(const PersistedRuntimeConfigRecord& record);
const StorageDataPolicy& storage_data_policy(StorageDataClass data_class);
bool storage_data_is_portable(StorageDataClass data_class);
uint32_t persisted_runtime_config_checksum(const PersistedRuntimeConfigRecord& record);
PersistedRuntimeConfigEnvelope make_persisted_runtime_config_envelope(
    const PersistedRuntimeConfigRecord& record,
    uint32_t generation);
bool sanitize_persisted_runtime_config_envelope(PersistedRuntimeConfigEnvelope* envelope);
bool is_valid_persisted_runtime_config_envelope(const PersistedRuntimeConfigEnvelope& envelope);
size_t persisted_runtime_config_envelope_size();

}  // namespace ncos::core::contracts
