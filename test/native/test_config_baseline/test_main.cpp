#include <unity.h>

#include "config/build_profile.hpp"
#include "config/pins/board_freenove_esp32s3_cam.hpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_build_profile_is_known() {
  TEST_ASSERT_TRUE(ncos::config::is_build_profile_valid());
  TEST_ASSERT_EQUAL_STRING("dev", ncos::config::build_profile_name());
}

void test_board_profile_pins_match_baseline() {
  TEST_ASSERT_EQUAL_INT(21, ncos::config::pins::kDisplayMosi);
  TEST_ASSERT_EQUAL_INT(47, ncos::config::pins::kDisplaySck);
  TEST_ASSERT_EQUAL_INT(45, ncos::config::pins::kDisplayDc);
  TEST_ASSERT_EQUAL_INT(20, ncos::config::pins::kDisplayRst);
  TEST_ASSERT_EQUAL_INT(-1, ncos::config::pins::kDisplayCs);
  TEST_ASSERT_EQUAL_INT(0, ncos::config::pins::kImuSda);
  TEST_ASSERT_EQUAL_INT(19, ncos::config::pins::kImuScl);
}

void test_power_rails_match_baseline() {
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 3.7F, ncos::config::power_rails::kBatteryNominalVolts);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 4.2F, ncos::config::power_rails::kBatteryMaxVolts);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 5.0F, ncos::config::power_rails::kMain5vRailVolts);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 3.3F, ncos::config::power_rails::kLogic3v3RailVolts);
}

void test_sensitive_pin_flags_are_set() {
  TEST_ASSERT_TRUE(ncos::config::pin_flags::kGpio0BootSensitive);
  TEST_ASSERT_TRUE(ncos::config::pin_flags::kGpio3UartSensitive);
  TEST_ASSERT_TRUE(ncos::config::pin_flags::kGpio46InputOnly);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_build_profile_is_known);
  RUN_TEST(test_board_profile_pins_match_baseline);
  RUN_TEST(test_power_rails_match_baseline);
  RUN_TEST(test_sensitive_pin_flags_are_set);
  return UNITY_END();
}
