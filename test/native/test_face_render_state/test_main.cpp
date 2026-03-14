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
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::FaceCompositionMode::kLayered),
                        static_cast<int>(state.composition.mode));
}

void test_face_layer_policy_is_explicit_for_all_layers() {
  const auto blink_policy =
      ncos::core::contracts::face_layer_policy(ncos::models::face::FaceLayer::kBlink);
  const auto gaze_policy =
      ncos::core::contracts::face_layer_policy(ncos::models::face::FaceLayer::kGaze);
  const auto transient_policy =
      ncos::core::contracts::face_layer_policy(ncos::models::face::FaceLayer::kTransient);
  const auto clip_policy =
      ncos::core::contracts::face_layer_policy(ncos::models::face::FaceLayer::kClip);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::FaceLayerOwnerRole::kBlinkOwner),
                        static_cast<int>(blink_policy.required_owner_role));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner),
                        static_cast<int>(gaze_policy.required_owner_role));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::FaceLayerOwnerRole::kTransientOwner),
                        static_cast<int>(transient_policy.required_owner_role));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::core::contracts::FaceLayerOwnerRole::kClipOwner),
                        static_cast<int>(clip_policy.required_owner_role));
}

void test_blink_and_clip_cannot_claim_each_other_layers() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  ncos::core::contracts::FaceLayerClaim invalid_clip_on_blink{};
  invalid_clip_on_blink.layer = ncos::models::face::FaceLayer::kBlink;
  invalid_clip_on_blink.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kClipOwner;
  invalid_clip_on_blink.requester_service = 40;
  invalid_clip_on_blink.priority = 9;

  TEST_ASSERT_FALSE(ncos::core::contracts::apply_layer_claim(&state, invalid_clip_on_blink, 1000));

  ncos::core::contracts::FaceLayerClaim invalid_blink_on_clip{};
  invalid_blink_on_clip.layer = ncos::models::face::FaceLayer::kClip;
  invalid_blink_on_clip.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBlinkOwner;
  invalid_blink_on_clip.requester_service = 20;
  invalid_blink_on_clip.priority = 9;

  TEST_ASSERT_FALSE(ncos::core::contracts::apply_layer_claim(&state, invalid_blink_on_clip, 1100));
}

void test_gaze_and_transient_do_not_conflict_by_layer_ownership() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  ncos::core::contracts::FaceLayerClaim gaze_claim{};
  gaze_claim.layer = ncos::models::face::FaceLayer::kGaze;
  gaze_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze_claim.requester_service = 31;
  gaze_claim.priority = 5;

  ncos::core::contracts::FaceLayerClaim transient_claim{};
  transient_claim.layer = ncos::models::face::FaceLayer::kTransient;
  transient_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kTransientOwner;
  transient_claim.requester_service = 41;
  transient_claim.priority = 7;

  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, gaze_claim, 1000));
  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, transient_claim, 1010));

  const size_t gaze_idx = ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kGaze);
  const size_t transient_idx =
      ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kTransient);

  TEST_ASSERT_EQUAL_UINT16(31, state.composition.layers[gaze_idx].owner_service);
  TEST_ASSERT_EQUAL_UINT16(41, state.composition.layers[transient_idx].owner_service);
}

void test_modulation_cannot_overwrite_base_layer() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  ncos::core::contracts::FaceLayerClaim base_claim{};
  base_claim.layer = ncos::models::face::FaceLayer::kBase;
  base_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBaseOwner;
  base_claim.requester_service = 11;
  base_claim.priority = 3;
  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, base_claim, 1000));

  ncos::core::contracts::FaceLayerClaim invalid_modulation_over_base{};
  invalid_modulation_over_base.layer = ncos::models::face::FaceLayer::kBase;
  invalid_modulation_over_base.requester_role =
      ncos::core::contracts::FaceLayerOwnerRole::kModulationOwner;
  invalid_modulation_over_base.requester_service = 55;
  invalid_modulation_over_base.priority = 9;

  TEST_ASSERT_FALSE(
      ncos::core::contracts::apply_layer_claim(&state, invalid_modulation_over_base, 1010));

  const size_t base_idx = ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kBase);
  TEST_ASSERT_EQUAL_UINT16(11, state.composition.layers[base_idx].owner_service);
}

