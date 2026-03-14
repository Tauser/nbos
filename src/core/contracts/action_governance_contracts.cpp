#include "core/contracts/action_governance_contracts.hpp"

namespace ncos::core::contracts {

ActionDomain command_topic_to_domain(CommandTopic topic) {
  switch (topic) {
    case CommandTopic::kMotionExecute:
      return ActionDomain::kMotion;
    case CommandTopic::kFaceRenderExecute:
      return ActionDomain::kFace;
    case CommandTopic::kAudioOutputExecute:
      return ActionDomain::kAudio;
    case CommandTopic::kPowerModeSet:
      return ActionDomain::kPower;
    default:
      return ActionDomain::kFace;
  }
}

const char* action_domain_name(ActionDomain domain) {
  switch (domain) {
    case ActionDomain::kFace:
      return "face";
    case ActionDomain::kMotion:
      return "motion";
    case ActionDomain::kAudio:
      return "audio";
    case ActionDomain::kPower:
      return "power";
    case ActionDomain::kLed:
      return "led";
    default:
      return "unknown";
  }
}

const char* governance_reject_reason_name(GovernanceRejectReason reason) {
  switch (reason) {
    case GovernanceRejectReason::kNone:
      return "none";
    case GovernanceRejectReason::kInvalidProposal:
      return "invalid_proposal";
    case GovernanceRejectReason::kDomainOwnedByOther:
      return "domain_owned_by_other";
    case GovernanceRejectReason::kInsufficientPriority:
      return "insufficient_priority";
    default:
      return "unknown";
  }
}

}  // namespace ncos::core::contracts
