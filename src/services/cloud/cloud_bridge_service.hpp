#pragma once

#include <stdint.h>

#include "config/system_config.hpp"
#include "core/contracts/cloud_bridge_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "interfaces/cloud/cloud_extension_port.hpp"
#include "interfaces/cloud/cloud_sync_port.hpp"
#include "services/cloud/cloud_sync_service.hpp"

namespace ncos::services::cloud {

class CloudBridgeService final {
 public:
  void bind_sync_port(ncos::interfaces::cloud::CloudSyncPort* port);
  void bind_extension_port(ncos::interfaces::cloud::CloudExtensionPort* port);

  bool initialize(uint16_t service_id, uint64_t now_ms, const ncos::config::RuntimeConfig& config);
  bool tick(const ncos::core::contracts::CompanionSnapshot& snapshot, uint64_t now_ms);

  bool submit_extension(const ncos::core::contracts::CloudExtensionRequest& request,
                        uint64_t now_ms,
                        ncos::core::contracts::CloudExtensionResponse* out_response);

  [[nodiscard]] const ncos::core::contracts::CloudBridgeRuntimeState& state() const;

 private:
  void refresh_state_from_sync(uint64_t now_ms);

  uint16_t service_id_ = 0;
  const ncos::config::RuntimeConfig* config_ = nullptr;
  ncos::interfaces::cloud::CloudExtensionPort* extension_port_ = nullptr;
  CloudSyncService sync_service_{};
  ncos::core::contracts::CloudBridgeRuntimeState state_ =
      ncos::core::contracts::make_cloud_bridge_runtime_baseline();
};

}  // namespace ncos::services::cloud
