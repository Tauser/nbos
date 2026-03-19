#include <unity.h>

#include "drivers/power/power_platform_bsp.hpp"

// Native tests run with test_build_src = no.
#include "drivers/power/power_platform_bsp.cpp"

void setUp() {}
void tearDown() {}

void test_power_platform_bsp_centralizes_local_fallback_profile() {
  const auto& bsp = ncos::drivers::power::active_power_platform_bsp();

  TEST_ASSERT_EQUAL_STRING("freenove_esp32s3_wroom_cam_n16r8", bsp.board_name);
  TEST_ASSERT_FALSE(bsp.sensing.battery_sensor_present);
  TEST_ASSERT_FALSE(bsp.sensing.external_power_sensor_present);
  TEST_ASSERT_FALSE(bsp.sensing.thermal_sensor_present);
  TEST_ASSERT_FALSE(bsp.fallback.measured_from_sensor);
  TEST_ASSERT_EQUAL_UINT16(3920U, bsp.fallback.baseline_battery_mv);
  TEST_ASSERT_EQUAL_UINT16(14U, bsp.fallback.ripple_drop_mv);
  TEST_ASSERT_EQUAL_UINT16(90U, bsp.fallback.ripple_period_seconds);
  TEST_ASSERT_EQUAL_UINT8(30U, bsp.fallback.baseline_thermal_load_percent);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_power_platform_bsp_centralizes_local_fallback_profile);
  return UNITY_END();
}
