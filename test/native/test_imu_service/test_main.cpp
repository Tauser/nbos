#include <unity.h>

#include "services/sensing/imu_service.hpp"

// Native tests run with test_build_src = no.
#include "services/sensing/imu_service.cpp"

namespace {

class FakeImuPort final : public ncos::interfaces::sensing::ImuPort {
 public:
  bool ensure_ready() override { return ready; }
  bool read_sample(ncos::interfaces::sensing::ImuSampleRaw* out_sample) override {
    ++reads;
    if (!read_ok || out_sample == nullptr) {
      return false;
    }
    *out_sample = sample;
    return true;
  }

  bool ready = true;
  bool read_ok = true;
  uint32_t reads = 0;
  ncos::interfaces::sensing::ImuSampleRaw sample{.ax = 16384, .ay = -8192, .az = 0, .gx = 131, .gy = -262, .gz = 0};
};

}  // namespace

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_imu_service_normalizes_units() {
  FakeImuPort fake{};

  ncos::services::sensing::ImuService service;
  service.bind_port(&fake);
  TEST_ASSERT_TRUE(service.initialize(100));

  service.tick(100);
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.initialized);
  TEST_ASSERT_TRUE(state.last_read_ok);
  TEST_ASSERT_EQUAL_INT16(1000, state.ax_mg);
  TEST_ASSERT_EQUAL_INT16(-500, state.ay_mg);
  TEST_ASSERT_EQUAL_INT16(1, state.gx_dps);
  TEST_ASSERT_EQUAL_INT16(-2, state.gy_dps);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_imu_service_normalizes_units);
  return UNITY_END();
}
