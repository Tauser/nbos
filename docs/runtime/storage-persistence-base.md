# Storage Persistence Base

## Objective

The persistence base now covers two official local records:
- runtime configuration
- persistent companion memory

The goal remains to keep storage:
- local-first
- explicit
- small in scope
- safe to evolve
- easy to reset and reason about
- resilient across interrupted writes

## Scope

This checkpoint now covers:
- storage BSP selection
- local blob persistence for small records
- isolated persisted runtime configuration
- isolated persisted companion memory for preferences, habits, and marked events
- official policy for retention, reset, erase, export, and import
- versioned envelope, integrity validation, and atomic write semantics for persisted records

This checkpoint still does **not** include:
- adaptive personality persistence
- short-session memory persistence
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
- whether runtime-config and companion-memory persistence are allowed

For the current board baseline:
- backend: `NVS`
- namespace: `ncos`
- legacy runtime-config key: `runtime_cfg`

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
- on native tests: uses fixed in-memory slots keyed by namespace and key
- no dynamic allocation in the hot path

### Typed stores

Typed persisted records and storage policy live in:
- `src/core/contracts/storage_runtime_contracts.hpp`
- `src/core/contracts/storage_runtime_contracts.cpp`

Concrete stores:
- `src/drivers/storage/runtime_config_store.hpp`
- `src/drivers/storage/runtime_config_store.cpp`
- `src/drivers/storage/persistent_companion_memory_store.hpp`
- `src/drivers/storage/persistent_companion_memory_store.cpp`

Responsibilities:
- isolate load/save/reset of each persisted record
- sanitize and validate stored values
- keep export/import strict and narrow
- use a dual-slot versioned envelope with checksum for local durability
- preserve the last-known-good record when a newer slot is corrupt or incomplete
- expose explicit recovery and profile-reset semantics

## Persisted data policy

### Officially persisted now

#### `runtime_config`

Fields:
- `diagnostics_enabled`
- `cloud_sync_enabled`
- `cloud_bridge_enabled`
- `cloud_extension_enabled`
- `telemetry_enabled`
- `telemetry_export_off_device`
- `cloud_sync_interval_ms`
- `telemetry_interval_ms`

Policy:
- local persistence: allowed
- retention: until overwrite or explicit reset
- integrity: schema checked, sanitized, and checksum protected in the local envelope
- erase on schema mismatch/corruption: allowed by policy
- erase on user reset: yes
- erase on factory reset: yes
- export/import: manual portable only

#### `persistent_companion_memory`

Fields are split into three explicit sections:
- preferences
- habits
- marked events

Policy:
- local persistence: allowed
- retention: until overwrite or explicit reset
- integrity: schema checked, sanitized, and checksum protected in the local envelope
- erase on schema mismatch/corruption: allowed by policy
- erase on user reset: yes
- erase on factory reset: yes
- export/import: blocked in this phase

### Officially not persisted now

The following remain volatile in this phase:
- short session memory
- recent interaction history inside the session snapshot
- adaptive personality biases
- telemetry buffers
- fault history

Policy for these classes:
- local persistence: blocked
- export/import: blocked
- retention: RAM only

This is intentional so the product does not silently turn short-lived context into historical storage.

## Persistent companion memory structure

The persistent companion-memory record is deliberately small and explicit.

### Preferences

Stable, user-facing tendencies that may outlive a session:
- `social_warmth_preference_percent`
- `response_energy_preference_percent`
- `stimulus_sensitivity_percent`
- `preferred_attention_channel`

### Habits

Bounded aggregate tendencies, not raw logs:
- `touch_engagement_affinity_percent`
- `repeat_engagement_affinity_percent`
- `calm_recovery_affinity_percent`
- `preferred_engagement_window`
- `reinforced_sessions`

### Marked events

A tiny set of notable anchors, not a timeline database:
- `last_user_event`
- `last_companion_event`
- `last_environment_event`

Each marked event stores:
- event kind
- semantic target
- channel
- salience percent
- reinforcement count
- last revision token

## Separation from session memory

The separation is now explicit:
- `CompanionSnapshot.session` remains volatile, short-lived, and runtime-facing
- `persistent_companion_memory` is a separate local storage record for stable preferences, bounded habits, and a few notable anchors

What stays in session memory:
- warm context
- last stimulus of the current session
- recent interaction context
- short-lived engagement heat
- retention windows tied to the active session

What may persist:
- small stable preferences
- bounded habit aggregates
- a few marked anchors worth carrying across reboot/reset boundaries until the user clears them

## Versioning, atomicity, and last-known-good

Both official persisted records now use a versioned local envelope with:
- envelope magic
- envelope version
- payload schema version
- payload size
- monotonic generation counter
- checksum over header + payload

Local write model:
- runtime config: `runtime_cfg_a` / `runtime_cfg_b`
- companion memory: `comp_mem_a` / `comp_mem_b`
- legacy fallback read remains only for `runtime_config`

Write rule:
- new data is written to the opposite slot from the current valid winner
- previous valid slot is left intact
- a corrupt newer slot never invalidates an older valid slot

Read rule:
- if both slots are valid, highest generation wins
- if the newest slot is corrupt, the older valid slot becomes the last-known-good
- if no companion-memory slot is valid after corruption, the store rebuilds the profile baseline

## Recovery and reset

Recovery is explicit for both stores:
- direct load: current slot is valid
- last-known-good fallback: older valid slot wins when the newer slot is corrupt
- profile reset: if no valid snapshot remains, the store rewrites a safe default profile

For companion memory, profile reset means:
- preferences return to neutral bounded defaults
- habits return to a clean baseline
- marked events are cleared

This reset is intentionally narrow:
- it resets only the persisted companion-memory record
- it does not change the fixed identity baseline of the companion
- it does not restore session memory, which remains runtime-only

## Trade-offs

Accepted trade-offs for this stage:
- persistent companion memory stores aggregate meaning, not raw logs
- export/import for companion memory is still blocked to avoid turning habits into a hidden sync surface too early
- session memory and adaptive personality remain volatile
- dual-slot durability is implemented for the official small persisted records only

This keeps the subsystem auditable, reviewable, and ready for later operational stages.
