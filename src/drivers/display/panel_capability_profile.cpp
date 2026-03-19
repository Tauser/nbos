#include "drivers/display/panel_capability_profile.hpp"

namespace {

constexpr ncos::drivers::display::DisplayPanelCapabilityProfile St7789CapabilityProfile = {
    ncos::drivers::display::DisplayPanelId::kSt7789_240x320,
    "st7789_240x320_freenove_variant",
    240,
    320,
    false,
    true,
    {
        40000000,
        16000000,
        90,
        90,
        120,
    },
    {
        true,
        true,
        true,
        true,
    },
    {
        true,
        true,
        true,
        true,
        true,
    },
};

}  // namespace

namespace ncos::drivers::display {

const DisplayPanelCapabilityProfile& active_panel_capability_profile() {
  return St7789CapabilityProfile;
}

bool flush_path_recommended(const DisplayPanelCapabilityProfile& profile,
                            DisplayFlushPath path,
                            bool high_contrast_motion) {
  switch (path) {
    case DisplayFlushPath::kDirectPrimitives:
      if (high_contrast_motion && profile.workarounds.avoid_large_high_contrast_full_redraw_when_possible) {
        return true;
      }
      return profile.workarounds.prefer_direct_primitives;

    case DisplayFlushPath::kRegionalComposite:
      if (!profile.workarounds.allow_dirty_rect_for_small_eye_motion) {
        return false;
      }
      return profile.observed.dirty_rect_motion_artifacts;

    case DisplayFlushPath::kSpriteWindow:
      if (profile.workarounds.avoid_sprite_window_in_product_path) {
        return false;
      }
      return true;

    default:
      return false;
  }
}

}  // namespace ncos::drivers::display
