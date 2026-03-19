# Storage Persistence Base

## Objective

This stage consolidates the official persistence policy before adaptive memory or richer state storage.

The goal is to keep storage:
- local-first
- explicit
- small in scope
- safe to evolve
- easy to reset and reason about

## Scope

This checkpoint covers only:
- storage BSP selection
- local blob persistence for small records
- isolated persisted runtime configuration
- official policy for retention, reset, erase, export, and import

This checkpoint does **not** yet include:
- adaptive memory persistence
- large file storage
- SD card filesystem usage
- automatic runtime boot overlay
- long-term telemetry or fault-history storage

## Structure

### Storage BSP

The active storage baseline is centralized in:
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

The typed persisted record and storage policy now live in:
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
- expose a strict manual export/import path for the portable record
- erase corrupt records when policy allows

## Persisted data policy

### Officially persisted now

Current persisted runtime fields:
- `diagnostics_enabled`
- `cloud_sync_enabled`
- `cloud_bridge_enabled`
- `cloud_extension_enabled`
- `telemetry_enabled`
- `telemetry_export_off_device`
- `cloud_sync_interval_ms`
- `telemetry_interval_ms`

Policy for `runtime_config`:
- local persistence: allowed
- retention: until overwrite or explicit reset
- integrity: schema checked and sanitized
- erase on schema mismatch/corruption: allowed by policy
- erase on user reset: yes
- erase on factory reset: yes
- export/import: manual portable only

### Officially not persisted now

The following remain volatile in this phase:
- short session memory
- recent interaction history
- adaptive personality biases
- telemetry buffers
- fault history

Policy for these classes:
- local persistence: blocked
- export/import: blocked
- retention: RAM only

This is intentional so the product does not silently turn short-lived context into historical storage.

## Retention and integrity rules

The persistence base now follows these rules:
- invalid records are rejected
- schema version is explicit
- intervals are clamped to safe bounds
- corrupt records may be erased by policy
- absence of a record is not treated as corruption
- imported records must already match the known schema before sanitation is accepted
- only the portable runtime-config subset can cross export/import boundaries

## Reset and erase rules

Reset behavior is now explicit:
- soft reboot: keeps local persisted runtime config
- user config reset: erases only the persisted runtime-config record
- factory reset: should erase the full local persistence namespace
- corrupt record handling: record may be erased automatically when the board policy allows it

Current implementation in this checkpoint:
- `RuntimeConfigStore::reset()` covers the user-facing config erase path
- local corruption erase is already supported through the BSP policy
- full factory-reset flow remains a higher-level operation for a later stage

## Export and import policy

Portable export/import is intentionally narrow:
- only `runtime_config` is exportable/importable
- transfer mode is manual, not automatic background sync
- exported data must stay small, typed, and schema-versioned
- import rejects unknown schema revisions
- import sanitizes safe ranges before applying them to runtime

This keeps product portability useful without turning storage into a hidden replication surface.

## Trade-offs

Accepted trade-offs for this stage:
- only one small persisted config record is supported
- SD remains deferred for future storage phases
- the runtime is not automatically overlaid from persistence yet
- factory reset is policy-defined here, but still needs its own orchestration step later
- telemetry and session continuity remain intentionally ephemeral for now

This keeps the checkpoint small, reviewable, and safe before the next memory/storage stages.
