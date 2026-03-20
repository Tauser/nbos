#pragma once

#include "interfaces/update/update_port.hpp"

namespace ncos::hal::platform {

bool read_ota_boot_info(ncos::interfaces::update::OtaBootInfo* out_info);
bool confirm_running_image();
bool request_rollback();

}  // namespace ncos::hal::platform
