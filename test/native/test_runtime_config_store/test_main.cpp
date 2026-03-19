#include <unity.h>

#include "config/system_config.hpp"
#include "core/contracts/storage_runtime_contracts.hpp"
#include "drivers/storage/runtime_config_store.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/storage_runtime_contracts.cpp"
#include "drivers/storage/storage_platform_bsp.cpp"
#include "drivers/storage/local_persistence.cpp"
#include "drivers/storage/runtime_config_store.cpp"

void setUp() {
  ncos::drivers::storage::RuntimeConfigStore store;
  (void)store.reset();
}

void tearDown() {}

void test_storage_policy_limits_persistence_to_runtime_config_baseline() {
  const auto& runtime_policy =
      ncos::core::contracts::storage_data_policy(ncos::core::contracts::StorageDataClass::kRuntimeConfig);
  const auto& session_policy = ncos::core::contracts::storage_data_policy(
      ncos::core::contracts::StorageDataClass::kShortSessionMemory);
  const auto& adaptive_policy = ncos::core::contracts::storage_data_policy(
      ncos::core::contracts::StorageDataClass::kAdaptivePersonality);

  TEST_ASSERT_TRUE(runtime_policy.persist_locally);
  TEST_ASSERT_TRUE(runtime_policy.integrity_checked);
  TEST_ASSERT_TRUE(runtime_policy.erase_on_schema_mismatch);
  TEST_ASSERT_TRUE(runtime_policy.erase_on_user_reset);
  TEST_ASSERT_TRUE(runtime_policy.erase_on_factory_reset);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::core::contracts::StorageRetentionKind::kUntilOverwriteOrReset),
                          static_cast<uint8_t>(runtime_policy.retention));
  TEST_ASSERT_TRUE(
      ncos::core::contracts::storage_data_is_portable(ncos::core::contracts::StorageDataClass::kRuntimeConfig));

  TEST_ASSERT_FALSE(session_policy.persist_locally);
  TEST_ASSERT_FALSE(
      ncos::core::contracts::storage_data_is_portable(ncos::core::contracts::StorageDataClass::kShortSessionMemory));
  TEST_ASSERT_FALSE(adaptive_policy.persist_locally);
  TEST_ASSERT_FALSE(
      ncos::core::contracts::storage_data_is_portable(ncos::core::contracts::StorageDataClass::kAdaptivePersonality));
}

void test_runtime_config_envelope_is_versioned_and_checksum_protected() {
  auto record = ncos::core::contracts::make_default_persisted_runtime_config();
  record.cloud_sync_enabled = true;
  const auto envelope =
      ncos::core::contracts::make_persisted_runtime_config_envelope(record, 7);

  TEST_ASSERT_EQUAL_UINT32(7U, envelope.generation);
  TEST_ASSERT_EQUAL_UINT16(sizeof(ncos::core::contracts::PersistedRuntimeConfigRecord), envelope.payload_size);
  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid_persisted_runtime_config_envelope(envelope));

  auto corrupted = envelope;
  corrupted.payload.telemetry_enabled = !corrupted.payload.telemetry_enabled;
  TEST_ASSERT_FALSE(ncos::core::contracts::is_valid_persisted_runtime_config_envelope(corrupted));
}

void test_runtime_config_store_roundtrips_sanitized_record() {
  ncos::drivers::storage::RuntimeConfigStore store;
  ncos::core::contracts::PersistedRuntimeConfigRecord record{};
  record.diagnostics_enabled = false;
  record.cloud_sync_enabled = true;
  record.cloud_bridge_enabled = true;
  record.telemetry_enabled = true;
  record.cloud_sync_interval_ms = 999999U;
  record.telemetry_interval_ms = 10U;

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(record)));

  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::RuntimeConfigRecoveryPath::kDirectLoad),
                          static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_FALSE(load_result.repaired_storage);
  TEST_ASSERT_FALSE(loaded.diagnostics_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_sync_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_bridge_enabled);
  TEST_ASSERT_TRUE(loaded.telemetry_enabled);
  TEST_ASSERT_EQUAL_UINT32(60000U, loaded.cloud_sync_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(1000U, loaded.telemetry_interval_ms);
}

