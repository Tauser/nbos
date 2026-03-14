# Bring-up de Audio (INMP441 + MAX98357A)

Data: 2026-03-14

## Objetivo
Validar captura e reproducao basicas de audio no NC-OS com testes controlados.

## Pinagem usada
### INMP441 (RX)
- WS: GPIO14
- SCK: GPIO3
- SD: GPIO46

### MAX98357A (TX)
- LRC: GPIO41
- BCLK: GPIO42
- DIN: GPIO1

## Smoke tests implementados
1. Inicializacao de TX I2S para MAX98357A.
2. Inicializacao de RX I2S para INMP441.
3. Reproducao de 2 tons curtos (440Hz e 880Hz).
4. Captura de 1s com medicao de pico absoluto e quantidade de amostras.

## Como validar
- Build: `pio run -e esp32s3_dev`
- Upload: `pio run -e esp32s3_dev -t upload`
- Monitor serial: `pio device monitor -b 115200 -p <COM>`

Logs esperados (resumo):
- `Audio TX init: OK`
- `Audio RX init: OK`
- `Audio TX tones: OK`
- `Audio RX capture: OK (...)`

## Limitacoes desta etapa
- Nao implementa pipeline de voz (VAD, ASR, wake-word ou streaming).
- Nao valida qualidade/acuracia acustica, apenas bring-up funcional.
- Niveis de ganho e filtros ainda nao foram calibrados.
- GPIO3 permanece sensivel para cenarios de debug/serial.
- Se `peak=0`, validar selecao de canal L/R do INMP441, alimentacao do microfone e montagem.
- Driver I2S usado e legado nesta etapa; migracao para `i2s_std` fica para hardening posterior.
