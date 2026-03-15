#include <unity.h>

#include "config/system_config.hpp"
#include "core/contracts/update_runtime_contracts.hpp"
#include "core/runtime/system_manager.hpp"
#include "interfaces/update/update_port.hpp"
#include "services/update/update_service.hpp"

// Native tests run with test_build_src = no.
#include "services/update/update_service.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

namespace {

class FakeUpdatePort final : public ncos::interfaces::update::UpdatePort {
 public:
  bool ensure_ready() override {
    ++ensure_ready_calls;
    return ensure_ready_ok;
  }

  bool read_boot_info(ncos::interfaces::update::OtaBootInfo* out_info) override {
    ++read_boot_info_calls;
    if (!read_boot_info_ok || out_info == nullptr) {
      return false;
    }
    *out_info = boot_info;
    return true;
  }

  bool confirm_running_image() override {
    ++confirm_calls;
    return confirm_ok;
  }

  bool request_rollback() override {
    ++rollback_calls;
    return rollback_ok;
  }

  bool ensure_ready_ok = true;
  bool read_boot_info_ok = true;
  bool confirm_ok = true;
  bool rollback_ok = true;

  ncos::interfaces::update::OtaBootInfo boot_info{};

  uint32_t ensure_ready_calls = 0;
  uint32_t read_boot_info_calls = 0;
  uint32_t confirm_calls = 0;
  uint32_t rollback_calls = 0;
};

ncos::config::RuntimeConfig make_runtime_cfg() {
  ncos::config::RuntimeConfig cfg = ncos::config::kGlobalConfig.runtime;
  cfg.ota_enabled = true;
  cfg.ota_remote_allowed = false;
  cfg.ota_confirm_uptime_ms = 1000;
  return cfg;
}

}  // namespace

void test_update_service_blocks_remote_ota_without_rollback_support() {
  FakeUpdatePort port{};
  port.boot_info.component_available = true;
  port.boot_info.rollback_supported = false;
  port.boot_info.pending_verify = false;
  port.boot_info.running_slot = ncos::core::contracts::OtaRunningSlot::kFactory;

  ncos::services::update::UpdateService service;
  service.bind_port(&port);

  ncos::config::RuntimeConfig cfg = make_runtime_cfg();
  cfg.ota_remote_allowed = true;

  TEST_ASSERT_TRUE(service.initialize(1, 100, cfg));
  const auto decision = service.evaluate_boot_policy(150);
  const auto& state = service.state();

  TEST_ASSERT_TRUE(decision.valid);
  TEST_ASSERT_TRUE(decision.request_safe_fallback);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::OtaHealth::kBlockedUnsafe),
                        static_cast<int>(state.health));
  TEST_ASSERT_EQUAL_UINT32(1, state.blocked_total);
}

void test_update_service_confirms_image_after_uptime_window() {
  FakeUpdatePort port{};
  port.boot_info.component_available = true;
  port.boot_info.rollback_supported = true;
  port.boot_info.pending_verify = true;
  port.boot_info.running_slot = ncos::core::contracts::OtaRunningSlot::kSlotA;
  port.confirm_ok = true;

  ncos::services::update::UpdateService service;
  service.bind_port(&port);

  ncos::config::RuntimeConfig cfg = make_runtime_cfg();
  cfg.ota_confirm_uptime_ms = 500;

  TEST_ASSERT_TRUE(service.initialize(1, 1000, cfg));

  ncos::core::runtime::RuntimeStatus runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.safe_mode = false;

  const auto decision = service.tick(1600, runtime);
  const auto& state = service.state();

  TEST_ASSERT_TRUE(decision.valid);
  TEST_ASSERT_TRUE(decision.request_confirm);
  TEST_ASSERT_EQUAL_UINT32(1, port.confirm_calls);
  TEST_ASSERT_FALSE(state.pending_confirm);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::OtaHealth::kNominal),
                        static_cast<int>(state.health));
}

void test_update_service_requests_operational_fallback_when_confirm_expires_in_safe_mode() {
  FakeUpdatePort port{};
  port.boot_info.component_available = true;
  port.boot_info.rollback_supported = true;
  port.boot_info.pending_verify = true;
  port.boot_info.running_slot = ncos::core::contracts::OtaRunningSlot::kSlotB;

  ncos::services::update::UpdateService service;
  service.bind_port(&port);

  ncos::config::RuntimeConfig cfg = make_runtime_cfg();
  cfg.ota_confirm_uptime_ms = 400;

  TEST_ASSERT_TRUE(service.initialize(1, 2000, cfg));

  ncos::core::runtime::RuntimeStatus runtime{};
  runtime.initialized = true;
  runtime.started = true;
  runtime.safe_mode = true;

  const auto decision = service.tick(2500, runtime);
  const auto& state = service.state();

  TEST_ASSERT_TRUE(decision.valid);
  TEST_ASSERT_TRUE(decision.request_safe_fallback);
  TEST_ASSERT_FALSE(decision.request_confirm);
  TEST_ASSERT_EQUAL_UINT32(0, port.confirm_calls);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::OtaHealth::kFallbackOperational),
                        static_cast<int>(state.health));
  TEST_ASSERT_EQUAL_UINT32(1, state.fallback_total);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_update_service_blocks_remote_ota_without_rollback_support);
  RUN_TEST(test_update_service_confirms_image_after_uptime_window);
  RUN_TEST(test_update_service_requests_operational_fallback_when_confirm_expires_in_safe_mode);
  return UNITY_END();
}
