#include "drivers/storage/runtime_config_store.hpp"

#include <string.h>

#include "drivers/storage/storage_platform_bsp.hpp"

namespace {

ncos::interfaces::state::RuntimeConfigPersistenceStatus map_status(
    ncos::drivers::storage::LocalPersistenceStatus status) {
  using ncos::drivers::storage::LocalPersistenceStatus;
  using ncos::interfaces::state::RuntimeConfigPersistenceStatus;

  switch (status) {
    case LocalPersistenceStatus::kOk:
      return RuntimeConfigPersistenceStatus::kOk;
    case LocalPersistenceStatus::kNotFound:
      return RuntimeConfigPersistenceStatus::kNotFound;
    case LocalPersistenceStatus::kUnavailable:
      return RuntimeConfigPersistenceStatus::kUnavailable;
    case LocalPersistenceStatus::kInvalidArgument:
      return RuntimeConfigPersistenceStatus::kInvalidData;
    case LocalPersistenceStatus::kIoError:
    default:
      return RuntimeConfigPersistenceStatus::kIoError;
  }
}

}  // namespace

namespace ncos::drivers::storage {

RuntimeConfigStore::RuntimeConfigStore() : persistence_(&owned_persistence_) {}

RuntimeConfigStore::RuntimeConfigStore(LocalPersistence* persistence)
    : persistence_(persistence != nullptr ? persistence : &owned_persistence_) {}

ncos::interfaces::state::RuntimeConfigPersistenceStatus RuntimeConfigStore::load(
    ncos::core::contracts::PersistedRuntimeConfigRecord* out_record) {
  if (out_record == nullptr) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kInvalidData;
  }

  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_runtime_config_persistence || bsp.config_backend == StorageBackendId::kUnavailable) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kUnavailable;
  }

  ncos::core::contracts::PersistedRuntimeConfigRecord stored{};
  size_t stored_size = 0;
  const auto status = persistence_->read_blob(bsp.config_namespace,
                                              bsp.runtime_config_key,
                                              &stored,
                                              sizeof(stored),
                                              &stored_size);
  if (status != LocalPersistenceStatus::kOk) {
    return map_status(status);
  }

  if (stored_size != sizeof(stored) || !ncos::core::contracts::is_valid_persisted_runtime_config(stored)) {
    if (bsp.persistence.erase_corrupt_records) {
      (void)persistence_->erase_key(bsp.config_namespace, bsp.runtime_config_key);
    }
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kInvalidData;
  }

  *out_record = stored;
  return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk;
}

ncos::interfaces::state::RuntimeConfigPersistenceStatus RuntimeConfigStore::save(
    const ncos::core::contracts::PersistedRuntimeConfigRecord& record) {
  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_runtime_config_persistence || bsp.config_backend == StorageBackendId::kUnavailable) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kUnavailable;
  }

  auto sanitized = record;
  if (!ncos::core::contracts::sanitize_persisted_runtime_config(&sanitized) ||
      !ncos::core::contracts::is_valid_persisted_runtime_config(sanitized)) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kInvalidData;
  }

  return map_status(
      persistence_->write_blob(bsp.config_namespace, bsp.runtime_config_key, &sanitized, sizeof(sanitized)));
}

ncos::interfaces::state::RuntimeConfigPersistenceStatus RuntimeConfigStore::reset() {
  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_runtime_config_persistence || bsp.config_backend == StorageBackendId::kUnavailable) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kUnavailable;
  }

  const auto status = persistence_->erase_key(bsp.config_namespace, bsp.runtime_config_key);
  if (status == LocalPersistenceStatus::kNotFound) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk;
  }
  return map_status(status);
}

ncos::core::contracts::PersistedRuntimeConfigRecord RuntimeConfigStore::capture_runtime_config(
    const ncos::config::RuntimeConfig& runtime_config) {
  ncos::core::contracts::PersistedRuntimeConfigRecord record{};
  record.diagnostics_enabled = runtime_config.diagnostics_enabled;
  record.cloud_sync_enabled = runtime_config.cloud_sync_enabled;
  record.cloud_bridge_enabled = runtime_config.cloud_bridge_enabled;
  record.cloud_extension_enabled = runtime_config.cloud_extension_enabled;
  record.telemetry_enabled = runtime_config.telemetry_enabled;
  record.telemetry_export_off_device = runtime_config.telemetry_export_off_device;
  record.cloud_sync_interval_ms = runtime_config.cloud_sync_interval_ms;
  record.telemetry_interval_ms = runtime_config.telemetry_interval_ms;
  ncos::core::contracts::sanitize_persisted_runtime_config(&record);
  return record;
}

void RuntimeConfigStore::apply_record_to_runtime_config(
    const ncos::core::contracts::PersistedRuntimeConfigRecord& record,
    ncos::config::RuntimeConfig* runtime_config) {
  if (runtime_config == nullptr) {
    return;
  }

  auto sanitized = record;
  if (!ncos::core::contracts::sanitize_persisted_runtime_config(&sanitized)) {
    return;
  }

  runtime_config->diagnostics_enabled = sanitized.diagnostics_enabled;
  runtime_config->cloud_sync_enabled = sanitized.cloud_sync_enabled;
  runtime_config->cloud_bridge_enabled = sanitized.cloud_bridge_enabled;
  runtime_config->cloud_extension_enabled = sanitized.cloud_extension_enabled;
  runtime_config->telemetry_enabled = sanitized.telemetry_enabled;
  runtime_config->telemetry_export_off_device = sanitized.telemetry_export_off_device;
  runtime_config->cloud_sync_interval_ms = sanitized.cloud_sync_interval_ms;
  runtime_config->telemetry_interval_ms = sanitized.telemetry_interval_ms;
}

}  // namespace ncos::drivers::storage
