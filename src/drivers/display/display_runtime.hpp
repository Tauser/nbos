#pragma once

#include "drivers/display/display_driver.hpp"

namespace ncos::drivers::display {

DisplayDriver* acquire_shared_display();

}  // namespace ncos::drivers::display
