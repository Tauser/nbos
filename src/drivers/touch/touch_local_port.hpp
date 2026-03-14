#pragma once

#include "drivers/touch/touch_bringup.hpp"
#include "interfaces/sensing/touch_port.hpp"

namespace ncos::drivers::touch {

class TouchLocalPort final : public ncos::interfaces::sensing::TouchPort {
 public:
  bool ensure_ready() override;
  bool calibrate_idle() override;
  bool read_raw(uint32_t* out_raw) override;
  uint32_t baseline_raw() const override;
  uint32_t trigger_delta() const override;

 private:
  TouchBringup bringup_{};
  bool calibrated_ = false;
  uint32_t baseline_raw_ = 0;
  uint32_t trigger_delta_ = 400;
};

ncos::interfaces::sensing::TouchPort* acquire_shared_touch_port();

}  // namespace ncos::drivers::touch
