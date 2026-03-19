#include <unity.h>

#include "drivers/storage/storage_platform_bsp.hpp"

// Native tests run with test_build_src = no.
#include "drivers/storage/storage_platform_bsp.cpp"

void setUp() {}
void tearDown() {}

void test_storage_platform_bsp_exposes_local_persistence_defaults() {
  const auto& bsp = ncos::drivers::storage::active_storage_platform_bsp();

  TEST_ASSERT_EQUAL_STRING("freenove_esp32s3_wroom_cam_n16r8", bsp.board_name);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::drivers::storage::StorageBackendId::kNvs),
                          static_cast<uint8_t>(bsp.config_backend));
  TEST_ASSERT_EQUAL_STRING("ncos", bsp.config_namespace);
  TEST_ASSERT_EQUAL_STRING("runtime_cfg", bsp.runtime_config_key);
  TEST_ASSERT_EQUAL_UINT32(128U, static_cast<uint32_t>(bsp.persistence.max_record_bytes));
  TEST_ASSERT_TRUE(bsp.persistence.erase_corrupt_records);
  TEST_ASSERT_TRUE(bsp.persistence.allow_runtime_config_persistence);
  TEST_ASSERT_TRUE(bsp.persistence.allow_companion_memory_persistence);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_storage_platform_bsp_exposes_local_persistence_defaults);
  return UNITY_END();
}
