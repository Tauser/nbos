#include "drivers/power/power_platform_bsp.hpp"

#include "config/pins/board_pins.hpp"

namespace {

const ncos::drivers::power::PowerPlatformBsp PowerBsp = {
    ncos::config::kBoardName,
    {
        false,
        false,
        false,
    },
    {
        false,
        false,
        3920U,
        14U,
        90U,
        30U,
    },
};

}  // namespace

namespace ncos::drivers::power {

const PowerPlatformBsp& active_power_platform_bsp() {
  return PowerBsp;
}

}  // namespace ncos::drivers::power
