#pragma once

#include "config/build_profile.hpp"
#include "config/pins/board_freenove_esp32s3_cam.hpp"

namespace ncos::config {

inline constexpr bool kConfigReady = is_build_profile_valid();

}  // namespace ncos::config
