#include "services/face/face_compositor.hpp"

namespace {

uint32_t with_default(uint32_t requested, uint32_t fallback_value) {
  return requested == 0 ? fallback_value : requested;
}

}  // namespace

namespace ncos::services::face {

FaceLayerTimingPolicy FaceCompositor::default_timing_for(ncos::models::face::FaceLayer layer) {
  switch (layer) {
    case ncos::models::face::FaceLayer::kBase:
      return {450, 220};
    case ncos::models::face::FaceLayer::kBlink:
      return {110, 80};
    case ncos::models::face::FaceLayer::kGaze:
      return {280, 140};
    case ncos::models::face::FaceLayer::kModulation:
      return {180, 120};
    case ncos::models::face::FaceLayer::kTransient:
      return {220, 180};
    case ncos::models::face::FaceLayer::kClip:
      return {800, 280};
    default:
      return {200, 120};
  }
}

bool FaceCompositor::bind_state(ncos::core::contracts::FaceRenderState* state) {
  if (state == nullptr || !ncos::core::contracts::is_valid(*state)) {
    return false;
  }

  state_ = state;
  for (size_t i = 0; i < ncos::models::face::kFaceLayerCount; ++i) {
    runtime_[i] = FaceLayerRuntime{};
  }
  return true;
}

FaceLayerDecision FaceCompositor::request_layer(const FaceLayerRequest& request, uint64_t now_ms) {
  FaceLayerDecision decision{};

  if (state_ == nullptr || !ncos::core::contracts::is_valid(*state_) || request.requester_service == 0) {
    decision.reason = FaceComposeRejectReason::kInvalidState;
    return decision;
  }

  const auto policy = ncos::core::contracts::face_layer_policy(request.layer);
  if (policy.required_owner_role != request.requester_role) {
    decision.reason = FaceComposeRejectReason::kOwnershipMismatch;
    return decision;
  }

  const size_t idx = ncos::models::face::face_layer_index(request.layer);
  auto& runtime = runtime_[idx];
  const auto& current = state_->composition.layers[idx];

  decision.active_owner_service = current.owner_service;
  decision.hold_until_ms = runtime.hold_until_ms;
  decision.cooldown_until_ms = runtime.cooldown_until_ms;

  if (runtime.cooldown_blocked_service == request.requester_service && now_ms < runtime.cooldown_until_ms) {
    decision.reason = FaceComposeRejectReason::kCooldownActive;
    return decision;
  }

  const bool same_owner = current.owner_service == request.requester_service;
  if (!same_owner && current.owner_service != 0 && now_ms < runtime.hold_until_ms &&
      request.priority <= current.priority) {
    decision.reason = FaceComposeRejectReason::kHoldActive;
    return decision;
  }

  ncos::core::contracts::FaceLayerClaim claim{};
  claim.layer = request.layer;
  claim.requester_role = request.requester_role;
  claim.requester_service = request.requester_service;
  claim.priority = request.priority;
  claim.source_clip_id = request.source_clip_id;

  const uint16_t previous_owner = current.owner_service;
  if (!ncos::core::contracts::apply_layer_claim(state_, claim, now_ms)) {
    decision.reason = FaceComposeRejectReason::kClaimRejected;
    return decision;
  }

  const auto timing = default_timing_for(request.layer);
  const uint32_t hold_ms = with_default(request.hold_ms, timing.default_hold_ms);
  const uint32_t cooldown_ms = with_default(request.cooldown_ms, timing.default_cooldown_ms);

  runtime.hold_until_ms = now_ms + hold_ms;

  if (previous_owner != 0 && previous_owner != request.requester_service) {
    runtime.cooldown_blocked_service = previous_owner;
    runtime.cooldown_until_ms = now_ms + cooldown_ms;
  }

  decision.granted = true;
  decision.reason = FaceComposeRejectReason::kNone;
  decision.active_owner_service = request.requester_service;
  decision.hold_until_ms = runtime.hold_until_ms;
  decision.cooldown_until_ms = runtime.cooldown_until_ms;
  return decision;
}

bool FaceCompositor::can_write(ncos::models::face::FaceLayer layer, uint16_t requester_service) const {
  if (state_ == nullptr || requester_service == 0) {
    return false;
  }

  const size_t idx = ncos::models::face::face_layer_index(layer);
  const auto& current = state_->composition.layers[idx];
  return current.owner_service == requester_service;
}

void FaceCompositor::tick(uint64_t now_ms) {
  for (size_t i = 0; i < ncos::models::face::kFaceLayerCount; ++i) {
    auto& slot = runtime_[i];
    if (slot.cooldown_blocked_service != 0 && now_ms >= slot.cooldown_until_ms) {
      slot.cooldown_blocked_service = 0;
      slot.cooldown_until_ms = 0;
    }
  }
}

uint64_t FaceCompositor::hold_until_ms(ncos::models::face::FaceLayer layer) const {
  return runtime_[ncos::models::face::face_layer_index(layer)].hold_until_ms;
}

uint64_t FaceCompositor::cooldown_until_ms(ncos::models::face::FaceLayer layer) const {
  return runtime_[ncos::models::face::face_layer_index(layer)].cooldown_until_ms;
}

}  // namespace ncos::services::face
