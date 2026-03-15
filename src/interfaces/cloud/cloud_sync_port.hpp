#pragma once

#include "core/contracts/cloud_sync_runtime_contracts.hpp"

namespace ncos::interfaces::cloud {

class CloudSyncPort {
 public:
  virtual ~CloudSyncPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool send_packet(const ncos::core::contracts::CloudSyncPacket& packet) = 0;
};

}  // namespace ncos::interfaces::cloud
