#include <string.h>

#include <unity.h>

#include "core/contracts/storage_runtime_contracts.hpp"
#include "drivers/storage/persistent_companion_memory_store.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/storage_runtime_contracts.cpp"
#include "drivers/storage/storage_platform_bsp.cpp"
#include "drivers/storage/local_persistence.cpp"
#include "drivers/storage/persistent_companion_memory_store.cpp"

void setUp() {
  ncos::drivers::storage::PersistentCompanionMemoryStore store;
  (void)store.reset();
}

void tearDown() {}

void test_persistent_companion_memory_policy_separates_persistent_memory_from_session_memory() {
  const auto& memory_policy = ncos::core::contracts::storage_data_policy(
      ncos::core::contracts::StorageDataClass::kPersistentCompanionMemory);
  const auto& session_policy = ncos::core::contracts::storage_data_policy(
      ncos::core::contracts::StorageDataClass::kShortSessionMemory);

  TEST_ASSERT_TRUE(memory_policy.persist_locally);
  TEST_ASSERT_TRUE(memory_policy.integrity_checked);
  TEST_ASSERT_TRUE(memory_policy.erase_on_user_reset);
  TEST_ASSERT_TRUE(memory_policy.erase_on_factory_reset);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::core::contracts::StorageRetentionKind::kUntilOverwriteOrReset),
                          static_cast<uint8_t>(memory_policy.retention));
  TEST_ASSERT_FALSE(ncos::core::contracts::storage_data_is_portable(
      ncos::core::contracts::StorageDataClass::kPersistentCompanionMemory));

  TEST_ASSERT_FALSE(session_policy.persist_locally);
  TEST_ASSERT_FALSE(ncos::core::contracts::storage_data_is_portable(
      ncos::core::contracts::StorageDataClass::kShortSessionMemory));
}

void test_persistent_companion_memory_envelope_is_versioned_and_checksum_protected() {
  auto record = ncos::core::contracts::make_default_persisted_companion_memory();
  record.last_user_event.kind = ncos::core::contracts::PersistedMarkedEventKind::kTouchComfort;
  record.last_user_event.target = ncos::core::contracts::AttentionTarget::kUser;
  record.last_user_event.channel = ncos::core::contracts::AttentionChannel::kTouch;
  record.last_user_event.salience_percent = 88;
  record.last_user_event.reinforcement_count = 3;
  record.last_user_event.last_revision = 7;
  const auto envelope =
      ncos::core::contracts::make_persisted_companion_memory_envelope(record, 9);

  TEST_ASSERT_EQUAL_UINT32(9U, envelope.generation);
  TEST_ASSERT_EQUAL_UINT16(sizeof(ncos::core::contracts::PersistedCompanionMemoryRecord),
                           envelope.payload_size);
  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid_persisted_companion_memory_envelope(envelope));

  auto corrupted = envelope;
  corrupted.payload.preferences.response_energy_preference_percent = 101;
  TEST_ASSERT_FALSE(ncos::core::contracts::is_valid_persisted_companion_memory_envelope(corrupted));
}

void test_persistent_companion_memory_store_roundtrips_sanitized_record() {
  ncos::drivers::storage::PersistentCompanionMemoryStore store;
  ncos::core::contracts::PersistedCompanionMemoryRecord record{};
  record.preferences.social_warmth_preference_percent = 120;
  record.preferences.response_energy_preference_percent = 72;
  record.preferences.stimulus_sensitivity_percent = 130;
  record.preferences.preferred_attention_channel =
      static_cast<ncos::core::contracts::AttentionChannel>(99);
  record.habits.touch_engagement_affinity_percent = 85;
  record.habits.repeat_engagement_affinity_percent = 140;
  record.habits.calm_recovery_affinity_percent = 33;
  record.habits.preferred_engagement_window =
      static_cast<ncos::core::contracts::PersistedHabitWindow>(99);
  record.last_environment_event.kind = ncos::core::contracts::PersistedMarkedEventKind::kCuriousStimulus;
  record.last_environment_event.target = ncos::core::contracts::AttentionTarget::kStimulus;
  record.last_environment_event.channel = ncos::core::contracts::AttentionChannel::kAuditory;
  record.last_environment_event.salience_percent = 135;
  record.last_environment_event.reinforcement_count = 4;
  record.last_environment_event.last_revision = 12;

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryStatus::kOk),
                          static_cast<uint8_t>(store.save(record)));

  ncos::core::contracts::PersistedCompanionMemoryRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryRecoveryPath::kDirectLoad),
      static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_FALSE(load_result.repaired_storage);
  TEST_ASSERT_EQUAL_UINT8(100U, loaded.preferences.social_warmth_preference_percent);
  TEST_ASSERT_EQUAL_UINT8(72U, loaded.preferences.response_energy_preference_percent);
  TEST_ASSERT_EQUAL_UINT8(100U, loaded.preferences.stimulus_sensitivity_percent);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::core::contracts::AttentionChannel::kTouch),
                          static_cast<uint8_t>(loaded.preferences.preferred_attention_channel));
  TEST_ASSERT_EQUAL_UINT8(100U, loaded.habits.repeat_engagement_affinity_percent);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::core::contracts::PersistedHabitWindow::kUnknown),
                          static_cast<uint8_t>(loaded.habits.preferred_engagement_window));
  TEST_ASSERT_EQUAL_UINT8(100U, loaded.last_environment_event.salience_percent);
}

