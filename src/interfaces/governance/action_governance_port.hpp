#pragma once

#include <stdint.h>

#include "core/contracts/action_governance_contracts.hpp"

namespace ncos::interfaces::governance {

class ActionGovernancePort {
 public:
  virtual ~ActionGovernancePort() = default;

  virtual ncos::core::contracts::GovernanceDecision evaluate(
      const ncos::core::contracts::ActionProposal& proposal, uint64_t now_ms) = 0;
  virtual bool release(ncos::core::contracts::ActionDomain domain, uint16_t owner_service) = 0;
  virtual ncos::core::contracts::DomainLease lease(
      ncos::core::contracts::ActionDomain domain) const = 0;
};

}  // namespace ncos::interfaces::governance
