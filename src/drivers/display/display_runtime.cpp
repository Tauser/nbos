#include "drivers/display/display_runtime.hpp"

namespace {
static ncos::drivers::display::DisplayDriver s_display;
static bool s_display_initialized = false;
}

namespace ncos::drivers::display {

DisplayDriver* acquire_shared_display() {
  if (!s_display_initialized) {
    s_display_initialized = s_display.init();
  }

  if (!s_display_initialized) {
    return nullptr;
  }

  return &s_display;
}

}  // namespace ncos::drivers::display
