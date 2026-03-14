# Bring-up da IMU MPU6050

Data: 2026-03-14

## Objetivo
Validar leitura minima da MPU6050, observar ruido/resposta basicos e registrar riscos da pinagem provisoria.

## Pinagem usada (provisoria)
- SDA: GPIO0
- SCL: GPIO19

## Implementacao de bring-up
- Driver dedicado em `src/drivers/imu/mpu6050_bringup.*`.
- Inicializacao I2C mestre (`I2C_NUM_0`, 400kHz).
- Validacao de presenca via `WHO_AM_I` (`0x68`).
- Configuracao basica da MPU6050:
  - wake-up (`PWR_MGMT_1 = 0x00`)
  - sample rate/divisor, filtro e full-scale default.
- Leitura minima de amostra bruta:
  - acelerometro: `ax/ay/az`
  - giroscopio: `gx/gy/gz`
- Janela de ruido com min/max/media por eixo.
- Medicao de resposta por pico de delta entre amostras consecutivas.

## Validacao executada nesta etapa
- Build local: `pio run -e esp32s3_dev` -> **OK**.
- Upload para hardware: `pio run -e esp32s3_dev -t upload` (COM12) -> **OK**.
- Captura serial automatizada de logs IMU: **bloqueada** (porta COM12 em `Acesso negado`, processo externo segurando serial).

## Logs esperados no smoke test
- `IMU init: OK`
- `IMU sample ax=... ay=... az=... gx=... gy=... gz=...`
- `IMU noise window: OK (...)`
- `IMU noise spans: ax=... ay=... az=... gx=... gy=... gz=...; avg ...`
- `IMU response peak delta: OK (...)`

## Riscos da pinagem provisoria
- `GPIO0` e pino de bootstrap no ESP32-S3; ruido/estado incorreto no boot pode afetar modo de inicializacao.
- I2C em `GPIO0` aumenta risco de interferencia com debug/bring-up em cenarios de bancada.
- `GPIO19` e estavel para I2C, mas o par atual deve ser tratado como temporario ate validacao longa de boot e ruidez.

## Trade-offs desta etapa
- Mantivemos pinagem congelada para continuidade do projeto, com risco assumido documentado.
- Bring-up focado em leitura local e diagnostico bruto; nao ha sensor fusion nem filtros avancados.
- A metrica de ruido/resposta e suficiente para baseline inicial, nao para calibracao final.

## Proximo passo recomendado
- Capturar uma sessao real de logs em bancada com placa parada e depois com movimento manual curto.
- Registrar valores medidos no proprio documento para congelar baseline de ruido/resposta da IMU.
