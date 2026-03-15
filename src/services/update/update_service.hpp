#pragma once

#include <stdint.h>

#include "config/system_config.hpp"
#include "core/contracts/update_runtime_contracts.hpp"
#include "core/runtime/system_manager.hpp"
#include "interfaces/update/update_port.hpp"

namespace ncos::services::update {

class UpdateService final {
 public:
  void bind_port(ncos::interfaces::update::UpdatePort* port);
  bool initialize(uint16_t service_id, uint64_t now_ms, const ncos::config::RuntimeConfig& config);

  ncos::core::contracts::UpdateDecision evaluate_boot_policy(uint64_t now_ms);
  ncos::core::contracts::UpdateDecision tick(uint64_t now_ms,
                                             const ncos::core::runtime::RuntimeStatus& runtime_status);

  [[nodiscard]] const ncos::core::contracts::UpdateRuntimeState& state() const;

 private:
  ncos::core::contracts::UpdateDecision make_blocked_decision(const char* reason,
                                                              bool fallback,
                                                              bool rollback,
                                                              uint64_t now_ms);

  uint16_t service_id_ = 0;
  const ncos::config::RuntimeConfig* config_ = nullptr;
  ncos::interfaces::update::UpdatePort* port_ = nullptr;
  ncos::interfaces::update::OtaBootInfo boot_info_{};
  ncos::core::contracts::UpdateRuntimeState state_ = ncos::core::contracts::make_update_runtime_baseline();
};

}  // namespace ncos::services::update
