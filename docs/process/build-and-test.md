# Build and test quickstart

## Build profiles
- Dev firmware build: `pio run -e esp32s3_dev`
- Prod firmware build: `pio run -e esp32s3_prod`

## Minimal validation track
- Host config tests: `pio test -e native`
- Firmware compile check: `pio run -e esp32s3_dev`

This stage intentionally keeps validation lightweight:
- verify centralized config and board profile integrity on host
- verify ESP-IDF firmware compiles for the target baseline

## Native test note for Windows + VS Code

On this project, `native` is more stable when run through a clean terminal/task than through the PlatformIO IDE "update/configure" flow.

Recommended path:
- use the VS Code tasks in `.vscode/tasks.json`
- or run `pio test -e native ...` from a clean PowerShell terminal
- avoid relying on the PlatformIO IDE project reconfiguration button before native tests

Reason:
- PlatformIO may reconfigure `esp32s3_dev` metadata and remove/reinstall host-side dependencies like `Unity`
- on Windows, stale `pio`, `python` or `program.exe` processes may lock `.pio/build`
- that leads to `WinError 5`, `WinError 145`, missing `unity_config.h`, or slow rebuild loops
