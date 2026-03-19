#pragma once

#include "core/contracts/storage_runtime_contracts.hpp"

namespace ncos::interfaces::state {

enum class RuntimeConfigPersistenceStatus : uint8_t {
  kOk = 0,
  kNotFound,
  kUnavailable,
  kInvalidData,
  kIoError,
};

class RuntimeConfigPersistencePort {
 public:
  virtual ~RuntimeConfigPersistencePort() = default;
  virtual RuntimeConfigPersistenceStatus load(
      ncos::core::contracts::PersistedRuntimeConfigRecord* out_record) = 0;
  virtual RuntimeConfigPersistenceStatus save(
      const ncos::core::contracts::PersistedRuntimeConfigRecord& record) = 0;
  virtual RuntimeConfigPersistenceStatus reset() = 0;
};

}  // namespace ncos::interfaces::state
