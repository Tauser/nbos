#include <unity.h>
#include <string.h>

#include "core/contracts/face_multimodal_contracts.hpp"
#include "services/face/face_multimodal_sync.hpp"
#include "services/face/face_tooling.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_multimodal_contracts.cpp"
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_compositor.cpp"
#include "services/face/face_multimodal_sync.cpp"
#include "services/face/face_tooling.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_multimodal_input_aggregates_audio_touch_and_motion() {
  ncos::core::contracts::AudioRuntimeState audio{};
  audio.initialized = true;
  audio.output_ready = true;
  audio.input_ready = true;
  audio.last_capture_ok = true;
  audio.last_peak_level = 12000;
  audio.last_capture_samples = 256;

  ncos::core::contracts::TouchRuntimeState touch{};
  touch.initialized = true;
  touch.last_read_ok = true;
  touch.normalized_level = 650;

  ncos::core::contracts::ImuRuntimeState imu{};
  imu.initialized = true;
  imu.last_read_ok = true;
  imu.gx_dps = 80;
  imu.gy_dps = -60;
  imu.gz_dps = 40;

  ncos::core::contracts::CompanionSnapshot companion{};
  ncos::core::contracts::BehaviorRuntimeState behavior{};

  const auto input =
      ncos::core::contracts::make_face_multimodal_input(audio, touch, imu, companion, behavior, 1234);

  TEST_ASSERT_TRUE(input.audio_ready);
  TEST_ASSERT_TRUE(input.touch_active);
  TEST_ASSERT_TRUE(input.imu_ready);
  TEST_ASSERT_TRUE(input.motion_active);
  TEST_ASSERT_GREATER_THAN_UINT8(0, input.audio_energy_percent);
  TEST_ASSERT_GREATER_THAN_UINT8(0, input.touch_intensity_percent);
  TEST_ASSERT_GREATER_THAN_UINT8(0, input.motion_intensity_percent);
  TEST_ASSERT_EQUAL_UINT64(1234, input.observed_at_ms);
}

void test_face_multimodal_sync_applies_modulation_under_ownership() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  ncos::services::face::FaceCompositor compositor{};
  ncos::services::face::FaceMultimodalSync sync{};

  TEST_ASSERT_TRUE(compositor.bind_state(&state));

  ncos::core::contracts::FaceMultimodalInput input{};
  input.audio_ready = true;
  input.touch_active = true;
  input.imu_ready = true;
  input.motion_active = true;
  input.audio_energy_percent = 70;
  input.touch_intensity_percent = 55;
  input.motion_intensity_percent = 40;

  ncos::services::face::FaceMultimodalSyncResult result{};
  TEST_ASSERT_TRUE(sync.apply(input, &compositor, &state, 34, 2222, &result));

  TEST_ASSERT_TRUE(result.applied);
  TEST_ASSERT_EQUAL_UINT8(result.target_focus_percent, state.eyes.focus_percent);
  TEST_ASSERT_EQUAL_UINT8(result.target_lid_open_percent, state.lids.openness_percent);
  TEST_ASSERT_EQUAL_UINT8(result.target_brow_intensity_percent, state.brows.intensity_percent);
}

void test_face_tooling_exports_preview_json_snapshot() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  state.revision = 42;
  state.owner_service = 34;
  state.eyes.focus_percent = 61;

  ncos::services::face::FaceTuningTelemetry tuning{};
  tuning.frame_budget_us = 14000;
  tuning.stages.total_us = 9100;
  tuning.stages.render_us = 5300;
  tuning.dirty_area_px = 1824;
  tuning.avg_frame_time_us = 6200;
  tuning.peak_frame_time_us = 9800;
  tuning.rendered_frames = 12;
  tuning.skipped_duplicate_frames = 4;
  tuning.degradation = ncos::services::face::FaceVisualDegradationFlag::kHighContrastMotion |
                       ncos::services::face::FaceVisualDegradationFlag::kDiagonalMotion;

  const auto snapshot = ncos::services::face::make_face_preview_snapshot(state, false, 5000, tuning);

  char json[512] = {};
  const size_t written = ncos::services::face::export_face_preview_json(snapshot, json, sizeof(json));

  TEST_ASSERT_GREATER_THAN_UINT32(0, static_cast<uint32_t>(written));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"tooling\":\"exploratory\""));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"state_revision\":42"));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"focus\":61"));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"total_us\":9100"));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"render_us\":5300"));
  TEST_ASSERT_NOT_EQUAL(nullptr, strstr(json, "\"degradation\":\"high_contrast_motion|diagonal_motion\""));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_multimodal_input_aggregates_audio_touch_and_motion);
  RUN_TEST(test_face_multimodal_sync_applies_modulation_under_ownership);
  RUN_TEST(test_face_tooling_exports_preview_json_snapshot);
  return UNITY_END();
}