void test_runtime_config_store_recovers_last_known_good_and_repairs_storage() {
  ncos::drivers::storage::LocalPersistence persistence;
  ncos::drivers::storage::RuntimeConfigStore store(&persistence);

  ncos::core::contracts::PersistedRuntimeConfigRecord first{};
  first.diagnostics_enabled = false;
  first.cloud_sync_enabled = true;
  first.cloud_sync_interval_ms = 4200U;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(first)));

  ncos::core::contracts::PersistedRuntimeConfigRecord second{};
  second.telemetry_enabled = true;
  second.telemetry_interval_ms = 18000U;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(second)));

  uint8_t garbage[sizeof(ncos::core::contracts::PersistedRuntimeConfigEnvelope)] = {};
  garbage[0] = 0xAAU;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
      static_cast<uint8_t>(persistence.write_blob("ncos",
                                                  ncos::drivers::storage::RuntimeConfigStore::backup_slot_key(),
                                                  garbage,
                                                  sizeof(garbage))));

  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::RuntimeConfigRecoveryPath::kRecoveredLastKnownGood),
      static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_TRUE(load_result.repaired_storage);
  TEST_ASSERT_FALSE(loaded.diagnostics_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_sync_enabled);
  TEST_ASSERT_EQUAL_UINT32(4200U, loaded.cloud_sync_interval_ms);
  TEST_ASSERT_FALSE(loaded.telemetry_enabled);

  ncos::core::contracts::PersistedRuntimeConfigRecord reloaded{};
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.load(&reloaded)));
  TEST_ASSERT_EQUAL_UINT32(4200U, reloaded.cloud_sync_interval_ms);
}

void test_runtime_config_store_loads_legacy_single_slot_record_and_migrates() {
  ncos::drivers::storage::LocalPersistence persistence;
  ncos::drivers::storage::RuntimeConfigStore store(&persistence);

  auto legacy = ncos::core::contracts::make_default_persisted_runtime_config();
  legacy.cloud_extension_enabled = true;
  legacy.telemetry_enabled = true;
  legacy.telemetry_interval_ms = 22000U;
  TEST_ASSERT_TRUE(ncos::core::contracts::sanitize_persisted_runtime_config(&legacy));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
      static_cast<uint8_t>(persistence.write_blob("ncos",
                                                  ncos::drivers::storage::RuntimeConfigStore::legacy_slot_key(),
                                                  &legacy,
                                                  sizeof(legacy))));

  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::RuntimeConfigRecoveryPath::kRecoveredLegacy),
                          static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_TRUE(load_result.repaired_storage);
  TEST_ASSERT_TRUE(loaded.cloud_extension_enabled);
  TEST_ASSERT_TRUE(loaded.telemetry_enabled);
  TEST_ASSERT_EQUAL_UINT32(22000U, loaded.telemetry_interval_ms);

  uint8_t legacy_raw[sizeof(legacy)] = {};
  size_t legacy_size = 0;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kNotFound),
      static_cast<uint8_t>(persistence.read_blob("ncos",
                                                 ncos::drivers::storage::RuntimeConfigStore::legacy_slot_key(),
                                                 legacy_raw,
                                                 sizeof(legacy_raw),
                                                 &legacy_size)));
}

