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
};

class ActionGovernor final : public ncos::interfaces::governance::ActionGovernancePort {
 public:
  ncos::core::contracts::GovernanceDecision evaluate(
      const ncos::core::contracts::ActionProposal& proposal, uint64_t now_ms) override;

  bool release(ncos::core::contracts::ActionDomain domain, uint16_t owner_service) override;

  ncos::core::contracts::DomainLease lease(
      ncos::core::contracts::ActionDomain domain) const override;

  ActionGovernanceStats stats() const;

 private:
  static size_t domain_index(ncos::core::contracts::ActionDomain domain);
  void expire_lease_if_needed(size_t domain_idx, uint64_t now_ms);

  ncos::core::contracts::DomainLease leases_[ncos::core::contracts::kActionDomainCount] = {};
  ActionGovernanceStats stats_{};
};

}  // namespace ncos::core::governance
