#include <unity.h>

#include "drivers/display/panel_capability_profile.hpp"
#include "drivers/display/panel_capability_profile.cpp"

void setUp() {}
void tearDown() {}

void test_display_panel_profile_captures_real_panel_quirks() {
  const auto& profile = ncos::drivers::display::active_panel_capability_profile();

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::drivers::display::DisplayPanelId::kSt7789_240x320),
                        static_cast<int>(profile.panel_id));
  TEST_ASSERT_EQUAL_UINT16(240, profile.width);
  TEST_ASSERT_EQUAL_UINT16(320, profile.height);
  TEST_ASSERT_TRUE(profile.reset_shared_with_enable);
  TEST_ASSERT_TRUE(profile.observed.panel_polarity_flip_clean);
  TEST_ASSERT_TRUE(profile.observed.full_redraw_motion_artifacts);
  TEST_ASSERT_TRUE(profile.observed.dirty_rect_motion_artifacts);
  TEST_ASSERT_TRUE(profile.observed.sprite_window_flicker);
  TEST_ASSERT_TRUE(profile.workarounds.prefer_direct_primitives);
  TEST_ASSERT_TRUE(profile.workarounds.avoid_sprite_window_in_product_path);
}

void test_display_panel_profile_recommends_regional_composite_and_rejects_sprite_window() {
  const auto& profile = ncos::drivers::display::active_panel_capability_profile();

  TEST_ASSERT_TRUE(ncos::drivers::display::flush_path_recommended(
      profile, ncos::drivers::display::DisplayFlushPath::kRegionalComposite, true));
  TEST_ASSERT_FALSE(ncos::drivers::display::flush_path_recommended(
      profile, ncos::drivers::display::DisplayFlushPath::kSpriteWindow, true));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_display_panel_profile_captures_real_panel_quirks);
  RUN_TEST(test_display_panel_profile_recommends_regional_composite_and_rejects_sprite_window);
  return UNITY_END();
}