void test_runtime_config_store_resets_profile_to_default_when_no_valid_snapshot_remains() {
  ncos::drivers::storage::LocalPersistence persistence;
  ncos::drivers::storage::RuntimeConfigStore store(&persistence);

  uint8_t garbage[sizeof(ncos::core::contracts::PersistedRuntimeConfigEnvelope)] = {};
  memset(garbage, 0x5AU, sizeof(garbage));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
      static_cast<uint8_t>(persistence.write_blob("ncos",
                                                  ncos::drivers::storage::RuntimeConfigStore::primary_slot_key(),
                                                  garbage,
                                                  sizeof(garbage))));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
      static_cast<uint8_t>(persistence.write_blob("ncos",
                                                  ncos::drivers::storage::RuntimeConfigStore::backup_slot_key(),
                                                  garbage,
                                                  sizeof(garbage))));

  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  const auto defaults = ncos::drivers::storage::RuntimeConfigStore::default_profile_record();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::RuntimeConfigRecoveryPath::kResetProfileBaseline),
      static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_TRUE(load_result.repaired_storage);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(defaults.schema_version), static_cast<uint8_t>(loaded.schema_version));
  TEST_ASSERT_EQUAL(defaults.diagnostics_enabled, loaded.diagnostics_enabled);
  TEST_ASSERT_EQUAL(defaults.cloud_sync_enabled, loaded.cloud_sync_enabled);
  TEST_ASSERT_EQUAL_UINT32(defaults.cloud_sync_interval_ms, loaded.cloud_sync_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(defaults.telemetry_interval_ms, loaded.telemetry_interval_ms);
}

void test_runtime_config_store_reset_profile_writes_default_record() {
  ncos::drivers::storage::RuntimeConfigStore store;
  ncos::core::contracts::PersistedRuntimeConfigRecord record{};
  record.diagnostics_enabled = false;
  record.cloud_sync_enabled = true;
  record.telemetry_enabled = true;
  record.cloud_sync_interval_ms = 4200U;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(record)));

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.reset_profile()));

  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.load(&loaded)));
  const auto defaults = ncos::drivers::storage::RuntimeConfigStore::default_profile_record();
  TEST_ASSERT_EQUAL(defaults.diagnostics_enabled, loaded.diagnostics_enabled);
  TEST_ASSERT_EQUAL(defaults.cloud_sync_enabled, loaded.cloud_sync_enabled);
  TEST_ASSERT_EQUAL(defaults.telemetry_enabled, loaded.telemetry_enabled);
  TEST_ASSERT_EQUAL_UINT32(defaults.cloud_sync_interval_ms, loaded.cloud_sync_interval_ms);
}

void test_runtime_config_store_resets_all_slots_to_not_found() {
  ncos::drivers::storage::RuntimeConfigStore store;
  const auto defaults = ncos::core::contracts::make_default_persisted_runtime_config();

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(defaults)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.reset()));

  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kNotFound),
                          static_cast<uint8_t>(store.load(&loaded)));
}

