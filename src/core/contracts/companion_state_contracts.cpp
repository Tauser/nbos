#include "core/contracts/companion_state_contracts.hpp"

namespace ncos::core::contracts {

GovernanceHealth evaluate_governance_health(uint32_t allowed_total, uint32_t preempted_total,
                                            uint32_t rejected_total) {
  const uint32_t accepted = allowed_total + preempted_total;
  if (rejected_total == 0 && accepted == 0) {
    return GovernanceHealth::kUnknown;
  }

  if (rejected_total >= 3 && accepted == 0) {
    return GovernanceHealth::kContended;
  }

  return GovernanceHealth::kStable;
}

bool can_writer_mutate_domain(CompanionStateWriter writer, CompanionStateDomain domain) {
  switch (writer) {
    case CompanionStateWriter::kBootstrap:
      return domain == CompanionStateDomain::kStructural;
    case CompanionStateWriter::kRuntimeCore:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance;
    case CompanionStateWriter::kGovernanceCore:
      return domain == CompanionStateDomain::kTransient;
    case CompanionStateWriter::kEmotionService:
      return domain == CompanionStateDomain::kEmotional;
    case CompanionStateWriter::kAttentionService:
      return domain == CompanionStateDomain::kAttentional;
    case CompanionStateWriter::kPowerService:
      return domain == CompanionStateDomain::kEnergetic;
    case CompanionStateWriter::kInteractionService:
      return domain == CompanionStateDomain::kInteractional;
    default:
      return false;
  }
}

bool can_reader_observe_domain(CompanionStateReader reader, CompanionStateDomain domain) {
  switch (reader) {
    case CompanionStateReader::kRuntimeCore:
      return true;
    case CompanionStateReader::kBehaviorService:
      return domain != CompanionStateDomain::kStructural;
    case CompanionStateReader::kFaceService:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kEmotional ||
             domain == CompanionStateDomain::kAttentional ||
             domain == CompanionStateDomain::kInteractional;
    case CompanionStateReader::kMotionService:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kAttentional ||
             domain == CompanionStateDomain::kEnergetic ||
             domain == CompanionStateDomain::kInteractional;
    case CompanionStateReader::kVoiceService:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kEmotional ||
             domain == CompanionStateDomain::kAttentional ||
             domain == CompanionStateDomain::kEnergetic ||
             domain == CompanionStateDomain::kInteractional;
    case CompanionStateReader::kPowerService:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kEnergetic;
    case CompanionStateReader::kDiagnostics:
      return domain == CompanionStateDomain::kStructural || domain == CompanionStateDomain::kRuntime ||
             domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kEnergetic ||
             domain == CompanionStateDomain::kTransient;
    case CompanionStateReader::kCloudBridge:
      return domain == CompanionStateDomain::kRuntime || domain == CompanionStateDomain::kGovernance ||
             domain == CompanionStateDomain::kInteractional;
    default:
      return false;
  }
}

CompanionSnapshot redact_snapshot_for_reader(const CompanionSnapshot& source,
                                             CompanionStateReader reader) {
  CompanionSnapshot redacted = source;

  if (!can_reader_observe_domain(reader, CompanionStateDomain::kStructural)) {
    redacted.structural = CompanionStructuralState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kRuntime)) {
    redacted.runtime = CompanionRuntimeState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kGovernance)) {
    redacted.governance = CompanionGovernanceState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kEmotional)) {
    redacted.emotional = CompanionEmotionalState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kAttentional)) {
    redacted.attentional = CompanionAttentionalState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kEnergetic)) {
    redacted.energetic = CompanionEnergeticState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kInteractional)) {
    redacted.interactional = CompanionInteractionState{};
  }
  if (!can_reader_observe_domain(reader, CompanionStateDomain::kTransient)) {
    redacted.transient = CompanionTransientState{};
  }

  return redacted;
}

}  // namespace ncos::core::contracts
