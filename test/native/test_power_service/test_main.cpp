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

void test_power_service_trips_thermal_guard_and_constrains_mode() {
  FakePowerPort port;
  port.sample.valid = true;
  port.sample.external_power_detected = false;
  port.sample.battery_mv = 3950;
  port.sample.thermal_load_percent = 80;

  ncos::services::power::PowerService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(66, 4000));

  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionEnergeticSignal energetic{};
  TEST_ASSERT_TRUE(service.tick(companion, 4000, &energetic));

  TEST_ASSERT_TRUE(service.state().thermal_guard_active);
  TEST_ASSERT_TRUE(service.state().thermal_guard_latched);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kConstrained),
                        static_cast<int>(energetic.mode));
}

void test_power_service_trips_electrical_guard_after_repeated_sample_failures() {
  FakePowerPort port;
  port.sample.valid = false;

  ncos::services::power::PowerService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(66, 5000));

  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::CompanionEnergeticSignal energetic{};

  TEST_ASSERT_FALSE(service.tick(companion, 5000, &energetic));
  TEST_ASSERT_FALSE(service.tick(companion, 6800, &energetic));
  TEST_ASSERT_TRUE(service.tick(companion, 8600, &energetic));

  TEST_ASSERT_TRUE(service.state().electrical_guard_active);
  TEST_ASSERT_TRUE(service.state().electrical_guard_latched);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::EnergyMode::kCritical),
                        static_cast<int>(energetic.mode));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_power_service_maps_battery_percent_and_constrained_mode);
  RUN_TEST(test_power_service_prefers_charging_mode_when_external_power_detected);
  RUN_TEST(test_power_service_forces_critical_in_safe_mode);
  RUN_TEST(test_power_service_trips_thermal_guard_and_constrains_mode);
  RUN_TEST(test_power_service_trips_electrical_guard_after_repeated_sample_failures);
  return UNITY_END();
}
