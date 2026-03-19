# Critical Runtime BSP

## Objective

This stage centralizes three platform-critical runtime baselines:
- monotonic time base
- local power sensing baseline
- TTLinker transport baseline

It also makes reset reason explicit as platform metadata for boot/runtime decisions.

## Structure

### Monotonic clock
- `src/hal/platform/monotonic_clock.hpp`
- `src/hal/platform/monotonic_clock.cpp`

Provides a single monotonic source for:
- runtime ticks
- frame timing
- local sensing timing

Behavior:
- ESP: `esp_timer_get_time()`
- native: `std::chrono::steady_clock`

### Reset reason
- `src/hal/platform/reset_reason.hpp`
- `src/hal/platform/reset_reason.cpp`

Provides:
- normalized reset reason kind
- stable name for logs/boot notes
- `unstable_boot_context` flag for brownout/watchdog/panic classes

### Power sensing BSP
- `src/drivers/power/power_platform_bsp.hpp`
- `src/drivers/power/power_platform_bsp.cpp`

Provides the current board baseline for:
- sensor presence flags
- fallback local sample model

Current official baseline:
- no battery ADC wired in firmware yet
- no external power detector wired in firmware yet
- no thermal sensor wired in firmware yet
- local driver uses a deterministic fallback sample

### TTLinker transport BSP
- `src/drivers/ttlinker/ttlinker_transport_bsp.hpp`
- `src/drivers/ttlinker/ttlinker_transport_bsp.cpp`

Provides:
- UART wiring
- UART buffer/baud baseline
- console conflict status for the current build
- probe allowance flag

Current official baseline:
- TTLinker uses GPIO43/44
- active probe stays blocked when console conflicts with those pins

## Trade-offs

Accepted trade-offs for this stage:
- TTLinker conflict policy remains conservative
- reset reason is normalized but not yet used to change product behavior beyond observability
- power sensing remains fallback-based until ADC/charger telemetry is wired

This keeps the runtime foundation explicit without reopening subsystem architecture.
