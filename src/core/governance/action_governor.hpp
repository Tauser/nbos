#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"
#include "interfaces/governance/action_governance_port.hpp"

namespace ncos::core::governance {

struct ActionGovernanceStats {
  uint32_t allowed = 0;
  uint32_t preempted = 0;
  uint32_t rejected = 0;
  uint32_t debounced = 0;
};

class ActionGovernor final : public ncos::interfaces::governance::ActionGovernancePort {
 public:
  static constexpr uint32_t kSemanticDebounceWindowMs = 75;

  ncos::core::contracts::GovernanceDecision evaluate(
      const ncos::core::contracts::ActionProposal& proposal, uint64_t now_ms) override;

  bool release(ncos::core::contracts::ActionDomain domain, uint16_t owner_service) override;

  ncos::core::contracts::DomainLease lease(
      ncos::core::contracts::ActionDomain domain) const override;

  ActionGovernanceStats stats() const;

 private:
  struct SemanticSignal {
    bool initialized = false;
    ncos::core::contracts::ProposalOrigin origin = ncos::core::contracts::ProposalOrigin::kIntent;
    uint16_t requester_service = 0;
    ncos::core::contracts::CommandTopic action =
        ncos::core::contracts::CommandTopic::kMotionExecute;
    ncos::core::contracts::IntentTopic intent_context =
        ncos::core::contracts::IntentTopic::kAttendUser;
    uint64_t timestamp_ms = 0;
  };

  static size_t domain_index(ncos::core::contracts::ActionDomain domain);
  void expire_lease_if_needed(size_t domain_idx, uint64_t now_ms);
  bool is_semantically_debounced(size_t domain_idx,
                                 const ncos::core::contracts::ActionProposal& proposal,
                                 uint64_t now_ms) const;
  void remember_signal(size_t domain_idx, const ncos::core::contracts::ActionProposal& proposal,
                       uint64_t now_ms);

  ncos::core::contracts::DomainLease leases_[ncos::core::contracts::kActionDomainCount] = {};
  SemanticSignal last_signals_[ncos::core::contracts::kActionDomainCount] = {};
  ActionGovernanceStats stats_{};
};

}  // namespace ncos::core::governance
