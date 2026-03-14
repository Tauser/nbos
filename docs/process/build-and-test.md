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
