#include <unity.h>

#include "app/lifecycle/system_lifecycle.hpp"
#include "config/system_config.hpp"
#include "core/runtime/runtime_readiness.hpp"

// Native tests run with test_build_src = no, so we pull the required units explicitly.
#include "app/lifecycle/system_lifecycle.cpp"
#include "core/runtime/runtime_readiness.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_runtime_readiness_ready_when_all_criteria_are_true() {
  ncos::app::lifecycle::SystemLifecycle lifecycle;
  lifecycle.start_boot();
  lifecycle.finish_boot(false, false);

  ncos::core::runtime::RuntimeStatus status{};
  status.initialized = true;
  status.started = true;
  status.scheduler_tasks = 2;
  status.safe_mode = false;
  status.fault_count = 0;

  const auto report =
      ncos::core::runtime::evaluate_runtime_readiness(ncos::config::kGlobalConfig, lifecycle, status);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::runtime::RuntimeReadinessLevel::kReady),
                        static_cast<int>(report.level));
  TEST_ASSERT_TRUE(report.config_valid);
  TEST_ASSERT_TRUE(report.board_profile_bound);
}

void test_runtime_readiness_conditionally_ready_when_fault_history_exists() {
  ncos::app::lifecycle::SystemLifecycle lifecycle;
  lifecycle.start_boot();
  lifecycle.finish_boot(false, false);

  ncos::core::runtime::RuntimeStatus status{};
  status.initialized = true;
  status.started = true;
  status.scheduler_tasks = 1;
  status.safe_mode = false;
  status.fault_count = 1;

  const auto report =
      ncos::core::runtime::evaluate_runtime_readiness(ncos::config::kGlobalConfig, lifecycle, status);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::runtime::RuntimeReadinessLevel::kConditionallyReady),
                        static_cast<int>(report.level));
}

void test_runtime_readiness_not_ready_when_runtime_not_started() {
  ncos::app::lifecycle::SystemLifecycle lifecycle;
  lifecycle.start_boot();
  lifecycle.finish_boot(false, false);

  ncos::core::runtime::RuntimeStatus status{};
  status.initialized = true;
  status.started = false;
  status.scheduler_tasks = 0;

  const auto report =
      ncos::core::runtime::evaluate_runtime_readiness(ncos::config::kGlobalConfig, lifecycle, status);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::runtime::RuntimeReadinessLevel::kNotReady),
                        static_cast<int>(report.level));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_runtime_readiness_ready_when_all_criteria_are_true);
  RUN_TEST(test_runtime_readiness_conditionally_ready_when_fault_history_exists);
  RUN_TEST(test_runtime_readiness_not_ready_when_runtime_not_started);
  return UNITY_END();
}
