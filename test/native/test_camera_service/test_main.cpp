#include <unity.h>

#include "services/vision/camera_service.hpp"

// Native tests run with test_build_src = no.
#include "services/vision/camera_service.cpp"

namespace {

class FakeCameraPort final : public ncos::interfaces::vision::CameraPort {
 public:
  bool ready = true;
  bool component = true;
  bool capture_ok = true;
  uint16_t width = 320;
  uint16_t height = 240;
  uint32_t bytes = 1024;
  uint32_t capture_calls = 0;

  bool ensure_ready() override {
    return ready;
  }

  bool is_component_available() const override {
    return component;
  }

  bool capture_frame(ncos::interfaces::vision::CameraFrameMeta* out_frame) override {
    ++capture_calls;
    if (out_frame != nullptr) {
      out_frame->width = width;
      out_frame->height = height;
      out_frame->payload_bytes = bytes;
    }
    return capture_ok;
  }
};

}  // namespace

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_camera_service_initializes_when_port_ready() {
  FakeCameraPort port;

  ncos::services::vision::CameraService service;
  service.bind_port(&port);

  TEST_ASSERT_TRUE(service.initialize(1000));
  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.initialized);
  TEST_ASSERT_TRUE(state.component_available);
  TEST_ASSERT_TRUE(state.capture_ready);
}

void test_camera_service_tracks_frame_ingestion() {
  FakeCameraPort port;

  ncos::services::vision::CameraService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(2000));

  service.tick(2000);

  const auto& state = service.state();
  TEST_ASSERT_TRUE(state.last_capture_ok);
  TEST_ASSERT_EQUAL_UINT32(1, state.capture_success_total);
  TEST_ASSERT_EQUAL_UINT32(0, state.capture_failure_total);
  TEST_ASSERT_EQUAL_UINT16(320, state.last_frame_width);
  TEST_ASSERT_EQUAL_UINT16(240, state.last_frame_height);
  TEST_ASSERT_EQUAL_UINT32(1024, state.last_frame_bytes);
  TEST_ASSERT_EQUAL_UINT64(1, state.last_frame_sequence);
}

void test_camera_service_records_capture_failure() {
  FakeCameraPort port;
  port.capture_ok = false;

  ncos::services::vision::CameraService service;
  service.bind_port(&port);
  TEST_ASSERT_TRUE(service.initialize(3000));

  service.tick(3000);

  const auto& state = service.state();
  TEST_ASSERT_FALSE(state.last_capture_ok);
  TEST_ASSERT_EQUAL_UINT32(0, state.capture_success_total);
  TEST_ASSERT_EQUAL_UINT32(1, state.capture_failure_total);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_camera_service_initializes_when_port_ready);
  RUN_TEST(test_camera_service_tracks_frame_ingestion);
  RUN_TEST(test_camera_service_records_capture_failure);
  return UNITY_END();
}
