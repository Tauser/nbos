#include <unity.h>

#include "config/system_config.hpp"
#include "core/contracts/cloud_sync_runtime_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "interfaces/cloud/cloud_sync_port.hpp"
#include "services/cloud/cloud_sync_service.hpp"

// Native tests run with test_build_src = no.
#include "services/cloud/cloud_sync_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

class FakeCloudPort final : public ncos::interfaces::cloud::CloudSyncPort {
 public:
  bool ensure_ready() override {
    ++ensure_calls;
    return ready;
  }

  bool send_packet(const ncos::core::contracts::CloudSyncPacket& packet) override {
    ++send_calls;
    last_packet = packet;
    if (next_result_count == 0) {
      return default_send_result;
    }

    const bool result = scripted_results[next_result_index];
    next_result_index = (next_result_index + 1U) % kMaxScript;
    --next_result_count;
    return result;
  }

  void script_results(const bool* values, uint8_t count) {
    next_result_index = 0;
    next_result_count = count > kMaxScript ? kMaxScript : count;
    for (uint8_t i = 0; i < next_result_count; ++i) {
      scripted_results[i] = values[i];
    }
  }

  static constexpr uint8_t kMaxScript = 8;
  bool ready = true;
  bool default_send_result = true;
  uint32_t ensure_calls = 0;
  uint32_t send_calls = 0;
  uint8_t next_result_index = 0;
  uint8_t next_result_count = 0;
  bool scripted_results[kMaxScript] = {};
  ncos::core::contracts::CloudSyncPacket last_packet{};
};

ncos::config::RuntimeConfig make_cloud_config(bool enabled) {
  ncos::config::RuntimeConfig cfg = ncos::config::kGlobalConfig.runtime;
  cfg.cloud_sync_enabled = enabled;
  cfg.cloud_sync_interval_ms = 1000;
  cfg.cloud_sync_retry_backoff_ms = 4000;
  cfg.cloud_sync_failure_threshold = 3;
  return cfg;
}

ncos::core::contracts::CompanionSnapshot make_snapshot_for_sync() {
  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.revision = 42;
  snapshot.captured_at_ms = 9000;

  snapshot.runtime.initialized = true;
  snapshot.runtime.started = true;
  snapshot.runtime.scheduler_tasks = 2;

  snapshot.governance.allowed_total = 8;
  snapshot.governance.preempted_total = 1;
  snapshot.governance.rejected_total = 0;

  snapshot.energetic.mode = ncos::core::contracts::EnergyMode::kNominal;
  snapshot.energetic.battery_percent = 78;
  snapshot.energetic.thermal_load_percent = 32;

  snapshot.attentional.target = ncos::core::contracts::AttentionTarget::kUser;
  snapshot.attentional.channel = ncos::core::contracts::AttentionChannel::kAuditory;
  snapshot.attentional.focus_confidence_percent = 75;
  snapshot.attentional.lock_active = true;

  snapshot.interactional.phase = ncos::core::contracts::InteractionPhase::kListening;
  snapshot.interactional.session_active = true;
  snapshot.interactional.response_pending = true;

  snapshot.emotional.intensity_percent = 60;
  snapshot.emotional.stability_percent = 82;
  snapshot.emotional.tone = ncos::core::contracts::EmotionalTone::kCurious;
  return snapshot;
}

}  // namespace

void test_cloud_sync_service_sends_selective_packet_without_breaking_local_flow() {
  FakeCloudPort port{};
  ncos::services::cloud::CloudSyncService service;
  service.bind_port(&port);

  const auto cfg = make_cloud_config(true);
  TEST_ASSERT_TRUE(service.initialize(1, 1000, cfg));

  auto snapshot = make_snapshot_for_sync();
  TEST_ASSERT_TRUE(service.tick(snapshot, 1500));

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.connected);
  TEST_ASSERT_FALSE(state.offline_fallback_active);
  TEST_ASSERT_EQUAL_UINT32(1, state.synced_total);

  TEST_ASSERT_TRUE(port.last_packet.include_runtime);
  TEST_ASSERT_TRUE(port.last_packet.include_governance);
  TEST_ASSERT_TRUE(port.last_packet.include_energetic);
  TEST_ASSERT_TRUE(port.last_packet.include_attentional);
  TEST_ASSERT_TRUE(port.last_packet.include_interactional);
  TEST_ASSERT_TRUE(port.last_packet.include_emotional);
}

void test_cloud_sync_service_enters_safe_offline_fallback_after_repeated_failures_and_recovers() {
  FakeCloudPort port{};
  const bool script[] = {false, false, false, true};
  port.script_results(script, 4);

  ncos::services::cloud::CloudSyncService service;
  service.bind_port(&port);

  const auto cfg = make_cloud_config(true);
  TEST_ASSERT_TRUE(service.initialize(1, 1000, cfg));

  const auto snapshot = make_snapshot_for_sync();

  TEST_ASSERT_FALSE(service.tick(snapshot, 1000));
  TEST_ASSERT_FALSE(service.tick(snapshot, 5000));
  TEST_ASSERT_FALSE(service.tick(snapshot, 9000));

  const auto& degraded = service.state();
  TEST_ASSERT_TRUE(degraded.degraded);
  TEST_ASSERT_TRUE(degraded.offline_fallback_active);
  TEST_ASSERT_EQUAL_UINT8(3, degraded.consecutive_failures);

  TEST_ASSERT_TRUE(service.tick(snapshot, 13000));

  const auto& recovered = service.state();
  TEST_ASSERT_FALSE(recovered.degraded);
  TEST_ASSERT_FALSE(recovered.offline_fallback_active);
  TEST_ASSERT_EQUAL_UINT8(0, recovered.consecutive_failures);
  TEST_ASSERT_EQUAL_UINT32(1, recovered.synced_total);
}

void test_cloud_sync_service_keeps_sync_disabled_when_policy_is_offline_only() {
  FakeCloudPort port{};
  ncos::services::cloud::CloudSyncService service;
  service.bind_port(&port);

  const auto cfg = make_cloud_config(false);
  TEST_ASSERT_TRUE(service.initialize(1, 1000, cfg));

  const auto snapshot = make_snapshot_for_sync();
  TEST_ASSERT_FALSE(service.tick(snapshot, 2000));

  const auto& state = service.state();
  TEST_ASSERT_FALSE(state.sync_enabled);
  TEST_ASSERT_EQUAL_UINT32(0, state.synced_total);
  TEST_ASSERT_EQUAL_UINT32(0, state.failed_total);
  TEST_ASSERT_EQUAL_UINT32(0, port.send_calls);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_cloud_sync_service_sends_selective_packet_without_breaking_local_flow);
  RUN_TEST(test_cloud_sync_service_enters_safe_offline_fallback_after_repeated_failures_and_recovers);
  RUN_TEST(test_cloud_sync_service_keeps_sync_disabled_when_policy_is_offline_only);
  return UNITY_END();
}
