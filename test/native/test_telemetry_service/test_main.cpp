#include <unity.h>

#include "config/system_config.hpp"
#include "core/contracts/cloud_bridge_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "core/contracts/telemetry_runtime_contracts.hpp"
#include "interfaces/telemetry/telemetry_port.hpp"
#include "services/telemetry/telemetry_service.hpp"

// Native tests run with test_build_src = no.
#include "services/telemetry/telemetry_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

class FakeTelemetryPort final : public ncos::interfaces::telemetry::TelemetryPort {
 public:
  bool ensure_ready() override {
    ++ensure_calls;
    return ready;
  }

  bool publish_sample(const ncos::core::contracts::TelemetrySample& sample) override {
    ++publish_calls;
    last_sample = sample;
    return publish_ok;
  }

  bool ready = true;
  bool publish_ok = true;
  uint32_t ensure_calls = 0;
  uint32_t publish_calls = 0;
  ncos::core::contracts::TelemetrySample last_sample{};
};

ncos::config::RuntimeConfig make_config(bool enabled, bool export_off_device) {
  ncos::config::RuntimeConfig cfg = ncos::config::kGlobalConfig.runtime;
  cfg.telemetry_enabled = enabled;
  cfg.telemetry_export_off_device = export_off_device;
  cfg.telemetry_interval_ms = 1000;
  cfg.telemetry_collect_interactional = false;
  cfg.telemetry_collect_emotional = false;
  cfg.telemetry_collect_transient = false;
  return cfg;
}

ncos::core::contracts::TelemetryRuntimeInput make_runtime_input() {
  ncos::core::contracts::TelemetryRuntimeInput input{};
  input.initialized = true;
  input.started = true;
  input.safe_mode = false;
  input.scheduler_tasks = 3;
  input.fault_count = 0;
  input.bus_published_total = 22;
  input.bus_dispatched_total = 21;
  input.bus_dropped_total = 1;
  input.governance_allowed_total = 9;
  input.governance_preempted_total = 2;
  input.governance_rejected_total = 1;
  input.companion_state_revision = 44;
  return input;
}

ncos::core::contracts::CompanionSnapshot make_companion_snapshot() {
  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.energetic.mode = ncos::core::contracts::EnergyMode::kConstrained;
  snapshot.energetic.battery_percent = 37;
  snapshot.interactional.phase = ncos::core::contracts::InteractionPhase::kListening;
  snapshot.interactional.session_active = true;
  snapshot.emotional.tone = ncos::core::contracts::EmotionalTone::kAffiliative;
  snapshot.emotional.intensity_percent = 74;
  snapshot.transient.has_active_trace = true;
  snapshot.transient.active_trace_id = 777;
  return snapshot;
}

ncos::core::contracts::CloudBridgeRuntimeState make_cloud_state() {
  ncos::core::contracts::CloudBridgeRuntimeState cloud{};
  cloud.connected = true;
  cloud.degraded = false;
  cloud.offline_authoritative = true;
  return cloud;
}

}  // namespace

void test_telemetry_service_keeps_collection_disabled_by_default_policy() {
  FakeTelemetryPort port{};
  ncos::services::telemetry::TelemetryService service;
  service.bind_port(&port);

  const auto cfg = make_config(false, false);
  TEST_ASSERT_TRUE(service.initialize(69, 1000, cfg));

  TEST_ASSERT_FALSE(service.tick(make_runtime_input(), make_companion_snapshot(), make_cloud_state(), 1100));
  TEST_ASSERT_EQUAL_UINT32(0, port.publish_calls);
  TEST_ASSERT_EQUAL_STRING("telemetry_disabled", service.state().last_reason);
}

void test_telemetry_service_redacts_sensitive_domains_by_default_surface() {
  FakeTelemetryPort port{};
  ncos::services::telemetry::TelemetryService service;
  service.bind_port(&port);

  const auto cfg = make_config(true, true);
  TEST_ASSERT_TRUE(service.initialize(69, 1000, cfg));

  TEST_ASSERT_TRUE(service.tick(make_runtime_input(), make_companion_snapshot(), make_cloud_state(), 1200));

  TEST_ASSERT_EQUAL_UINT32(1, port.publish_calls);
  TEST_ASSERT_TRUE(port.last_sample.include_energetic);
  TEST_ASSERT_TRUE(port.last_sample.include_cloud);
  TEST_ASSERT_FALSE(port.last_sample.include_interactional);
  TEST_ASSERT_FALSE(port.last_sample.include_emotional);
  TEST_ASSERT_FALSE(port.last_sample.include_transient);
}

void test_telemetry_service_respects_rate_limit_window() {
  FakeTelemetryPort port{};
  ncos::services::telemetry::TelemetryService service;
  service.bind_port(&port);

  const auto cfg = make_config(true, true);
  TEST_ASSERT_TRUE(service.initialize(69, 1000, cfg));

  TEST_ASSERT_TRUE(service.tick(make_runtime_input(), make_companion_snapshot(), make_cloud_state(), 1000));
  TEST_ASSERT_FALSE(service.tick(make_runtime_input(), make_companion_snapshot(), make_cloud_state(), 1500));
  TEST_ASSERT_TRUE(service.tick(make_runtime_input(), make_companion_snapshot(), make_cloud_state(), 2000));

  TEST_ASSERT_EQUAL_UINT32(2, port.publish_calls);
  TEST_ASSERT_EQUAL_STRING("telemetry_publish_ok", service.state().last_reason);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_telemetry_service_keeps_collection_disabled_by_default_policy);
  RUN_TEST(test_telemetry_service_redacts_sensitive_domains_by_default_surface);
  RUN_TEST(test_telemetry_service_respects_rate_limit_window);
  return UNITY_END();
}
