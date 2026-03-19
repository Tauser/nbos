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

}  // namespace ncos::core::contracts
