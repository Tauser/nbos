#include "drivers/storage/storage_platform_bsp.hpp"

#include "config/pins/board_pins.hpp"

namespace {

const ncos::drivers::storage::StoragePlatformBsp StorageBsp = {
    ncos::config::kBoardName,
    ncos::drivers::storage::StorageBackendId::kNvs,
    "ncos",
    "runtime_cfg",
    {
        128U,
        true,
        true,
        true,
    },
};

}  // namespace

namespace ncos::drivers::storage {

const StoragePlatformBsp& active_storage_platform_bsp() {
  return StorageBsp;
}

}  // namespace ncos::drivers::storage
