#pragma once

#include <stdint.h>

#include "config/system_config.hpp"
#include "core/contracts/cloud_sync_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "interfaces/cloud/cloud_sync_port.hpp"

namespace ncos::services::cloud {

class CloudSyncService final {
 public:
  void bind_port(ncos::interfaces::cloud::CloudSyncPort* port);
  bool initialize(uint16_t service_id, uint64_t now_ms, const ncos::config::RuntimeConfig& config);

  bool tick(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);

  [[nodiscard]] const ncos::core::contracts::CloudSyncRuntimeState& state() const;

 private:
  static bool should_include_attentional(const ncos::core::contracts::CompanionSnapshot& snapshot);
  static bool should_include_interactional(const ncos::core::contracts::CompanionSnapshot& snapshot);
  static bool should_include_emotional(const ncos::core::contracts::CompanionSnapshot& snapshot);

  static ncos::core::contracts::CloudSyncPacket make_selective_packet(
      const ncos::core::contracts::CompanionSnapshot& snapshot);

  uint16_t service_id_ = 0;
  const ncos::config::RuntimeConfig* config_ = nullptr;
  ncos::interfaces::cloud::CloudSyncPort* port_ = nullptr;
  ncos::core::contracts::CloudSyncRuntimeState state_ =
      ncos::core::contracts::make_cloud_sync_runtime_baseline();
};

}  // namespace ncos::services::cloud
