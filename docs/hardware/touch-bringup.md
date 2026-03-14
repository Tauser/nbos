# Bring-up do Touch Capacitivo (GPIO2)

Data: 2026-03-14

## Objetivo
Validar leitura local minima do touch capacitivo, sem acoplar a comportamento.

## Pinagem usada
- Touch capacitivo: GPIO2

## Implementacao de bring-up
- Inicializacao do touch via driver legado do ESP-IDF (`touch_pad_*`).
- Leitura bruta (`raw`) do canal de touch.
- Coleta de estabilidade em idle com estatistica:
  - minimo
  - maximo
  - media
  - faixa de ruido (`noise_span`)
  - delta inicial sugerido (`suggested_delta`)

## Resultado medido em hardware real
- Touch init: `OK`
- Touch raw (single): `3278660` (primeira leitura apos init; outlier esperado)
- Touch idle stats: `OK`
  - min: `61325`
  - max: `62324`
  - avg: `61674`
  - noise_span: `999`
  - suggested_delta: `3083`

## Sensibilidade inicial registrada
- Delta inicial sugerido: **3083** (max entre `3x noise_span` e `~5%` da media).
- Uso recomendado nesta etapa: apenas referencia de calibracao inicial.

## Limitacoes desta etapa
- Nao modela gesto/comportamento, apenas leitura local.
- Primeira leitura apos init pode ter outlier.
- Driver de touch legado esta deprecado no IDF 5.x (migracao para `driver/touch_sens.h` pendente).
- Sensibilidade precisa ser recalibrada apos definicao mecanica final do eletrodo e aterramento.