void test_runtime_config_store_applies_persisted_subset_over_runtime_config() {
  auto runtime_config = ncos::config::make_runtime_config();
  ncos::core::contracts::PersistedRuntimeConfigRecord record{};
  record.diagnostics_enabled = false;
  record.cloud_sync_enabled = true;
  record.cloud_extension_enabled = true;
  record.telemetry_enabled = true;
  record.telemetry_export_off_device = true;
  record.cloud_sync_interval_ms = 4200U;
  record.telemetry_interval_ms = 15000U;

  ncos::drivers::storage::RuntimeConfigStore::apply_record_to_runtime_config(record, &runtime_config);

  TEST_ASSERT_FALSE(runtime_config.diagnostics_enabled);
  TEST_ASSERT_TRUE(runtime_config.cloud_sync_enabled);
  TEST_ASSERT_TRUE(runtime_config.cloud_extension_enabled);
  TEST_ASSERT_TRUE(runtime_config.telemetry_enabled);
  TEST_ASSERT_TRUE(runtime_config.telemetry_export_off_device);
  TEST_ASSERT_EQUAL_UINT32(4200U, runtime_config.cloud_sync_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(15000U, runtime_config.telemetry_interval_ms);
}

void test_runtime_config_store_export_import_policy_stays_manual_and_sanitized() {
  auto runtime_config = ncos::config::make_runtime_config();
  runtime_config.cloud_sync_enabled = true;
  runtime_config.telemetry_enabled = true;
  runtime_config.telemetry_export_off_device = true;
  runtime_config.cloud_sync_interval_ms = 65000U;
  runtime_config.telemetry_interval_ms = 200U;

  const auto record = ncos::drivers::storage::RuntimeConfigStore::capture_runtime_config(runtime_config);
  TEST_ASSERT_TRUE(ncos::drivers::storage::RuntimeConfigStore::is_exportable_record(record));
  TEST_ASSERT_TRUE(
      ncos::drivers::storage::RuntimeConfigStore::apply_import_record_to_runtime_config(record, &runtime_config));
  TEST_ASSERT_EQUAL_UINT32(60000U, runtime_config.cloud_sync_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(1000U, runtime_config.telemetry_interval_ms);
}

void test_runtime_config_store_rejects_import_with_unknown_schema() {
  auto runtime_config = ncos::config::make_runtime_config();
  ncos::core::contracts::PersistedRuntimeConfigRecord record{};
  record.schema_version = static_cast<ncos::core::contracts::PersistedRuntimeConfigSchemaVersion>(99);
  record.cloud_sync_enabled = true;
  record.telemetry_enabled = true;

  TEST_ASSERT_FALSE(ncos::drivers::storage::RuntimeConfigStore::is_exportable_record(record));
  TEST_ASSERT_FALSE(
      ncos::drivers::storage::RuntimeConfigStore::apply_import_record_to_runtime_config(record, &runtime_config));
  TEST_ASSERT_FALSE(runtime_config.cloud_sync_enabled);
  TEST_ASSERT_FALSE(runtime_config.telemetry_enabled);
}

void test_runtime_config_store_survives_reboot_with_latest_committed_snapshot() {
  ncos::drivers::storage::LocalPersistence persistence;
  ncos::drivers::storage::RuntimeConfigStore writer(&persistence);

  auto first = ncos::core::contracts::make_default_persisted_runtime_config();
  first.cloud_sync_enabled = true;
  first.cloud_sync_interval_ms = 3200U;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(writer.save(first)));

  auto second = first;
  second.telemetry_enabled = true;
  second.telemetry_interval_ms = 18000U;
  second.cloud_extension_enabled = true;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(writer.save(second)));

  ncos::drivers::storage::RuntimeConfigStore after_reboot(&persistence);
  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  const auto load_result = after_reboot.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::RuntimeConfigRecoveryPath::kDirectLoad),
                          static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_FALSE(load_result.repaired_storage);
  TEST_ASSERT_TRUE(loaded.cloud_sync_enabled);
  TEST_ASSERT_TRUE(loaded.telemetry_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_extension_enabled);
  TEST_ASSERT_EQUAL_UINT32(18000U, loaded.telemetry_interval_ms);
}

void test_runtime_config_store_survives_interrupted_write_across_reboot() {
  ncos::drivers::storage::LocalPersistence persistence;
  ncos::drivers::storage::RuntimeConfigStore store(&persistence);

  auto first = ncos::core::contracts::make_default_persisted_runtime_config();
  first.cloud_sync_enabled = true;
  first.cloud_sync_interval_ms = 4200U;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(first)));

  auto second = first;
  second.telemetry_enabled = true;
  second.telemetry_interval_ms = 17000U;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(second)));

  uint8_t garbage[sizeof(ncos::core::contracts::PersistedRuntimeConfigEnvelope)] = {};
  memset(garbage, 0xA5, sizeof(garbage));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::LocalPersistenceStatus::kOk),
      static_cast<uint8_t>(persistence.write_blob("ncos",
                                                  ncos::drivers::storage::RuntimeConfigStore::primary_slot_key(),
                                                  garbage,
                                                  sizeof(garbage))));

  ncos::drivers::storage::RuntimeConfigStore after_reboot(&persistence);
  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  const auto load_result = after_reboot.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(ncos::drivers::storage::RuntimeConfigRecoveryPath::kRecoveredLastKnownGood),
      static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_TRUE(load_result.repaired_storage);
  TEST_ASSERT_TRUE(loaded.cloud_sync_enabled);
  TEST_ASSERT_TRUE(loaded.telemetry_enabled);
  TEST_ASSERT_EQUAL_UINT32(17000U, loaded.telemetry_interval_ms);
}

