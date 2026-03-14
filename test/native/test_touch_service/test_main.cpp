#include <unity.h>

#include "services/sensing/touch_service.hpp"

// Native tests run with test_build_src = no.
#include "services/sensing/touch_service.cpp"

namespace {

class FakeTouchPort final : public ncos::interfaces::sensing::TouchPort {
 public:
  bool ensure_ready() override { return ready; }
  bool calibrate_idle() override {
    calibrated = true;
    return calibrate_ok;
  }
  bool read_raw(uint32_t* out_raw) override {
    ++reads;
    if (!read_ok) {
      return false;
    }
    if (out_raw != nullptr) {
      *out_raw = raw_value;
    }
    return true;
  }
  uint32_t baseline_raw() const override { return baseline; }
  uint32_t trigger_delta() const override { return delta; }

  bool ready = true;
  bool calibrate_ok = true;
  bool read_ok = true;
  bool calibrated = false;
  uint32_t baseline = 3200000;
  uint32_t delta = 800;
  uint32_t raw_value = 3199200;
  uint32_t reads = 0;
};

}  // namespace

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_touch_service_normalizes_level() {
  FakeTouchPort fake{};

  ncos::services::sensing::TouchService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(100));

  service.tick(100);
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.initialized);
  TEST_ASSERT_TRUE(state.last_read_ok);
  TEST_ASSERT_EQUAL_UINT32(3199200, state.last_raw);
  TEST_ASSERT_EQUAL_UINT16(1000, state.normalized_level);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_touch_service_normalizes_level);
  return UNITY_END();
}
