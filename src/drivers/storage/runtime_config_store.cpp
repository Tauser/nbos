#include "drivers/storage/runtime_config_store.hpp"

#include <string.h>

#include "drivers/storage/storage_platform_bsp.hpp"

namespace {

constexpr char PrimarySlotKey[] = "runtime_cfg_a";
constexpr char BackupSlotKey[] = "runtime_cfg_b";

struct StoredEnvelopeSlot {
  bool present = false;
  bool valid = false;
  bool needs_erase = false;
  const char* key = nullptr;
  ncos::core::contracts::PersistedRuntimeConfigEnvelope envelope{};
};

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

StoredEnvelopeSlot read_slot(ncos::drivers::storage::LocalPersistence* persistence,
                             const char* ns,
                             const char* key,
                             bool erase_corrupt_records) {
  StoredEnvelopeSlot slot{};
  slot.key = key;
  size_t stored_size = 0;
  const auto status = persistence->read_blob(ns,
                                             key,
                                             &slot.envelope,
                                             sizeof(slot.envelope),
                                             &stored_size);
  if (status == ncos::drivers::storage::LocalPersistenceStatus::kNotFound) {
    return slot;
  }

  slot.present = status == ncos::drivers::storage::LocalPersistenceStatus::kOk;
  slot.valid = slot.present &&
               stored_size == ncos::core::contracts::persisted_runtime_config_envelope_size() &&
               ncos::core::contracts::is_valid_persisted_runtime_config_envelope(slot.envelope);
  slot.needs_erase = erase_corrupt_records && slot.present && !slot.valid;
  return slot;
}

const StoredEnvelopeSlot* choose_best_slot(const StoredEnvelopeSlot& primary,
                                           const StoredEnvelopeSlot& backup) {
  if (primary.valid && backup.valid) {
    return primary.envelope.generation >= backup.envelope.generation ? &primary : &backup;
  }
  if (primary.valid) {
    return &primary;
  }
  if (backup.valid) {
    return &backup;
  }
  return nullptr;
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
  if (!bsp.persistence.allow_runtime_config_persistence ||
      bsp.config_backend == StorageBackendId::kUnavailable) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kUnavailable;
  }

  const auto primary = read_slot(persistence_, bsp.config_namespace, primary_slot_key(),
                                 bsp.persistence.erase_corrupt_records);
  const auto backup = read_slot(persistence_, bsp.config_namespace, backup_slot_key(),
                                bsp.persistence.erase_corrupt_records);

  if (primary.needs_erase) {
    (void)persistence_->erase_key(bsp.config_namespace, primary.key);
  }
  if (backup.needs_erase) {
    (void)persistence_->erase_key(bsp.config_namespace, backup.key);
  }

  if (const auto* best = choose_best_slot(primary, backup); best != nullptr) {
    *out_record = best->envelope.payload;
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk;
  }

  ncos::core::contracts::PersistedRuntimeConfigRecord legacy{};
  size_t legacy_size = 0;
  const auto legacy_status = persistence_->read_blob(bsp.config_namespace,
                                                     legacy_slot_key(),
                                                     &legacy,
                                                     sizeof(legacy),
                                                     &legacy_size);
  if (legacy_status == LocalPersistenceStatus::kNotFound) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kNotFound;
  }
  if (legacy_status != LocalPersistenceStatus::kOk) {
    return map_status(legacy_status);
  }

  if (legacy_size != sizeof(legacy) ||
      !ncos::core::contracts::is_valid_persisted_runtime_config(legacy)) {
    if (bsp.persistence.erase_corrupt_records) {
      (void)persistence_->erase_key(bsp.config_namespace, legacy_slot_key());
    }
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kInvalidData;
  }

  *out_record = legacy;
  return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk;
}

ncos::interfaces::state::RuntimeConfigPersistenceStatus RuntimeConfigStore::save(
    const ncos::core::contracts::PersistedRuntimeConfigRecord& record) {
  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_runtime_config_persistence ||
      bsp.config_backend == StorageBackendId::kUnavailable) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kUnavailable;
  }

  auto sanitized = record;
  if (!ncos::core::contracts::sanitize_persisted_runtime_config(&sanitized) ||
      !ncos::core::contracts::is_valid_persisted_runtime_config(sanitized)) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kInvalidData;
  }

  const auto primary = read_slot(persistence_, bsp.config_namespace, primary_slot_key(), false);
  const auto backup = read_slot(persistence_, bsp.config_namespace, backup_slot_key(), false);
  const auto* best = choose_best_slot(primary, backup);

  const uint32_t next_generation =
      best != nullptr ? best->envelope.generation + 1U : 1U;
  const char* target_key = primary_slot_key();
  if (best != nullptr && best->key == primary_slot_key()) {
    target_key = backup_slot_key();
  }

  const auto envelope =
      ncos::core::contracts::make_persisted_runtime_config_envelope(sanitized, next_generation);
  const auto write_status = persistence_->write_blob(
      bsp.config_namespace, target_key, &envelope, sizeof(envelope));
  if (write_status != LocalPersistenceStatus::kOk) {
    return map_status(write_status);
  }

  (void)persistence_->erase_key(bsp.config_namespace, legacy_slot_key());
  return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk;
}

ncos::interfaces::state::RuntimeConfigPersistenceStatus RuntimeConfigStore::reset() {
  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_runtime_config_persistence ||
      bsp.config_backend == StorageBackendId::kUnavailable) {
    return ncos::interfaces::state::RuntimeConfigPersistenceStatus::kUnavailable;
  }

  bool had_error = false;
  const char* keys[] = {primary_slot_key(), backup_slot_key(), legacy_slot_key()};
  for (const char* key : keys) {
    const auto status = persistence_->erase_key(bsp.config_namespace, key);
    if (status != LocalPersistenceStatus::kOk && status != LocalPersistenceStatus::kNotFound) {
      had_error = true;
    }
  }

  return had_error ? ncos::interfaces::state::RuntimeConfigPersistenceStatus::kIoError
                   : ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk;
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

bool RuntimeConfigStore::is_exportable_record(
    const ncos::core::contracts::PersistedRuntimeConfigRecord& record) {
  if (!ncos::core::contracts::storage_data_is_portable(
          ncos::core::contracts::StorageDataClass::kRuntimeConfig)) {
    return false;
  }

  return ncos::core::contracts::is_importable_persisted_runtime_config(record);
}

bool RuntimeConfigStore::apply_import_record_to_runtime_config(
    const ncos::core::contracts::PersistedRuntimeConfigRecord& record,
    ncos::config::RuntimeConfig* runtime_config) {
  if (runtime_config == nullptr || !is_exportable_record(record)) {
    return false;
  }

  apply_record_to_runtime_config(record, runtime_config);
  return true;
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

const char* RuntimeConfigStore::primary_slot_key() {
  return PrimarySlotKey;
}

const char* RuntimeConfigStore::backup_slot_key() {
  return BackupSlotKey;
}

const char* RuntimeConfigStore::legacy_slot_key() {
  return active_storage_platform_bsp().runtime_config_key;
}

}  // namespace ncos::drivers::storage