void test_persistent_companion_memory_store_recovers_last_known_good_and_repairs_storage() {
  ncos::drivers::storage::LocalPersistence persistence;
  ncos::drivers::storage::PersistentCompanionMemoryStore store(&persistence);

  auto first = ncos::core::contracts::make_default_persisted_companion_memory();
  first.preferences.social_warmth_preference_percent = 62;
  first.habits.touch_engagement_affinity_percent = 74;
  first.last_user_event.kind = ncos::core::contracts::PersistedMarkedEventKind::kTouchComfort;
  first.last_user_event.target = ncos::core::contracts::AttentionTarget::kUser;
  first.last_user_event.channel = ncos::core::contracts::AttentionChannel::kTouch;
  first.last_user_event.salience_percent = 84;
  first.last_user_event.reinforcement_count = 2;
  first.last_user_event.last_revision = 1;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryStatus::kOk),
                          static_cast<uint8_t>(store.save(first)));

  auto second = first;
  second.preferences.response_energy_preference_percent = 67;
  second.last_companion_event.kind = ncos::core::contracts::PersistedMarkedEventKind::kWarmGreeting;
  second.last_companion_event.target = ncos::core::contracts::AttentionTarget::kUser;
  second.last_companion_event.channel = ncos::core::contracts::AttentionChannel::kTouch;
  second.last_companion_event.salience_percent = 70;
  second.last_companion_event.reinforcement_count = 3;
  second.last_companion_event.last_revision = 2;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryStatus::kOk),
                          static_cast<uint8_t>(store.save(second)));

  uint8_t garbage[sizeof(ncos::core::contracts::PersistedCompanionMemoryEnvelope)] = {};
  memset(garbage, 0xA5, sizeof(garbage));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
                          static_cast<uint8_t>(persistence.write_blob(
                              "ncos",
                              ncos::drivers::storage::PersistentCompanionMemoryStore::backup_slot_key(),
                              garbage,
                              sizeof(garbage))));

  ncos::core::contracts::PersistedCompanionMemoryRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryRecoveryPath::kRecoveredLastKnownGood),
      static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_TRUE(load_result.repaired_storage);
  TEST_ASSERT_EQUAL_UINT8(62U, loaded.preferences.social_warmth_preference_percent);
  TEST_ASSERT_EQUAL_UINT8(74U, loaded.habits.touch_engagement_affinity_percent);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::core::contracts::PersistedMarkedEventKind::kTouchComfort),
                          static_cast<uint8_t>(loaded.last_user_event.kind));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::core::contracts::PersistedMarkedEventKind::kNone),
                          static_cast<uint8_t>(loaded.last_companion_event.kind));
}

void test_persistent_companion_memory_store_resets_profile_to_default_when_no_valid_snapshot_remains() {
  ncos::drivers::storage::LocalPersistence persistence;
  ncos::drivers::storage::PersistentCompanionMemoryStore store(&persistence);

  uint8_t garbage[sizeof(ncos::core::contracts::PersistedCompanionMemoryEnvelope)] = {};
  memset(garbage, 0x5A, sizeof(garbage));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
                          static_cast<uint8_t>(persistence.write_blob(
                              "ncos",
                              ncos::drivers::storage::PersistentCompanionMemoryStore::primary_slot_key(),
                              garbage,
                              sizeof(garbage))));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
                          static_cast<uint8_t>(persistence.write_blob(
                              "ncos",
                              ncos::drivers::storage::PersistentCompanionMemoryStore::backup_slot_key(),
                              garbage,
                              sizeof(garbage))));

  ncos::core::contracts::PersistedCompanionMemoryRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  const auto defaults = ncos::drivers::storage::PersistentCompanionMemoryStore::default_profile_record();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::PersistentCompanionMemoryRecoveryPath::kResetProfileBaseline),
      static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_TRUE(load_result.repaired_storage);
  TEST_ASSERT_EQUAL_UINT8(defaults.preferences.social_warmth_preference_percent,
                          loaded.preferences.social_warmth_preference_percent);
  TEST_ASSERT_EQUAL_UINT8(defaults.habits.touch_engagement_affinity_percent,
                          loaded.habits.touch_engagement_affinity_percent);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(defaults.last_user_event.kind),
                          static_cast<uint8_t>(loaded.last_user_event.kind));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_persistent_companion_memory_policy_separates_persistent_memory_from_session_memory);
  RUN_TEST(test_persistent_companion_memory_envelope_is_versioned_and_checksum_protected);
  RUN_TEST(test_persistent_companion_memory_store_roundtrips_sanitized_record);
  RUN_TEST(test_persistent_companion_memory_store_recovers_last_known_good_and_repairs_storage);
  RUN_TEST(test_persistent_companion_memory_store_resets_profile_to_default_when_no_valid_snapshot_remains);
  return UNITY_END();
}
