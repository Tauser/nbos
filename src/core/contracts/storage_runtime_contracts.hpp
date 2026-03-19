#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

enum class PersistedRuntimeConfigSchemaVersion : uint8_t {
  kV1 = 1,
};

enum class PersistedCompanionMemorySchemaVersion : uint8_t {
  kV1 = 1,
};

enum class PersistedStorageEnvelopeVersion : uint8_t {
  kV1 = 1,
};

enum class StorageDataClass : uint8_t {
  kRuntimeConfig = 0,
  kPersistentCompanionMemory,
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

enum class PersistedHabitWindow : uint8_t {
  kUnknown = 1,
  kMorning = 2,
  kAfternoon = 3,
  kEvening = 4,
  kNight = 5,
  kMixed = 6,
};

enum class PersistedMarkedEventKind : uint8_t {
  kNone = 1,
  kWarmGreeting = 2,
  kTouchComfort = 3,
  kCuriousStimulus = 4,
  kCalmRecovery = 5,
  kEnergyCare = 6,
};

struct PersistedCompanionPreferenceRecord {
  uint8_t social_warmth_preference_percent = 50;
  uint8_t response_energy_preference_percent = 50;
  uint8_t stimulus_sensitivity_percent = 50;
  AttentionChannel preferred_attention_channel = AttentionChannel::kTouch;
};

struct PersistedCompanionHabitRecord {
  uint8_t touch_engagement_affinity_percent = 50;
  uint8_t repeat_engagement_affinity_percent = 50;
  uint8_t calm_recovery_affinity_percent = 50;
  PersistedHabitWindow preferred_engagement_window = PersistedHabitWindow::kUnknown;
  uint16_t reinforced_sessions = 0;
};

struct PersistedMarkedEventRecord {
  PersistedMarkedEventKind kind = PersistedMarkedEventKind::kNone;
  AttentionTarget target = AttentionTarget::kNone;
  AttentionChannel channel = AttentionChannel::kVisual;
  uint8_t salience_percent = 0;
  uint16_t reinforcement_count = 0;
  uint16_t last_revision = 0;
};

struct PersistedCompanionMemoryRecord {
  PersistedCompanionMemorySchemaVersion schema_version =
      PersistedCompanionMemorySchemaVersion::kV1;
  PersistedCompanionPreferenceRecord preferences{};
  PersistedCompanionHabitRecord habits{};
  PersistedMarkedEventRecord last_user_event{};
  PersistedMarkedEventRecord last_companion_event{};
  PersistedMarkedEventRecord last_environment_event{};
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

struct PersistedCompanionMemoryEnvelope {
  uint32_t magic = 0;
  PersistedStorageEnvelopeVersion envelope_version = PersistedStorageEnvelopeVersion::kV1;
  uint8_t reserved = 0;
  uint16_t payload_size = 0;
  uint32_t generation = 0;
  uint32_t checksum = 0;
  PersistedCompanionMemoryRecord payload{};
};

PersistedRuntimeConfigRecord make_default_persisted_runtime_config();
bool sanitize_persisted_runtime_config(PersistedRuntimeConfigRecord* record);
bool is_valid_persisted_runtime_config(const PersistedRuntimeConfigRecord& record);
bool is_importable_persisted_runtime_config(const PersistedRuntimeConfigRecord& record);
PersistedCompanionMemoryRecord make_default_persisted_companion_memory();
bool sanitize_persisted_companion_memory(PersistedCompanionMemoryRecord* record);
bool is_valid_persisted_companion_memory(const PersistedCompanionMemoryRecord& record);
const StorageDataPolicy& storage_data_policy(StorageDataClass data_class);
bool storage_data_is_portable(StorageDataClass data_class);
uint32_t persisted_runtime_config_checksum(const PersistedRuntimeConfigRecord& record);
uint32_t persisted_companion_memory_checksum(const PersistedCompanionMemoryRecord& record);
PersistedRuntimeConfigEnvelope make_persisted_runtime_config_envelope(
    const PersistedRuntimeConfigRecord& record,
    uint32_t generation);
PersistedCompanionMemoryEnvelope make_persisted_companion_memory_envelope(
    const PersistedCompanionMemoryRecord& record,
    uint32_t generation);
bool sanitize_persisted_runtime_config_envelope(PersistedRuntimeConfigEnvelope* envelope);
bool sanitize_persisted_companion_memory_envelope(PersistedCompanionMemoryEnvelope* envelope);
bool is_valid_persisted_runtime_config_envelope(const PersistedRuntimeConfigEnvelope& envelope);
bool is_valid_persisted_companion_memory_envelope(const PersistedCompanionMemoryEnvelope& envelope);
size_t persisted_runtime_config_envelope_size();
size_t persisted_companion_memory_envelope_size();

}  // namespace ncos::core::contracts
