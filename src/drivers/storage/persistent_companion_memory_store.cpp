#include "drivers/storage/persistent_companion_memory_store.hpp"

#include "drivers/storage/storage_platform_bsp.hpp"

namespace {

constexpr char PrimarySlotKey[] = "comp_mem_a";
constexpr char BackupSlotKey[] = "comp_mem_b";

struct StoredEnvelopeSlot {
  bool present = false;
  bool valid = false;
  bool needs_erase = false;
  const char* key = nullptr;
  ncos::core::contracts::PersistedCompanionMemoryEnvelope envelope{};
};

ncos::drivers::storage::PersistentCompanionMemoryStatus map_status(
    ncos::drivers::storage::LocalPersistenceStatus status) {
  using ncos::drivers::storage::LocalPersistenceStatus;
  using ncos::drivers::storage::PersistentCompanionMemoryStatus;

  switch (status) {
    case LocalPersistenceStatus::kOk:
      return PersistentCompanionMemoryStatus::kOk;
    case LocalPersistenceStatus::kNotFound:
      return PersistentCompanionMemoryStatus::kNotFound;
    case LocalPersistenceStatus::kUnavailable:
      return PersistentCompanionMemoryStatus::kUnavailable;
    case LocalPersistenceStatus::kInvalidArgument:
      return PersistentCompanionMemoryStatus::kInvalidData;
    case LocalPersistenceStatus::kIoError:
    default:
      return PersistentCompanionMemoryStatus::kIoError;
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
               stored_size == ncos::core::contracts::persisted_companion_memory_envelope_size() &&
               ncos::core::contracts::is_valid_persisted_companion_memory_envelope(slot.envelope);
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

bool slot_corrupted(const StoredEnvelopeSlot& slot) {
  return slot.present && !slot.valid;
}

}  // namespace

namespace ncos::drivers::storage {

PersistentCompanionMemoryStore::PersistentCompanionMemoryStore() : persistence_(&owned_persistence_) {}

PersistentCompanionMemoryStore::PersistentCompanionMemoryStore(LocalPersistence* persistence)
    : persistence_(persistence != nullptr ? persistence : &owned_persistence_) {}

PersistentCompanionMemoryLoadResult PersistentCompanionMemoryStore::load_with_recovery(
    ncos::core::contracts::PersistedCompanionMemoryRecord* out_record) {
  PersistentCompanionMemoryLoadResult result{};
  if (out_record == nullptr) {
    result.status = PersistentCompanionMemoryStatus::kInvalidData;
    return result;
  }

  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_companion_memory_persistence ||
      bsp.config_backend == StorageBackendId::kUnavailable) {
    result.status = PersistentCompanionMemoryStatus::kUnavailable;
    return result;
  }

  const auto primary = read_slot(persistence_, bsp.config_namespace, primary_slot_key(),
                                 bsp.persistence.erase_corrupt_records);
  const auto backup = read_slot(persistence_, bsp.config_namespace, backup_slot_key(),
                                bsp.persistence.erase_corrupt_records);
  const bool corruption_seen = slot_corrupted(primary) || slot_corrupted(backup);

  if (primary.needs_erase) {
    (void)persistence_->erase_key(bsp.config_namespace, primary.key);
  }
  if (backup.needs_erase) {
    (void)persistence_->erase_key(bsp.config_namespace, backup.key);
  }

  if (const auto* best = choose_best_slot(primary, backup); best != nullptr) {
    *out_record = best->envelope.payload;
    result.status = PersistentCompanionMemoryStatus::kOk;
    result.recovery_path = corruption_seen ? PersistentCompanionMemoryRecoveryPath::kRecoveredLastKnownGood
                                           : PersistentCompanionMemoryRecoveryPath::kDirectLoad;
    if (corruption_seen) {
      result.repaired_storage = save(*out_record) == PersistentCompanionMemoryStatus::kOk;
    }
    return result;
  }

  ncos::core::contracts::PersistedCompanionMemoryRecord legacy{};
  size_t legacy_size = 0;
  const auto legacy_status = persistence_->read_blob(bsp.config_namespace,
                                                     legacy_slot_key(),
                                                     &legacy,
                                                     sizeof(legacy),
                                                     &legacy_size);
  if (legacy_status == LocalPersistenceStatus::kNotFound) {
    if (!corruption_seen) {
      result.status = PersistentCompanionMemoryStatus::kNotFound;
      return result;
    }

    *out_record = default_profile_record();
    result.status = PersistentCompanionMemoryStatus::kOk;
    result.recovery_path = PersistentCompanionMemoryRecoveryPath::kResetProfileBaseline;
    result.repaired_storage = reset_profile() == PersistentCompanionMemoryStatus::kOk;
    return result;
  }
  if (legacy_status != LocalPersistenceStatus::kOk) {
    result.status = map_status(legacy_status);
    return result;
  }

  if (legacy_size != sizeof(legacy) ||
      !ncos::core::contracts::is_valid_persisted_companion_memory(legacy)) {
    if (bsp.persistence.erase_corrupt_records) {
      (void)persistence_->erase_key(bsp.config_namespace, legacy_slot_key());
    }
    *out_record = default_profile_record();
    result.status = PersistentCompanionMemoryStatus::kOk;
    result.recovery_path = PersistentCompanionMemoryRecoveryPath::kResetProfileBaseline;
    result.repaired_storage = reset_profile() == PersistentCompanionMemoryStatus::kOk;
    return result;
  }

  *out_record = legacy;
  result.status = PersistentCompanionMemoryStatus::kOk;
  result.recovery_path = PersistentCompanionMemoryRecoveryPath::kRecoveredLegacy;
  result.repaired_storage = save(legacy) == PersistentCompanionMemoryStatus::kOk;
  return result;
}

PersistentCompanionMemoryStatus PersistentCompanionMemoryStore::load(
    ncos::core::contracts::PersistedCompanionMemoryRecord* out_record) {
  return load_with_recovery(out_record).status;
}

PersistentCompanionMemoryStatus PersistentCompanionMemoryStore::save(
    const ncos::core::contracts::PersistedCompanionMemoryRecord& record) {
  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_companion_memory_persistence ||
      bsp.config_backend == StorageBackendId::kUnavailable) {
    return PersistentCompanionMemoryStatus::kUnavailable;
  }

  auto sanitized = record;
  if (!ncos::core::contracts::sanitize_persisted_companion_memory(&sanitized) ||
      !ncos::core::contracts::is_valid_persisted_companion_memory(sanitized)) {
    return PersistentCompanionMemoryStatus::kInvalidData;
  }

  const auto primary = read_slot(persistence_, bsp.config_namespace, primary_slot_key(), false);
  const auto backup = read_slot(persistence_, bsp.config_namespace, backup_slot_key(), false);
  const auto* best = choose_best_slot(primary, backup);

  const uint32_t next_generation = best != nullptr ? best->envelope.generation + 1U : 1U;
  const char* target_key = primary_slot_key();
  if (best != nullptr && best->key == primary_slot_key()) {
    target_key = backup_slot_key();
  }

  const auto envelope =
      ncos::core::contracts::make_persisted_companion_memory_envelope(sanitized, next_generation);
  const auto write_status =
      persistence_->write_blob(bsp.config_namespace, target_key, &envelope, sizeof(envelope));
  if (write_status != LocalPersistenceStatus::kOk) {
    return map_status(write_status);
  }

  (void)persistence_->erase_key(bsp.config_namespace, legacy_slot_key());
  return PersistentCompanionMemoryStatus::kOk;
}

PersistentCompanionMemoryStatus PersistentCompanionMemoryStore::reset() {
  const auto& bsp = active_storage_platform_bsp();
  if (!bsp.persistence.allow_companion_memory_persistence ||
      bsp.config_backend == StorageBackendId::kUnavailable) {
    return PersistentCompanionMemoryStatus::kUnavailable;
  }

  bool had_error = false;
  const char* keys[] = {primary_slot_key(), backup_slot_key(), legacy_slot_key()};
  for (const char* key : keys) {
    const auto status = persistence_->erase_key(bsp.config_namespace, key);
    if (status != LocalPersistenceStatus::kOk && status != LocalPersistenceStatus::kNotFound) {
      had_error = true;
    }
  }

  return had_error ? PersistentCompanionMemoryStatus::kIoError
                   : PersistentCompanionMemoryStatus::kOk;
}

PersistentCompanionMemoryStatus PersistentCompanionMemoryStore::reset_profile() {
  return save(default_profile_record());
}

ncos::core::contracts::PersistedCompanionMemoryRecord
PersistentCompanionMemoryStore::default_profile_record() {
  return ncos::core::contracts::make_default_persisted_companion_memory();
}

const char* PersistentCompanionMemoryStore::primary_slot_key() {
  return PrimarySlotKey;
}

const char* PersistentCompanionMemoryStore::backup_slot_key() {
  return BackupSlotKey;
}

const char* PersistentCompanionMemoryStore::legacy_slot_key() {
  return active_storage_platform_bsp().persistent_companion_memory_key;
}

}  // namespace ncos::drivers::storage