void test_runtime_config_store_export_import_roundtrips_after_reset() {
  ncos::drivers::storage::RuntimeConfigStore store;
  auto runtime_config = ncos::config::make_runtime_config();
  runtime_config.diagnostics_enabled = false;
  runtime_config.cloud_sync_enabled = true;
  runtime_config.cloud_bridge_enabled = true;
  runtime_config.cloud_extension_enabled = true;
  runtime_config.telemetry_enabled = true;
  runtime_config.telemetry_export_off_device = true;
  runtime_config.cloud_sync_interval_ms = 61000U;
  runtime_config.telemetry_interval_ms = 700U;

  const auto exported = ncos::drivers::storage::RuntimeConfigStore::capture_runtime_config(runtime_config);
  TEST_ASSERT_TRUE(ncos::drivers::storage::RuntimeConfigStore::is_exportable_record(exported));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(exported)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.reset()));

  auto imported_runtime = ncos::config::make_runtime_config();
  TEST_ASSERT_TRUE(
      ncos::drivers::storage::RuntimeConfigStore::apply_import_record_to_runtime_config(exported, &imported_runtime));
  TEST_ASSERT_FALSE(imported_runtime.diagnostics_enabled);
  TEST_ASSERT_TRUE(imported_runtime.cloud_sync_enabled);
  TEST_ASSERT_TRUE(imported_runtime.cloud_bridge_enabled);
  TEST_ASSERT_TRUE(imported_runtime.cloud_extension_enabled);
  TEST_ASSERT_TRUE(imported_runtime.telemetry_enabled);
  TEST_ASSERT_TRUE(imported_runtime.telemetry_export_off_device);
  TEST_ASSERT_EQUAL_UINT32(60000U, imported_runtime.cloud_sync_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(1000U, imported_runtime.telemetry_interval_ms);

  const auto imported_record = ncos::drivers::storage::RuntimeConfigStore::capture_runtime_config(imported_runtime);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.save(imported_record)));

  ncos::core::contracts::PersistedRuntimeConfigRecord loaded{};
  const auto load_result = store.load_with_recovery(&loaded);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(load_result.status));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::RuntimeConfigRecoveryPath::kDirectLoad),
                          static_cast<uint8_t>(load_result.recovery_path));
  TEST_ASSERT_FALSE(loaded.diagnostics_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_sync_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_bridge_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_extension_enabled);
  TEST_ASSERT_TRUE(loaded.telemetry_enabled);
  TEST_ASSERT_TRUE(loaded.telemetry_export_off_device);
  TEST_ASSERT_EQUAL_UINT32(60000U, loaded.cloud_sync_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(1000U, loaded.telemetry_interval_ms);
}
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_storage_policy_limits_persistence_to_runtime_config_baseline);
  RUN_TEST(test_runtime_config_envelope_is_versioned_and_checksum_protected);
  RUN_TEST(test_runtime_config_store_roundtrips_sanitized_record);
  RUN_TEST(test_runtime_config_store_survives_reboot_with_latest_committed_snapshot);
  RUN_TEST(test_runtime_config_store_survives_interrupted_write_across_reboot);
  RUN_TEST(test_runtime_config_store_recovers_last_known_good_and_repairs_storage);
  RUN_TEST(test_runtime_config_store_loads_legacy_single_slot_record_and_migrates);
  RUN_TEST(test_runtime_config_store_resets_profile_to_default_when_no_valid_snapshot_remains);
  RUN_TEST(test_runtime_config_store_reset_profile_writes_default_record);
  RUN_TEST(test_runtime_config_store_resets_all_slots_to_not_found);
  RUN_TEST(test_runtime_config_store_applies_persisted_subset_over_runtime_config);
  RUN_TEST(test_runtime_config_store_export_import_policy_stays_manual_and_sanitized);
  RUN_TEST(test_runtime_config_store_export_import_roundtrips_after_reset);
  RUN_TEST(test_runtime_config_store_rejects_import_with_unknown_schema);
  return UNITY_END();
}



