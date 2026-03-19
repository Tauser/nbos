#include <unity.h>

#include "drivers/ttlinker/ttlinker_transport_bsp.hpp"

// Native tests run with test_build_src = no.
#include "drivers/ttlinker/ttlinker_transport_bsp.cpp"

void setUp() {}
void tearDown() {}

void test_ttlinker_transport_bsp_centralizes_uart_baseline() {
  const auto& bsp = ncos::drivers::ttlinker::active_ttlinker_transport_bsp();

  TEST_ASSERT_EQUAL_STRING("freenove_esp32s3_wroom_cam_n16r8", bsp.board_name);
  TEST_ASSERT_EQUAL_INT(43, bsp.wiring.tx_gpio);
  TEST_ASSERT_EQUAL_INT(44, bsp.wiring.rx_gpio);
  TEST_ASSERT_EQUAL_INT(1, bsp.uart.port);
  TEST_ASSERT_EQUAL_INT(115200, bsp.uart.baud_rate);
  TEST_ASSERT_EQUAL_INT(512, bsp.uart.rx_buffer_size);
  TEST_ASSERT_EQUAL_INT(512, bsp.uart.tx_buffer_size);
  TEST_ASSERT_TRUE(bsp.flags.probe_allowed);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ttlinker_transport_bsp_centralizes_uart_baseline);
  return UNITY_END();
}
