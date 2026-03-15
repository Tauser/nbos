#pragma once

#include "interfaces/cloud/cloud_sync_port.hpp"

namespace ncos::drivers::cloud {

class CloudLocalPort final : public ncos::interfaces::cloud::CloudSyncPort {
 public:
  bool ensure_ready() override;
  bool send_packet(const ncos::core::contracts::CloudSyncPacket& packet) override;
};

ncos::interfaces::cloud::CloudSyncPort* acquire_shared_cloud_port();

}  // namespace ncos::drivers::cloud
