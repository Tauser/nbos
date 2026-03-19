#include "core/contracts/storage_runtime_contracts.hpp"

namespace {

constexpr uint32_t RuntimeConfigEnvelopeMagic = 0x4E434647U;
constexpr uint32_t CompanionMemoryEnvelopeMagic = 0x4E434D4DU;
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

uint8_t clamp_percent(uint8_t value) {
  return value > 100U ? 100U : value;
}

uint16_t clamp_revision(uint16_t value) {
  return value;
}

uint32_t fnv1a_step(uint32_t checksum, uint8_t value) {
  return (checksum ^ value) * Fnv1aPrime;
}

uint32_t fnv1a_add_u16(uint32_t checksum, uint16_t value) {
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(value & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((value >> 8) & 0xFFU));
  return checksum;
}

uint32_t fnv1a_add_u32(uint32_t checksum, uint32_t value) {
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(value & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((value >> 8) & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((value >> 16) & 0xFFU));
  checksum = fnv1a_step(checksum, static_cast<uint8_t>((value >> 24) & 0xFFU));
  return checksum;
}

bool is_valid_attention_channel(ncos::core::contracts::AttentionChannel channel) {
  using ncos::core::contracts::AttentionChannel;
  switch (channel) {
    case AttentionChannel::kVisual:
    case AttentionChannel::kAuditory:
    case AttentionChannel::kTouch:
    case AttentionChannel::kMultimodal:
      return true;
    default:
      return false;
  }
}

bool is_valid_attention_target(ncos::core::contracts::AttentionTarget target) {
  using ncos::core::contracts::AttentionTarget;
  switch (target) {
    case AttentionTarget::kNone:
    case AttentionTarget::kUser:
    case AttentionTarget::kStimulus:
    case AttentionTarget::kInternalTask:
      return true;
    default:
      return false;
  }
}

bool is_valid_habit_window(ncos::core::contracts::PersistedHabitWindow window) {
  using ncos::core::contracts::PersistedHabitWindow;
  switch (window) {
    case PersistedHabitWindow::kUnknown:
    case PersistedHabitWindow::kMorning:
    case PersistedHabitWindow::kAfternoon:
    case PersistedHabitWindow::kEvening:
    case PersistedHabitWindow::kNight:
    case PersistedHabitWindow::kMixed:
      return true;
    default:
      return false;
  }
}

bool is_valid_marked_event_kind(ncos::core::contracts::PersistedMarkedEventKind kind) {
  using ncos::core::contracts::PersistedMarkedEventKind;
  switch (kind) {
    case PersistedMarkedEventKind::kNone:
    case PersistedMarkedEventKind::kWarmGreeting:
    case PersistedMarkedEventKind::kTouchComfort:
    case PersistedMarkedEventKind::kCuriousStimulus:
    case PersistedMarkedEventKind::kCalmRecovery:
    case PersistedMarkedEventKind::kEnergyCare:
      return true;
    default:
      return false;
  }
}

bool sanitize_marked_event(ncos::core::contracts::PersistedMarkedEventRecord* event) {
  if (event == nullptr) {
    return false;
  }

  if (!is_valid_marked_event_kind(event->kind)) {
    event->kind = ncos::core::contracts::PersistedMarkedEventKind::kNone;
  }
  if (!is_valid_attention_target(event->target)) {
    event->target = ncos::core::contracts::AttentionTarget::kNone;
  }
  if (!is_valid_attention_channel(event->channel)) {
    event->channel = ncos::core::contracts::AttentionChannel::kVisual;
  }
  event->salience_percent = clamp_percent(event->salience_percent);
  event->last_revision = clamp_revision(event->last_revision);

  if (event->kind == ncos::core::contracts::PersistedMarkedEventKind::kNone) {
    event->target = ncos::core::contracts::AttentionTarget::kNone;
    event->salience_percent = 0;
    event->reinforcement_count = 0;
    event->last_revision = 0;
  }

  return true;
}

uint32_t envelope_checksum(const ncos::core::contracts::PersistedRuntimeConfigEnvelope& envelope) {
  uint32_t checksum = Fnv1aSeed;
  checksum = fnv1a_add_u32(checksum, envelope.magic);
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(envelope.envelope_version));
  checksum = fnv1a_step(checksum, envelope.reserved);
  checksum = fnv1a_add_u16(checksum, envelope.payload_size);
  checksum = fnv1a_add_u32(checksum, envelope.generation);
  checksum = fnv1a_add_u32(checksum,
                           ncos::core::contracts::persisted_runtime_config_checksum(envelope.payload));
  return checksum;
}

uint32_t envelope_checksum(const ncos::core::contracts::PersistedCompanionMemoryEnvelope& envelope) {
  uint32_t checksum = Fnv1aSeed;
  checksum = fnv1a_add_u32(checksum, envelope.magic);
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(envelope.envelope_version));
  checksum = fnv1a_step(checksum, envelope.reserved);
  checksum = fnv1a_add_u16(checksum, envelope.payload_size);
  checksum = fnv1a_add_u32(checksum, envelope.generation);
  checksum = fnv1a_add_u32(checksum,
                           ncos::core::contracts::persisted_companion_memory_checksum(envelope.payload));
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

const ncos::core::contracts::StorageDataPolicy PersistentCompanionMemoryPolicy{
    "persistent_companion_memory",
    true,
    ncos::core::contracts::StorageRetentionKind::kUntilOverwriteOrReset,
    0,
    true,
    true,
    true,
    true,
    ncos::core::contracts::StorageTransferMode::kBlocked,
    ncos::core::contracts::StorageTransferMode::kBlocked,
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

PersistedCompanionMemoryRecord make_default_persisted_companion_memory() {
  return PersistedCompanionMemoryRecord{};
}

bool sanitize_persisted_companion_memory(PersistedCompanionMemoryRecord* record) {
  if (record == nullptr) {
    return false;
  }

  record->schema_version = PersistedCompanionMemorySchemaVersion::kV1;
  record->preferences.social_warmth_preference_percent =
      clamp_percent(record->preferences.social_warmth_preference_percent);
  record->preferences.response_energy_preference_percent =
      clamp_percent(record->preferences.response_energy_preference_percent);
  record->preferences.stimulus_sensitivity_percent =
      clamp_percent(record->preferences.stimulus_sensitivity_percent);
  if (!is_valid_attention_channel(record->preferences.preferred_attention_channel)) {
    record->preferences.preferred_attention_channel = AttentionChannel::kTouch;
  }

  record->habits.touch_engagement_affinity_percent =
      clamp_percent(record->habits.touch_engagement_affinity_percent);
  record->habits.repeat_engagement_affinity_percent =
      clamp_percent(record->habits.repeat_engagement_affinity_percent);
  record->habits.calm_recovery_affinity_percent =
      clamp_percent(record->habits.calm_recovery_affinity_percent);
  if (!is_valid_habit_window(record->habits.preferred_engagement_window)) {
    record->habits.preferred_engagement_window = PersistedHabitWindow::kUnknown;
  }

  return sanitize_marked_event(&record->last_user_event) &&
         sanitize_marked_event(&record->last_companion_event) &&
         sanitize_marked_event(&record->last_environment_event);
}

bool is_valid_persisted_companion_memory(const PersistedCompanionMemoryRecord& record) {
  if (record.schema_version != PersistedCompanionMemorySchemaVersion::kV1) {
    return false;
  }

  if (record.preferences.social_warmth_preference_percent > 100U ||
      record.preferences.response_energy_preference_percent > 100U ||
      record.preferences.stimulus_sensitivity_percent > 100U ||
      !is_valid_attention_channel(record.preferences.preferred_attention_channel)) {
    return false;
  }

  if (record.habits.touch_engagement_affinity_percent > 100U ||
      record.habits.repeat_engagement_affinity_percent > 100U ||
      record.habits.calm_recovery_affinity_percent > 100U ||
      !is_valid_habit_window(record.habits.preferred_engagement_window)) {
    return false;
  }

  const PersistedMarkedEventRecord* events[] = {
      &record.last_user_event,
      &record.last_companion_event,
      &record.last_environment_event,
  };
  for (const auto* event : events) {
    if (!is_valid_marked_event_kind(event->kind) || !is_valid_attention_target(event->target) ||
        !is_valid_attention_channel(event->channel) || event->salience_percent > 100U) {
      return false;
    }
    if (event->kind == PersistedMarkedEventKind::kNone &&
        (event->target != AttentionTarget::kNone || event->salience_percent != 0U ||
         event->reinforcement_count != 0U || event->last_revision != 0U)) {
      return false;
    }
  }

  return true;
}

const StorageDataPolicy& storage_data_policy(StorageDataClass data_class) {
  switch (data_class) {
    case StorageDataClass::kRuntimeConfig:
      return RuntimeConfigPolicy;
    case StorageDataClass::kPersistentCompanionMemory:
      return PersistentCompanionMemoryPolicy;
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

uint32_t persisted_companion_memory_checksum(const PersistedCompanionMemoryRecord& record) {
  uint32_t checksum = Fnv1aSeed;
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.schema_version));
  checksum = fnv1a_step(checksum, record.preferences.social_warmth_preference_percent);
  checksum = fnv1a_step(checksum, record.preferences.response_energy_preference_percent);
  checksum = fnv1a_step(checksum, record.preferences.stimulus_sensitivity_percent);
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.preferences.preferred_attention_channel));
  checksum = fnv1a_step(checksum, record.habits.touch_engagement_affinity_percent);
  checksum = fnv1a_step(checksum, record.habits.repeat_engagement_affinity_percent);
  checksum = fnv1a_step(checksum, record.habits.calm_recovery_affinity_percent);
  checksum = fnv1a_step(checksum, static_cast<uint8_t>(record.habits.preferred_engagement_window));
  checksum = fnv1a_add_u16(checksum, record.habits.reinforced_sessions);

