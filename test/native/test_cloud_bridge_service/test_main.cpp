#include <unity.h>

#include "config/system_config.hpp"
#include "core/contracts/cloud_bridge_contracts.hpp"
#include "core/contracts/companion_state_contracts.hpp"
#include "interfaces/cloud/cloud_extension_port.hpp"
#include "interfaces/cloud/cloud_sync_port.hpp"
#include "services/cloud/cloud_bridge_service.hpp"

// Native tests run with test_build_src = no.
#include "services/cloud/cloud_sync_service.cpp"
#include "services/cloud/cloud_bridge_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

class FakeCloudSyncPort final : public ncos::interfaces::cloud::CloudSyncPort {
 public:
  bool ensure_ready() override {
    ++ensure_calls;
    return ready;
  }

  bool send_packet(const ncos::core::contracts::CloudSyncPacket&) override {
    ++send_calls;
    return send_ok;
  }

  bool ready = true;
  bool send_ok = true;
  uint32_t ensure_calls = 0;
  uint32_t send_calls = 0;
};

class FakeCloudExtensionPort final : public ncos::interfaces::cloud::CloudExtensionPort {
 public:
  bool ensure_ready() override {
    ++ensure_calls;
    return ready;
  }

  bool submit_extension(const ncos::core::contracts::CloudExtensionRequest& request,
                        ncos::core::contracts::CloudExtensionResponse* out_response) override {
    ++submit_calls;
    last_request = request;

    if (out_response == nullptr) {
      return false;
    }

    out_response->accepted = accepted;
    out_response->applied = applied;
    out_response->safe_offline_compatible = true;
    out_response->reason = accepted ? "extension_applied" : "extension_rejected";
    return accepted;
  }

  bool ready = true;
  bool accepted = true;
  bool applied = true;
  uint32_t ensure_calls = 0;
  uint32_t submit_calls = 0;
  ncos::core::contracts::CloudExtensionRequest last_request{};
};

ncos::config::RuntimeConfig make_config(bool bridge_enabled, bool extension_enabled) {
  ncos::config::RuntimeConfig cfg = ncos::config::kGlobalConfig.runtime;
  cfg.cloud_sync_enabled = bridge_enabled;
  cfg.cloud_bridge_enabled = bridge_enabled;
  cfg.cloud_extension_enabled = extension_enabled;
  cfg.cloud_sync_interval_ms = 1000;
  cfg.cloud_sync_retry_backoff_ms = 4000;
  cfg.cloud_sync_failure_threshold = 3;
  return cfg;
}

ncos::core::contracts::CompanionSnapshot make_snapshot() {
  ncos::core::contracts::CompanionSnapshot snapshot{};
  snapshot.revision = 7;
  snapshot.captured_at_ms = 1000;
  snapshot.runtime.initialized = true;
  snapshot.runtime.started = true;
  snapshot.runtime.scheduler_tasks = 2;
  snapshot.governance.allowed_total = 3;
  snapshot.energetic.mode = ncos::core::contracts::EnergyMode::kNominal;
  snapshot.energetic.battery_percent = 74;
  return snapshot;
}

ncos::core::contracts::CloudExtensionRequest make_request(uint64_t created_at_ms,
                                                          uint32_t ttl_ms,
                                                          uint32_t trace_id = 123) {
  ncos::core::contracts::CloudExtensionRequest request{};
  request.trace_id = trace_id;
  request.capability = ncos::core::contracts::CloudExtensionCapability::kTelemetryHints;
  request.priority = 1;
  request.created_at_ms = created_at_ms;
  request.ttl_ms = ttl_ms;
  return request;
}

}  // namespace

void test_cloud_bridge_service_keeps_offline_authority_when_extension_is_disabled() {
  FakeCloudSyncPort sync_port{};
  FakeCloudExtensionPort extension_port{};

  ncos::services::cloud::CloudBridgeService service;
  service.bind_sync_port(&sync_port);
  service.bind_extension_port(&extension_port);

  const auto cfg = make_config(true, false);
  TEST_ASSERT_TRUE(service.initialize(1, 1000, cfg));
  TEST_ASSERT_TRUE(service.tick(make_snapshot(), 1500));

  ncos::core::contracts::CloudExtensionResponse response{};
  const bool ok = service.submit_extension(make_request(1500, 1000), 1800, &response);

  TEST_ASSERT_FALSE(ok);
  TEST_ASSERT_EQUAL_UINT32(1, service.state().extension_rejected_total);
  TEST_ASSERT_EQUAL_STRING("extension_disabled_offline_first", response.reason);
  TEST_ASSERT_TRUE(service.state().offline_authoritative);
}

void test_cloud_bridge_service_accepts_extension_when_connected_and_policy_allows() {
  FakeCloudSyncPort sync_port{};
  FakeCloudExtensionPort extension_port{};

  ncos::services::cloud::CloudBridgeService service;
  service.bind_sync_port(&sync_port);
  service.bind_extension_port(&extension_port);

  const auto cfg = make_config(true, true);
  TEST_ASSERT_TRUE(service.initialize(1, 1000, cfg));
  TEST_ASSERT_TRUE(service.tick(make_snapshot(), 1500));

  ncos::core::contracts::CloudExtensionResponse response{};
  const bool ok = service.submit_extension(make_request(1500, 1000), 1700, &response);

  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(response.accepted);
  TEST_ASSERT_TRUE(response.applied);
  TEST_ASSERT_EQUAL_UINT32(1, service.state().extension_applied_total);
  TEST_ASSERT_EQUAL_UINT32(1, extension_port.submit_calls);
}

void test_cloud_bridge_service_rejects_expired_extension_request() {
  FakeCloudSyncPort sync_port{};
  FakeCloudExtensionPort extension_port{};

  ncos::services::cloud::CloudBridgeService service;
  service.bind_sync_port(&sync_port);
  service.bind_extension_port(&extension_port);

  const auto cfg = make_config(true, true);
  TEST_ASSERT_TRUE(service.initialize(1, 1000, cfg));
  TEST_ASSERT_TRUE(service.tick(make_snapshot(), 1500));

  ncos::core::contracts::CloudExtensionResponse response{};
  const bool ok = service.submit_extension(make_request(1000, 200), 1500, &response);

  TEST_ASSERT_FALSE(ok);
  TEST_ASSERT_EQUAL_STRING("extension_request_expired", response.reason);
  TEST_ASSERT_EQUAL_UINT32(1, service.state().extension_rejected_total);
  TEST_ASSERT_EQUAL_UINT32(0, extension_port.submit_calls);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_cloud_bridge_service_keeps_offline_authority_when_extension_is_disabled);
  RUN_TEST(test_cloud_bridge_service_accepts_extension_when_connected_and_policy_allows);
  RUN_TEST(test_cloud_bridge_service_rejects_expired_extension_request);
  return UNITY_END();
}
