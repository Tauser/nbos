#include <unity.h>

#include "services/power/power_service.hpp"

// Native tests run with test_build_src = no.
#include "services/power/power_service.cpp"

namespace {

class FakePowerPort final : public ncos::interfaces::power::PowerPort {
 public:
  bool ready = true;
  bool read_ok = true;
  ncos::interfaces::power::PowerSampleRaw sample{};

  bool ensure_ready() override {
    return ready;
  }

  bool read_sample(ncos::interfaces::power::PowerSampleRaw* out_sample) override {
    if (!read_ok || out_sample == nullptr) {
      return false;
    }
    *out_sample = sample;
    return true;
  }
};

}  // namespace

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_power_service_maps_battery_percent_and_constrained_mode() {
  FakePowerPort port;
  port.sample.valid = true;
  port.sample.external_power_detected = false;
  port.sample.battery_mv = 3520;
  port.sample.thermal_load_percent = 40;

  ncos::services::power::PowerService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(66, 1000));

  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionEnergeticSignal energetic{};

  TEST_ASSERT_TRUE(service.tick(companion, 1000, &energetic));
  TEST_ASSERT_TRUE(energetic.battery_percent <= 30);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kConstrained),
                        static_cast<int>(energetic.mode));
}

void test_power_service_prefers_charging_mode_when_external_power_detected() {
  FakePowerPort port;
  port.sample.valid = true;
  port.sample.external_power_detected = true;
  port.sample.battery_mv = 3980;
  port.sample.thermal_load_percent = 25;

  ncos::services::power::PowerService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(66, 2000));

  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionEnergeticSignal energetic{};

  TEST_ASSERT_TRUE(service.tick(companion, 2000, &energetic));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kCharging),
                        static_cast<int>(energetic.mode));
  TEST_ASSERT_TRUE(energetic.external_power);
}

void test_power_service_forces_critical_in_safe_mode() {
  FakePowerPort port;
  port.sample.valid = true;
  port.sample.external_power_detected = false;
  port.sample.battery_mv = 3800;
  port.sample.thermal_load_percent = 30;

  ncos::services::power::PowerService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(66, 3000));

  ncos::core::contracts::CompanionSnapshot companion{};
  companion.runtime.safe_mode = true;

  ncos::core::contracts::CompanionEnergeticSignal energetic{};
  TEST_ASSERT_TRUE(service.tick(companion, 3000, &energetic));

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kCritical),
                        static_cast<int>(energetic.mode));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_power_service_maps_battery_percent_and_constrained_mode);
  RUN_TEST(test_power_service_prefers_charging_mode_when_external_power_detected);
  RUN_TEST(test_power_service_forces_critical_in_safe_mode);
  return UNITY_END();
}
