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

}  // namespace ncos::core::contracts
