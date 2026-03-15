#pragma once

#include "interfaces/update/update_port.hpp"

namespace ncos::drivers::update {

class UpdateLocalPort final : public ncos::interfaces::update::UpdatePort {
 public:
  bool ensure_ready() override;
  bool read_boot_info(ncos::interfaces::update::OtaBootInfo* out_info) override;
  bool confirm_running_image() override;
  bool request_rollback() override;
};

ncos::interfaces::update::UpdatePort* acquire_shared_update_port();

}  // namespace ncos::drivers::update
