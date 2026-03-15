#pragma once

#include "interfaces/cloud/cloud_extension_port.hpp"

namespace ncos::drivers::cloud {

class CloudLocalExtensionPort final : public ncos::interfaces::cloud::CloudExtensionPort {
 public:
  bool ensure_ready() override;
  bool submit_extension(const ncos::core::contracts::CloudExtensionRequest& request,
                        ncos::core::contracts::CloudExtensionResponse* out_response) override;
};

ncos::interfaces::cloud::CloudExtensionPort* acquire_shared_cloud_extension_port();

}  // namespace ncos::drivers::cloud
