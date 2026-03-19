#pragma once

#include <stdint.h>

#include "core/contracts/storage_runtime_contracts.hpp"
#include "drivers/storage/local_persistence.hpp"

namespace ncos::drivers::storage {

enum class PersistentCompanionMemoryStatus : uint8_t {
  kOk = 0,
  kNotFound,
  kUnavailable,
  kInvalidData,
  kIoError,
};

enum class PersistentCompanionMemoryRecoveryPath : uint8_t {
  kNone = 0,
  kDirectLoad,
  kRecoveredLastKnownGood,
  kRecoveredLegacy,
  kResetProfileBaseline,
};

struct PersistentCompanionMemoryLoadResult {
  PersistentCompanionMemoryStatus status = PersistentCompanionMemoryStatus::kNotFound;
  PersistentCompanionMemoryRecoveryPath recovery_path =
      PersistentCompanionMemoryRecoveryPath::kNone;
  bool repaired_storage = false;
};

class PersistentCompanionMemoryStore final {
 public:
  PersistentCompanionMemoryStore();
  explicit PersistentCompanionMemoryStore(LocalPersistence* persistence);

  PersistentCompanionMemoryLoadResult load_with_recovery(
      ncos::core::contracts::PersistedCompanionMemoryRecord* out_record);
  PersistentCompanionMemoryStatus load(
      ncos::core::contracts::PersistedCompanionMemoryRecord* out_record);
  PersistentCompanionMemoryStatus save(
      const ncos::core::contracts::PersistedCompanionMemoryRecord& record);
  PersistentCompanionMemoryStatus reset();
  PersistentCompanionMemoryStatus reset_profile();

  static ncos::core::contracts::PersistedCompanionMemoryRecord default_profile_record();
  static const char* primary_slot_key();
  static const char* backup_slot_key();
  static const char* legacy_slot_key();

 private:
  LocalPersistence owned_persistence_{};
  LocalPersistence* persistence_ = nullptr;
};

}  // namespace ncos::drivers::storage
