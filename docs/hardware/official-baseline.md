# Baseline Oficial de Hardware - NC-OS

Status: Congelado
Data: 2026-03-14

## Escopo
Este documento define a baseline oficial de hardware (pinagem e alimentacao) para o NC-OS.
Mudancas de pinagem exigem justificativa explicita e registro arquitetural.

## Placa principal
- Freenove ESP32-S3-WROOM CAM N16R8
- Camera OV2640 e SD com pinagem onboard oficial da placa

## Pin mapping oficial
### Display ST7789
- MOSI/SDA -> GPIO21
- SCK/SCL  -> GPIO47
- DC       -> GPIO45
- RST      -> GPIO20
- CS       -> GND no prototipo atual (sem GPIO dedicado)

### Microfone INMP441
- WS  -> GPIO14
- SCK -> GPIO3
- SD  -> GPIO46

### Amplificador MAX98357A
- LRC  -> GPIO41
- BCLK -> GPIO42
- DIN  -> GPIO1

### Touch capacitivo
- TOUCH -> GPIO2

### TTLinker Mini / Servos
- TX MCU -> GPIO43
- RX MCU -> GPIO44

### MPU6050 (provisorio)
- SDA -> GPIO0
- SCL -> GPIO19

## Trilhos de alimentacao (baseline)
- Bateria Li-ion nominal: 3.7V (maximo 4.2V)
- Rail principal de potencia: 5V (gerenciado pelo SW6106 PD18W)
- Rail logico: 3.3V para ESP32-S3 e perifericos logicos

## Pinos sensiveis e provisoes
- GPIO0: sensivel a boot strap; uso atual no SDA do MPU e provisoriamente aceito.
- GPIO1/GPIO3: podem interagir com console/serial dependendo da configuracao de debug.
- GPIO3: pode afetar cenarios de debug/serial conforme configuracao de placa e firmware.
- GPIO46: entrada apenas no ESP32-S3; uso atual no SD do INMP441 esta alinhado.

## Regra executiva
- Nao alterar pinagem congelada sem justificativa tecnica explicita.
- Qualquer remapeamento de pino sensivel requer nota de trade-off e registro em ADR.
