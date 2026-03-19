#include <unity.h>

#include "config/system_config.hpp"
#include "core/contracts/storage_runtime_contracts.hpp"
#include "drivers/storage/runtime_config_store.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/storage_runtime_contracts.cpp"
#include "drivers/storage/storage_platform_bsp.cpp"
#include "drivers/storage/local_persistence.cpp"
#include "drivers/storage/runtime_config_store.cpp"

void setUp() {}
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
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::interfaces::state::RuntimeConfigPersistenceStatus::kOk),
                          static_cast<uint8_t>(store.load(&loaded)));
  TEST_ASSERT_FALSE(loaded.diagnostics_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_sync_enabled);
  TEST_ASSERT_TRUE(loaded.cloud_bridge_enabled);
  TEST_ASSERT_TRUE(loaded.telemetry_enabled);
  TEST_ASSERT_EQUAL_UINT32(60000U, loaded.cloud_sync_interval_ms);
  TEST_ASSERT_EQUAL_UINT32(1000U, loaded.telemetry_interval_ms);
}

void test_runtime_config_store_resets_to_not_found() {
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

  TEST_ASSERT_FALSE(
      ncos::drivers::storage::RuntimeConfigStore::is_exportable_record(record));
  TEST_ASSERT_FALSE(
      ncos::drivers::storage::RuntimeConfigStore::apply_import_record_to_runtime_config(record, &runtime_config));
  TEST_ASSERT_FALSE(runtime_config.cloud_sync_enabled);
  TEST_ASSERT_FALSE(runtime_config.telemetry_enabled);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_storage_policy_limits_persistence_to_runtime_config_baseline);
  RUN_TEST(test_runtime_config_store_roundtrips_sanitized_record);
  RUN_TEST(test_runtime_config_store_resets_to_not_found);
  RUN_TEST(test_runtime_config_store_applies_persisted_subset_over_runtime_config);
  RUN_TEST(test_runtime_config_store_export_import_policy_stays_manual_and_sanitized);
  RUN_TEST(test_runtime_config_store_rejects_import_with_unknown_schema);
  return UNITY_END();
}