void test_face_claim_requires_explicit_service_identity() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  ncos::core::contracts::FaceLayerClaim missing_service{};
  missing_service.layer = ncos::models::face::FaceLayer::kBlink;
  missing_service.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBlinkOwner;
  missing_service.requester_service = 0;
  missing_service.priority = 8;

  TEST_ASSERT_FALSE(ncos::core::contracts::can_apply_layer_claim(state, missing_service));
  TEST_ASSERT_FALSE(ncos::core::contracts::apply_layer_claim(&state, missing_service, 1000));
}

void test_face_composition_lock_blocks_foreign_takeover() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  ncos::core::contracts::FaceLayerClaim owner_claim{};
  owner_claim.layer = ncos::models::face::FaceLayer::kClip;
  owner_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kClipOwner;
  owner_claim.requester_service = 90;
  owner_claim.priority = 8;
  owner_claim.source_clip_id = 123;

  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, owner_claim, 1000));
  state.composition.composition_locked = true;

  ncos::core::contracts::FaceLayerClaim foreign_claim = owner_claim;
  foreign_claim.requester_service = 91;
  foreign_claim.priority = 9;

  TEST_ASSERT_FALSE(ncos::core::contracts::can_apply_layer_claim(state, foreign_claim));
  TEST_ASSERT_FALSE(ncos::core::contracts::apply_layer_claim(&state, foreign_claim, 1010));

  const size_t clip_idx = ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kClip);
  TEST_ASSERT_EQUAL_UINT16(90, state.composition.layers[clip_idx].owner_service);
}

void test_face_claim_rejects_invalid_state_preventing_renderer_semantic_drift() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  const size_t blink_idx = ncos::models::face::face_layer_index(ncos::models::face::FaceLayer::kBlink);
  state.composition.layers[blink_idx].owner_role = ncos::core::contracts::FaceLayerOwnerRole::kClipOwner;

  ncos::core::contracts::FaceLayerClaim blink_claim{};
  blink_claim.layer = ncos::models::face::FaceLayer::kBlink;
  blink_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kBlinkOwner;
  blink_claim.requester_service = 22;
  blink_claim.priority = 6;

  TEST_ASSERT_FALSE(ncos::core::contracts::is_valid(state));
  TEST_ASSERT_FALSE(ncos::core::contracts::apply_layer_claim(&state, blink_claim, 1000));
}

void test_face_claim_updates_revision_and_timestamp() {
  ncos::core::contracts::FaceRenderState state =
      ncos::core::contracts::make_face_render_state_baseline();

  const uint32_t prev_revision = state.revision;

  ncos::core::contracts::FaceLayerClaim gaze_claim{};
  gaze_claim.layer = ncos::models::face::FaceLayer::kGaze;
  gaze_claim.requester_role = ncos::core::contracts::FaceLayerOwnerRole::kGazeOwner;
  gaze_claim.requester_service = 31;
  gaze_claim.priority = 5;

  TEST_ASSERT_TRUE(ncos::core::contracts::apply_layer_claim(&state, gaze_claim, 4242));
  TEST_ASSERT_EQUAL_UINT32(prev_revision + 1, state.revision);
  TEST_ASSERT_EQUAL_UINT64(4242, state.updated_at_ms);
  TEST_ASSERT_EQUAL_UINT16(31, state.owner_service);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_render_state_baseline_is_valid_and_safe);
  RUN_TEST(test_face_layer_policy_is_explicit_for_all_layers);
  RUN_TEST(test_blink_and_clip_cannot_claim_each_other_layers);
  RUN_TEST(test_gaze_and_transient_do_not_conflict_by_layer_ownership);
  RUN_TEST(test_modulation_cannot_overwrite_base_layer);
  RUN_TEST(test_face_claim_requires_explicit_service_identity);
  RUN_TEST(test_face_composition_lock_blocks_foreign_takeover);
  RUN_TEST(test_face_claim_rejects_invalid_state_preventing_renderer_semantic_drift);
  RUN_TEST(test_face_claim_updates_revision_and_timestamp);
  return UNITY_END();
}
