#pragma once

#include <stdint.h>

#include "core/contracts/led_orchestration_contracts.hpp"
#include "interfaces/led/led_port.hpp"

namespace ncos::services::led {

class LedService final {
 public:
  static constexpr size_t kMaxRequests = 6;

  void bind_port(ncos::interfaces::led::LedPort* port);
  bool initialize(uint64_t now_ms);
  bool submit_request(const ncos::core::contracts::LedRequest& request, uint64_t now_ms);
  void clear_owner(uint16_t owner_service);
  void tick(uint64_t now_ms, uint32_t refresh_interval_ms);
  [[nodiscard]] const ncos::core::contracts::LedRuntimeState& state() const;

 private:
  void expire_requests(uint64_t now_ms);
  bool select_top_request(ncos::core::contracts::LedRequest* out_request) const;
  bool apply_selected(const ncos::core::contracts::LedRequest& request, uint64_t now_ms);

  ncos::interfaces::led::LedPort* port_ = nullptr;
  ncos::core::contracts::LedRuntimeState state_ = ncos::core::contracts::make_led_runtime_baseline();
  ncos::core::contracts::LedRequest requests_[kMaxRequests] = {};
  uint64_t next_refresh_ms_ = 0;
};

}  // namespace ncos::services::led
