# Storage Persistence Base

## Objective

This stage establishes the official local persistence foundation before adaptive memory or richer state storage.

The goal is to keep persistence:
- local-first
- explicit
- small in scope
- safe to evolve

## Scope

This checkpoint covers only:
- storage BSP selection
- local blob persistence for small records
- isolated persisted runtime configuration

This checkpoint does **not** yet include:
- adaptive memory
- large file storage
- SD card filesystem usage
- automatic runtime boot overlay

## Structure

### Storage BSP

The active visual/platform storage baseline is centralized in:
- `src/drivers/storage/storage_platform_bsp.hpp`
- `src/drivers/storage/storage_platform_bsp.cpp`

It defines:
- active backend
- namespace/key for persisted runtime config
- size limits
- corrupt-record erase policy

For the current board baseline:
- backend: `NVS`
- namespace: `ncos`
- key: `runtime_cfg`

### Low-level local persistence

The low-level blob store is:
- `src/drivers/storage/local_persistence.hpp`
- `src/drivers/storage/local_persistence.cpp`

Responsibilities:
- initialize local persistence backend
- read/write/erase small blobs
- hide ESP NVS details from higher layers

Behavior:
- on ESP: uses `nvs_flash`
- on native tests: uses a fixed in-memory slot
- no dynamic allocation in the hot path

### Typed runtime config persistence

The typed persisted record is defined in:
- `src/core/contracts/storage_runtime_contracts.hpp`
- `src/core/contracts/storage_runtime_contracts.cpp`

The persistence port is:
- `src/interfaces/state/runtime_config_persistence_port.hpp`

The concrete store is:
- `src/drivers/storage/runtime_config_store.hpp`
- `src/drivers/storage/runtime_config_store.cpp`

Responsibilities:
- isolate load/save/reset of persisted runtime config
- sanitize and validate stored values
- erase corrupt records when policy allows

## Persisted fields

Current persisted runtime fields:
- `diagnostics_enabled`
- `cloud_sync_enabled`
- `cloud_bridge_enabled`
- `cloud_extension_enabled`
- `telemetry_enabled`
- `telemetry_export_off_device`
- `cloud_sync_interval_ms`
- `telemetry_interval_ms`

These are intentionally limited to a safe, product-relevant subset.

## Safety rules

The persistence base follows these rules:
- invalid records are rejected
- schema version is explicit
- intervals are clamped to safe bounds
- corrupt records may be erased by policy
- absence of a record is not treated as corruption

## Trade-offs

Accepted trade-offs for this stage:
- only one small persisted config record is supported
- SD remains deferred for future storage phases
- the runtime is not automatically overlaid from persistence yet

This keeps the checkpoint small, reviewable, and safe before the next memory/storage stages.
