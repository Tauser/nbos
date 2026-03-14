#pragma once

#include "app/lifecycle/system_lifecycle.hpp"
#include "core/runtime/system_manager.hpp"

namespace ncos::app::boot {

class FirmwareEntrypoint final {
 public:
  FirmwareEntrypoint() = default;

  void run();
  void tick();
  [[nodiscard]] const ncos::app::lifecycle::SystemLifecycle& lifecycle() const;

 private:
  ncos::app::lifecycle::SystemLifecycle lifecycle_{};
  ncos::core::runtime::SystemManager system_manager_{};
};

}  // namespace ncos::app::boot
