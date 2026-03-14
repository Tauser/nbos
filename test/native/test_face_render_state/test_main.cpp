#include <unity.h>

#include "core/contracts/face_render_state_contracts.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_render_state_baseline_is_valid_and_safe() {
  const ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(state));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::FaceRenderSafetyMode::kSafeFallback),
                        static_cast<int>(state.safety_mode));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::FacePresetId::kNeutralBaseline),
                        static_cast<int>(state.preset));
  TEST_ASSERT_EQUAL_UINT16(0, state.owner_service);
}

void test_face_render_state_rejects_invalid_percent_ranges() {
  ncos::core::contracts::FaceRenderState invalid =
      ncos::core::contracts::make_face_render_state_baseline();
  invalid.lids.openness_percent = 150;

  TEST_ASSERT_FALSE(ncos::core::contracts::is_valid(invalid));
}

void test_face_render_state_layer_claim_applies_with_explicit_priority() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  ncos::core::contracts::FaceLayerClaim first{};
  first.layer = ncos::models::face::FaceLayer::kEyes;
  first.requester_service = 10;
  first.priority = 3;
  first.source_clip_id = 1001;

  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, first, 1000));

  ncos::core::contracts::FaceLayerClaim lower{};
  lower.layer = ncos::models::face::FaceLayer::kEyes;
  lower.requester_service = 22;
  lower.priority = 2;
  lower.source_clip_id = 1002;

  TEST_ASSERT_FALSE(ncos::core::contracts::apply_layer_claim(&state, lower, 1100));

  ncos::core::contracts::FaceLayerClaim higher{};
  higher.layer = ncos::models::face::FaceLayer::kEyes;
  higher.requester_service = 22;
  higher.priority = 8;
  higher.source_clip_id = 1003;

  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, higher, 1200));

  const size_t eye_layer_idx = ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kEyes);
  TEST_ASSERT_EQUAL_UINT16(22, state.composition.layers[eye_layer_idx].owner_service);
  TEST_ASSERT_EQUAL_UINT8(8, state.composition.layers[eye_layer_idx].priority);
  TEST_ASSERT_EQUAL_UINT32(1003, state.composition.layers[eye_layer_idx].source_clip_id);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_render_state_baseline_is_valid_and_safe);
  RUN_TEST(test_face_render_state_rejects_invalid_percent_ranges);
  RUN_TEST(test_face_render_state_layer_claim_applies_with_explicit_priority);
  return UNITY_END();
}
