#include <unity.h>

#include "hal/platform/monotonic_clock.hpp"
#include "hal/platform/reset_reason.hpp"

// Native tests run with test_build_src = no.
#include "hal/platform/monotonic_clock.cpp"
#include "hal/platform/reset_reason.cpp"

void setUp() {}
void tearDown() {}

void test_monotonic_clock_is_non_decreasing() {
  const uint64_t before_us = ncos::hal::platform::monotonic_time_us();
  const uint64_t before_ms = ncos::hal::platform::monotonic_time_ms();
  const uint64_t after_us = ncos::hal::platform::monotonic_time_us();
  const uint64_t after_ms = ncos::hal::platform::monotonic_time_ms();

  TEST_ASSERT_TRUE(after_us >= before_us);
  TEST_ASSERT_TRUE(after_ms >= before_ms);
}

void test_native_reset_reason_defaults_to_unknown_stable() {
  const auto& reset = ncos::hal::platform::active_reset_reason();

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ncos::hal::platform::ResetReasonKind::kUnknown),
                          static_cast<uint8_t>(reset.kind));
  TEST_ASSERT_EQUAL_STRING("unknown", reset.name);
  TEST_ASSERT_FALSE(reset.unstable_boot_context);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_monotonic_clock_is_non_decreasing);
  RUN_TEST(test_native_reset_reason_defaults_to_unknown_stable);
  return UNITY_END();
}
