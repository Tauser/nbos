#include <unity.h>

#include "config/build_profile.hpp"
#include "config/pins/board_freenove_esp32s3_cam.hpp"

void test_build_profile_is_known() {
  TEST_ASSERT_TRUE(ncos::config::is_build_profile_valid());
  TEST_ASSERT_EQUAL_STRING("dev", ncos::config::build_profile_name());
}

void test_board_profile_pins_match_baseline() {
  TEST_ASSERT_EQUAL_INT(21, ncos::config::pins::kDisplayMosi);
  TEST_ASSERT_EQUAL_INT(47, ncos::config::pins::kDisplaySck);
  TEST_ASSERT_EQUAL_INT(45, ncos::config::pins::kDisplayDc);
  TEST_ASSERT_EQUAL_INT(20, ncos::config::pins::kDisplayRst);
  TEST_ASSERT_EQUAL_INT(0, ncos::config::pins::kImuSda);
  TEST_ASSERT_EQUAL_INT(19, ncos::config::pins::kImuScl);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_build_profile_is_known);
  RUN_TEST(test_board_profile_pins_match_baseline);
  return UNITY_END();
}
