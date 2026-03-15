#pragma once

#include "core/contracts/update_runtime_contracts.hpp"

namespace ncos::interfaces::update {

struct OtaBootInfo {
  bool component_available = false;
  bool rollback_supported = false;
  bool pending_verify = false;
  ncos::core::contracts::OtaRunningSlot running_slot =
      ncos::core::contracts::OtaRunningSlot::kUnknown;
};

class UpdatePort {
 public:
  virtual ~UpdatePort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool read_boot_info(OtaBootInfo* out_info) = 0;
  virtual bool confirm_running_image() = 0;
  virtual bool request_rollback() = 0;
};

}  // namespace ncos::interfaces::update
