# Bring-up ST7789 (NC-OS)

Data: 2026-03-18

## Objetivo
Validar inicializacao minima do display ST7789 em hardware real e consolidar as restricoes reais do painel usado no produto.

## Pinagem confirmada
- MOSI/SDA: GPIO21
- SCK/SCL: GPIO47
- DC: GPIO45
- RST dedicado por GPIO: nao
- RST compartilhado com `EN` da ESP32-S3
- CS: GND (sem GPIO dedicado, `-1` no driver)

## Bring-up minimo valido
1. Inicializa display via LovyanGFX.
2. Mantem `invert=true`.
3. Usa primitivas diretas como caminho principal.
4. Evita depender de sprite-window no caminho de produto.

## Observacoes consolidadas
- O painel ficou limpo em teste estatico de polaridade.
- Houve artefatos sob movimento com `full redraw`.
- `dirty rect` pequeno ficou melhor que `full redraw` amplo.
- `sprite-window` apresentou flicker e nao deve ser caminho padrao.

## Referencias
- Perfil consolidado: `docs/hardware/display-panel-profile.md`
- Diagnostico de pipeline: `docs/hardware/display-diagnostics.md`
