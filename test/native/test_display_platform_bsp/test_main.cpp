#include <unity.h>

#include "drivers/display/display_platform_bsp.hpp"

// Native tests run with test_build_src = no.
#include "drivers/display/panel_capability_profile.cpp"
#include "drivers/display/display_platform_bsp.cpp"

void setUp() {}
void tearDown() {}

void test_display_platform_bsp_centralizes_wiring_and_panel_flags() {
  const auto& bsp = ncos::drivers::display::active_display_platform_bsp();

  TEST_ASSERT_NOT_NULL(bsp.profile);
  TEST_ASSERT_EQUAL_STRING("freenove_esp32s3_wroom_cam_n16r8", bsp.board_name);
  TEST_ASSERT_EQUAL_INT(21, bsp.bus_wiring.mosi);
  TEST_ASSERT_EQUAL_INT(47, bsp.bus_wiring.sclk);
  TEST_ASSERT_EQUAL_INT(45, bsp.bus_wiring.dc);
  TEST_ASSERT_EQUAL_INT(-1, bsp.panel_wiring.cs);
  TEST_ASSERT_TRUE(bsp.panel_flags.keep_panel_inverted);
  TEST_ASSERT_TRUE(bsp.panel_flags.reset_shared_with_enable);
  TEST_ASSERT_EQUAL_UINT8(8, bsp.panel_flags.dummy_read_pixel);
  TEST_ASSERT_EQUAL_UINT8(1, bsp.panel_flags.dummy_read_bits);
}

void test_display_platform_bsp_stays_aligned_with_active_panel_profile() {
  const auto& bsp = ncos::drivers::display::active_display_platform_bsp();
  const auto& profile = ncos::drivers::display::active_panel_capability_profile();

  TEST_ASSERT_EQUAL_PTR(&profile, bsp.profile);
  TEST_ASSERT_EQUAL_UINT16(profile.width, 240);
  TEST_ASSERT_EQUAL_UINT32(profile.timing.spi_write_hz, 40000000UL);
  TEST_ASSERT_EQUAL_UINT32(profile.timing.spi_read_hz, 16000000UL);
  TEST_ASSERT_EQUAL_UINT8(profile.readable ? 1 : 0, bsp.panel_flags.readable ? 1 : 0);
  TEST_ASSERT_EQUAL_UINT8(profile.workarounds.keep_panel_inverted ? 1 : 0,
                          bsp.panel_flags.keep_panel_inverted ? 1 : 0);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_display_platform_bsp_centralizes_wiring_and_panel_flags);
  RUN_TEST(test_display_platform_bsp_stays_aligned_with_active_panel_profile);
  return UNITY_END();
}
