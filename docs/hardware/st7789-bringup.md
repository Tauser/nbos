# Bring-up ST7789 (NC-OS)

Data: 2026-03-14

## Objetivo
Validar inicializacao minima do display ST7789 em hardware real.

## Pinagem confirmada
- MOSI/SDA: GPIO21
- SCK/SCL: GPIO47
- DC: GPIO45
- RST: GPIO20
- CS: GND (sem GPIO dedicado, `-1` no driver)

## Smoke test implementado
1. Inicializa display via LovyanGFX.
2. Renderiza telas cheia em sequencia: vermelho, verde, azul.
3. Exibe texto final: `NC-OS ST7789` e `SMOKE OK`.

## Validacao executada
- Build: `pio run -e esp32s3_dev` (sucesso)
- Upload: `pio run -e esp32s3_dev -t upload` (sucesso em COM12)
- Log serial esperado:
  - `NCOS: Display smoke test completed`

## Observacoes
- Esta etapa nao implementa Face Engine.
- Esta validacao cobre apenas bring-up grafico minimo.
