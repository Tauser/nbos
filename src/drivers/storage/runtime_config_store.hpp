#pragma once

#include "config/system_config.hpp"
#include "core/contracts/storage_runtime_contracts.hpp"
#include "drivers/storage/local_persistence.hpp"
#include "interfaces/state/runtime_config_persistence_port.hpp"

namespace ncos::drivers::storage {

class RuntimeConfigStore final : public ncos::interfaces::state::RuntimeConfigPersistencePort {
 public:
  RuntimeConfigStore();
  explicit RuntimeConfigStore(LocalPersistence* persistence);

  ncos::interfaces::state::RuntimeConfigPersistenceStatus load(
      ncos::core::contracts::PersistedRuntimeConfigRecord* out_record) override;
  ncos::interfaces::state::RuntimeConfigPersistenceStatus save(
      const ncos::core::contracts::PersistedRuntimeConfigRecord& record) override;
  ncos::interfaces::state::RuntimeConfigPersistenceStatus reset() override;

  static ncos::core::contracts::PersistedRuntimeConfigRecord capture_runtime_config(
      const ncos::config::RuntimeConfig& runtime_config);
  static void apply_record_to_runtime_config(
      const ncos::core::contracts::PersistedRuntimeConfigRecord& record,
      ncos::config::RuntimeConfig* runtime_config);

 private:
  LocalPersistence owned_persistence_{};
  LocalPersistence* persistence_ = nullptr;
};

}  // namespace ncos::drivers::storage
