#include "core/contracts/storage_runtime_contracts.hpp"

namespace {

uint32_t clamp_interval(uint32_t value, uint32_t min_value, uint32_t max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

const ncos::core::contracts::StorageDataPolicy RuntimeConfigPolicy{
    "runtime_config",
    true,
    ncos::core::contracts::StorageRetentionKind::kUntilOverwriteOrReset,
    0,
    true,
    true,
    true,
    true,
    ncos::core::contracts::StorageTransferMode::kManualPortable,
    ncos::core::contracts::StorageTransferMode::kManualPortable,
};

const ncos::core::contracts::StorageDataPolicy ShortSessionMemoryPolicy{
    "short_session_memory",
    false,
    ncos::core::contracts::StorageRetentionKind::kVolatileOnly,
    0,
    false,
    false,
    false,
    false,
    ncos::core::contracts::StorageTransferMode::kBlocked,
    ncos::core::contracts::StorageTransferMode::kBlocked,
};

const ncos::core::contracts::StorageDataPolicy AdaptivePersonalityPolicy{
    "adaptive_personality",
    false,
    ncos::core::contracts::StorageRetentionKind::kVolatileOnly,
    0,
    false,
    false,
    false,
    false,
    ncos::core::contracts::StorageTransferMode::kBlocked,
    ncos::core::contracts::StorageTransferMode::kBlocked,
};

const ncos::core::contracts::StorageDataPolicy TelemetryBufferPolicy{
    "telemetry_buffer",
    false,
    ncos::core::contracts::StorageRetentionKind::kVolatileOnly,
    0,
    false,
    false,
    false,
    true,
    ncos::core::contracts::StorageTransferMode::kBlocked,
    ncos::core::contracts::StorageTransferMode::kBlocked,
};

const ncos::core::contracts::StorageDataPolicy FaultHistoryPolicy{
    "fault_history",
    false,
    ncos::core::contracts::StorageRetentionKind::kVolatileOnly,
    0,
    false,
    false,
    false,
    true,
    ncos::core::contracts::StorageTransferMode::kBlocked,
    ncos::core::contracts::StorageTransferMode::kBlocked,
};

}  // namespace

namespace ncos::core::contracts {

PersistedRuntimeConfigRecord make_default_persisted_runtime_config() {
  return PersistedRuntimeConfigRecord{};
}

bool sanitize_persisted_runtime_config(PersistedRuntimeConfigRecord* record) {
  if (record == nullptr) {
    return false;
  }

  record->schema_version = PersistedRuntimeConfigSchemaVersion::kV1;
  record->cloud_sync_interval_ms = clamp_interval(record->cloud_sync_interval_ms, 1000U, 60000U);
  record->telemetry_interval_ms = clamp_interval(record->telemetry_interval_ms, 1000U, 120000U);
  return true;
}

bool is_valid_persisted_runtime_config(const PersistedRuntimeConfigRecord& record) {
  if (record.schema_version != PersistedRuntimeConfigSchemaVersion::kV1) {
    return false;
  }

  return record.cloud_sync_interval_ms >= 1000U && record.cloud_sync_interval_ms <= 60000U &&
         record.telemetry_interval_ms >= 1000U && record.telemetry_interval_ms <= 120000U;
}

bool is_importable_persisted_runtime_config(const PersistedRuntimeConfigRecord& record) {
  if (storage_data_policy(StorageDataClass::kRuntimeConfig).import_mode !=
      StorageTransferMode::kManualPortable) {
    return false;
  }

  if (record.schema_version != PersistedRuntimeConfigSchemaVersion::kV1) {
    return false;
  }

  auto sanitized = record;
  return sanitize_persisted_runtime_config(&sanitized) && is_valid_persisted_runtime_config(sanitized);
}

const StorageDataPolicy& storage_data_policy(StorageDataClass data_class) {
  switch (data_class) {
    case StorageDataClass::kRuntimeConfig:
      return RuntimeConfigPolicy;
    case StorageDataClass::kShortSessionMemory:
      return ShortSessionMemoryPolicy;
    case StorageDataClass::kAdaptivePersonality:
      return AdaptivePersonalityPolicy;
    case StorageDataClass::kTelemetryBuffer:
      return TelemetryBufferPolicy;
    case StorageDataClass::kFaultHistory:
    default:
      return FaultHistoryPolicy;
  }
}

bool storage_data_is_portable(StorageDataClass data_class) {
  const auto& policy = storage_data_policy(data_class);
  return policy.export_mode == StorageTransferMode::kManualPortable &&
         policy.import_mode == StorageTransferMode::kManualPortable;
}

}  // namespace ncos::core::contracts
