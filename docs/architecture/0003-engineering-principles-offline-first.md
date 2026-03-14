# ADR-0003: Principios de Engenharia e Offline-First

- Status: Accepted
- Data: 2026-03-14

## Principios
1. Offline-first por padrao; cloud e extensao, nao dependencia.
2. Contratos explicitos acima de acoplamento implicito.
3. Evolucao incremental e testavel, sem "big bang" de features.
4. Nada de monolitos disfarcados.
5. Trade-offs devem ser documentados em decisoes relevantes.
6. Limites reais do ESP32-S3 guiam desenho de tarefas e memoria.

## Hardware congelado (baseline)
A baseline oficial de hardware (pinagem e alimentacao) esta em:
- `docs/hardware/official-baseline.md`

Resumo dos pontos mais criticos:
- Main board: Freenove ESP32-S3-WROOM CAM N16R8 (camera + SD onboard)
- Display ST7789: MOSI=21, SCK=47, DC=45, RST=20, CS=GND (prototipo)
- INMP441: WS=14, SCK=3, SD=46
- MAX98357A: LRC=41, BCLK=42, DIN=1
- Touch capacitivo: GPIO2
- TTLinker Mini: TX=43, RX=44
- MPU6050 (provisorio validavel): SDA=0, SCL=19

## Nota de risco
GPIO0 pode impactar boot/debug. A pinagem do MPU6050 permanece provisoria ate validacao eletrica e de boot. Qualquer alteracao deve ser aprovada por ADR.

## Consequencias
- Operacao robusta sem internet e comportamento previsivel em campo.
- Cloud bridge obrigatoriamente desacoplado, com fila/retry e fallback local.
