#include "core/contracts/storage_runtime_contracts.hpp"

namespace {

constexpr uint32_t RuntimeConfigEnvelopeMagic = 0x4E434647U;
constexpr uint32_t Fnv1aSeed = 2166136261U;
constexpr uint32_t Fnv1aPrime = 16777619U;

uint32_t clamp_interval(uint32_t value, uint32_t min_value, uint32_t max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

uint32_t fnv1a_step(uint32_t checksum, uint8_t value) {
  return (checksum ^ value) * Fnv1aPrime;
}

uint32_t fnv1a_add_u32(uint32_t checksum, uint32_t value) {
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(value & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((value >> 8) & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((value >> 16) & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((value >> 24) & 0xFFU));
  return checksum;
}

uint32_t envelope_checksum(const ncos::core::contracts::PersistedRuntimeConfigEnvelope& envelope) {
  uint32_t checksum = Fnv1aSeed;
  checksum = fnv1a_add_u32(checksum, envelope.magic);
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(envelope.envelope_version));
  checksum = fnv1a_step(checksum, envelope.reserved);
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(envelope.payload_size & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((envelope.payload_size >> 8) & 0xFFU));
  checksum = fnv1a_add_u32(checksum, envelope.generation);
  checksum = fnv1a_add_u32(checksum,
                           ncos::core::contracts::persisted_runtime_config_checksum(envelope.payload));
  return checksum;
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

uint32_t persisted_runtime_config_checksum(const PersistedRuntimeConfigRecord& record) {
  uint32_t checksum = Fnv1aSeed;
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.schema_version));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.diagnostics_enabled ? 1U : 0U));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.cloud_sync_enabled ? 1U : 0U));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.cloud_bridge_enabled ? 1U : 0U));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.cloud_extension_enabled ? 1U : 0U));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.telemetry_enabled ? 1U : 0U));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.telemetry_export_off_device ? 1U : 0U));
  checksum = fnv1a_add_u32(checksum, record.cloud_sync_interval_ms);
  checksum = fnv1a_add_u32(checksum, record.telemetry_interval_ms);
  return checksum;
}

PersistedRuntimeConfigEnvelope make_persisted_runtime_config_envelope(
    const PersistedRuntimeConfigRecord& record,
    uint32_t generation) {
  PersistedRuntimeConfigEnvelope envelope{};
  envelope.magic = RuntimeConfigEnvelopeMagic;
  envelope.envelope_version = PersistedStorageEnvelopeVersion::kV1;
  envelope.reserved = 0;
  envelope.payload_size = static_cast<uint16_t>(sizeof(PersistedRuntimeConfigRecord));
  envelope.generation = generation;
  envelope.payload = record;
  (void)sanitize_persisted_runtime_config(&envelope.payload);
  envelope.checksum = envelope_checksum(envelope);
  return envelope;
}

bool sanitize_persisted_runtime_config_envelope(PersistedRuntimeConfigEnvelope* envelope) {
  if (envelope == nullptr) {
    return false;
  }

  envelope->magic = RuntimeConfigEnvelopeMagic;
  envelope->envelope_version = PersistedStorageEnvelopeVersion::kV1;
  envelope->reserved = 0;
  envelope->payload_size = static_cast<uint16_t>(sizeof(PersistedRuntimeConfigRecord));
  if (!sanitize_persisted_runtime_config(&envelope->payload)) {
    return false;
  }
  envelope->checksum = envelope_checksum(*envelope);
  return true;
}

bool is_valid_persisted_runtime_config_envelope(const PersistedRuntimeConfigEnvelope& envelope) {
  if (envelope.magic != RuntimeConfigEnvelopeMagic ||
      envelope.envelope_version != PersistedStorageEnvelopeVersion::kV1 ||
      envelope.payload_size != sizeof(PersistedRuntimeConfigRecord)) {
    return false;
  }

  if (!is_valid_persisted_runtime_config(envelope.payload)) {
    return false;
  }

  return envelope.checksum == envelope_checksum(envelope);
}

size_t persisted_runtime_config_envelope_size() {
  return sizeof(PersistedRuntimeConfigEnvelope);
}

}  // namespace ncos::core::contracts