  const PersistedMarkedEventRecord* events[] = {
      &record.last_user_event,
      &record.last_companion_event,
      &record.last_environment_event,
  };
  for (const auto* event : events) {
    checksum = fnv1a_step(checksum, static_cast<uint8_t>(event->kind));
    checksum = fnv1a_step(checksum, static_cast<uint8_t>(event->target));
    checksum = fnv1a_step(checksum, static_cast<uint8_t>(event->channel));
    checksum = fnv1a_step(checksum, event->salience_percent);
    checksum = fnv1a_add_u16(checksum, event->reinforcement_count);
    checksum = fnv1a_add_u16(checksum, event->last_revision);
  }

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

PersistedCompanionMemoryEnvelope make_persisted_companion_memory_envelope(
    const PersistedCompanionMemoryRecord& record,
    uint32_t generation) {
  PersistedCompanionMemoryEnvelope envelope{};
  envelope.magic = CompanionMemoryEnvelopeMagic;
  envelope.envelope_version = PersistedStorageEnvelopeVersion::kV1;
  envelope.reserved = 0;
  envelope.payload_size = static_cast<uint16_t>(sizeof(PersistedCompanionMemoryRecord));
  envelope.generation = generation;
  envelope.payload = record;
  (void)sanitize_persisted_companion_memory(&envelope.payload);
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

bool sanitize_persisted_companion_memory_envelope(PersistedCompanionMemoryEnvelope* envelope) {
  if (envelope == nullptr) {
    return false;
  }

  envelope->magic = CompanionMemoryEnvelopeMagic;
  envelope->envelope_version = PersistedStorageEnvelopeVersion::kV1;
  envelope->reserved = 0;
  envelope->payload_size = static_cast<uint16_t>(sizeof(PersistedCompanionMemoryRecord));
  if (!sanitize_persisted_companion_memory(&envelope->payload)) {
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

bool is_valid_persisted_companion_memory_envelope(const PersistedCompanionMemoryEnvelope& envelope) {
  if (envelope.magic != CompanionMemoryEnvelopeMagic ||
      envelope.envelope_version != PersistedStorageEnvelopeVersion::kV1 ||
      envelope.payload_size != sizeof(PersistedCompanionMemoryRecord)) {
    return false;
  }

  if (!is_valid_persisted_companion_memory(envelope.payload)) {
    return false;
  }

  return envelope.checksum == envelope_checksum(envelope);
}

size_t persisted_runtime_config_envelope_size() {
  return sizeof(PersistedRuntimeConfigEnvelope);
}

size_t persisted_companion_memory_envelope_size() {
  return sizeof(PersistedCompanionMemoryEnvelope);
}

}  // namespace ncos::core::contracts
