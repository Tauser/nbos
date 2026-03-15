#include <unity.h>

#include "services/face/face_preset_library.hpp"

// Native tests run with test_build_src = no.
#include "core/contracts/face_render_state_contracts.cpp"
#include "services/face/face_preset_library.cpp"

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_face_preset_library_has_exploratory_catalog() {
  const size_t count = ncos::services::face::face_preset_library_count();
  TEST_ASSERT_TRUE(count >= 8);

  const auto* items = ncos::services::face::face_preset_library_items();
  TEST_ASSERT_NOT_NULL(items);

  for (size_t i = 0; i < count; ++i) {
    TEST_ASSERT_TRUE(items[i].exploratory_only);
    TEST_ASSERT_TRUE(items[i].name != nullptr);
    TEST_ASSERT_TRUE(items[i].contrast_note != nullptr);
    TEST_ASSERT_TRUE(items[i].name[0] != '\0');
    TEST_ASSERT_TRUE(items[i].contrast_note[0] != '\0');
  }
}

void test_face_preset_library_covers_all_readability_tiers_with_depth() {
  size_t tier_a = 0;
  size_t tier_b = 0;
  size_t tier_c = 0;

  const size_t count = ncos::services::face::face_preset_library_count();
  const auto* items = ncos::services::face::face_preset_library_items();

  for (size_t i = 0; i < count; ++i) {
    if (items[i].readability_tier == ncos::services::face::FaceReadabilityTier::kTierAImmediate) {
      ++tier_a;
    } else if (items[i].readability_tier ==
               ncos::services::face::FaceReadabilityTier::kTierBBalanced) {
      ++tier_b;
    } else if (items[i].readability_tier == ncos::services::face::FaceReadabilityTier::kTierCNuanced) {
      ++tier_c;
    }
  }

  TEST_ASSERT_TRUE(tier_a >= 2);
  TEST_ASSERT_TRUE(tier_b >= 2);
  TEST_ASSERT_TRUE(tier_c >= 2);
}

void test_face_preset_library_applies_preset_to_render_state() {
  auto state = ncos::core::contracts::make_face_render_state_baseline();
  const uint32_t before_revision = state.revision;

  TEST_ASSERT_TRUE(ncos::services::face::apply_face_preset(
      ncos::services::face::FaceExploratoryPresetId::kFocusedAttend, &state, 1234));

  TEST_ASSERT_TRUE(ncos::core::contracts::is_valid(state));
  TEST_ASSERT_TRUE(state.revision > before_revision);
  TEST_ASSERT_EQUAL_UINT64(1234, state.updated_at_ms);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::models::face::GazeAnchor::kUser),
                        static_cast<int>(state.eyes.anchor));
}

void test_face_preset_library_keeps_contrast_between_tiers() {
  auto tier_a_state = ncos::core::contracts::make_face_render_state_baseline();
  auto tier_c_state = ncos::core::contracts::make_face_render_state_baseline();

  TEST_ASSERT_TRUE(ncos::services::face::apply_face_preset(
      ncos::services::face::FaceExploratoryPresetId::kClarityNeutral, &tier_a_state, 2000));
  TEST_ASSERT_TRUE(ncos::services::face::apply_face_preset(
      ncos::services::face::FaceExploratoryPresetId::kAttentiveLock, &tier_c_state, 2100));

  TEST_ASSERT_NOT_EQUAL(tier_a_state.geometry.eye_spacing_percent,
                        tier_c_state.geometry.eye_spacing_percent);
  TEST_ASSERT_NOT_EQUAL(tier_a_state.eyes.focus_percent, tier_c_state.eyes.focus_percent);
  TEST_ASSERT_NOT_EQUAL(tier_a_state.lids.openness_percent, tier_c_state.lids.openness_percent);
}

void test_face_preset_library_supports_new_exploratory_entries() {
  const auto* social = ncos::services::face::find_face_preset(
      ncos::services::face::FaceExploratoryPresetId::kSocialGlance);
  const auto* idle = ncos::services::face::find_face_preset(
      ncos::services::face::FaceExploratoryPresetId::kIdleDrift);

  TEST_ASSERT_NOT_NULL(social);
  TEST_ASSERT_NOT_NULL(idle);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::services::face::FaceReadabilityTier::kTierAImmediate),
                        static_cast<int>(social->readability_tier));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ncos::services::face::FaceReadabilityTier::kTierCNuanced),
                        static_cast<int>(idle->readability_tier));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_face_preset_library_has_exploratory_catalog);
  RUN_TEST(test_face_preset_library_covers_all_readability_tiers_with_depth);
  RUN_TEST(test_face_preset_library_applies_preset_to_render_state);
  RUN_TEST(test_face_preset_library_keeps_contrast_between_tiers);
  RUN_TEST(test_face_preset_library_supports_new_exploratory_entries);
  return UNITY_END();
}
