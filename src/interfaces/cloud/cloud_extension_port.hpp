#pragma once

#include "core/contracts/cloud_bridge_contracts.hpp"

namespace ncos::interfaces::cloud {

class CloudExtensionPort {
 public:
  virtual ~CloudExtensionPort() = default;

  virtual bool ensure_ready() = 0;
  virtual bool submit_extension(const ncos::core::contracts::CloudExtensionRequest& request,
                                ncos::core::contracts::CloudExtensionResponse* out_response) = 0;
};

}  // namespace ncos::interfaces::cloud
