# NC-OS

Firmware do **NC-OS**, um robô desktop companion premium baseado em ESP32-S3.

## Visão do produto
- Operação **offline-first** (funciona localmente por padrão)
- Nuvem como extensão opcional (`Cloud Bridge`), nunca dependência principal
- Evolução incremental com arquitetura profissional e testável
- Identidade visual e comportamental consistente (Face/Emotion/Behavior/Motion)

## Hardware congelado (baseline)
- Placa: Freenove ESP32-S3-WROOM CAM N16R8 (câmera + SD onboard)
- Display: ST7789 2" IPS (LovyanGFX)
- Câmera: OV2640 (onboard)
- Servos: 2x Feetech SCS0009 + FE-TTLinker-Mini
- IMU: MPU6050 (GY-521)
- Áudio out: MAX98357A + alto-falante 4 ohms 3W
- Microfone: INMP441
- Touch: fita de cobre capacitiva
- Energia: SW6106 PD18W + bateria 3.7V 3000mAh

Pinagem de referência e riscos (ex.: MPU em GPIO0) estão documentados em `docs/architecture/`.

## Stack-base
- Framework principal: **ESP-IDF**
- Build/orquestração: **PlatformIO**
- Gráficos: **LovyanGFX**

Decisões oficiais: `docs/architecture/0001..0003`.

## Arquitetura em camadas
A estrutura oficial fica em `src/`:
- `app`, `core`, `interfaces`, `drivers`, `hal`, `services`, `ai`, `models`, `utils`, `config`

Princípios:
- sem monólitos
- contratos explícitos
- hardware desacoplado da regra de negócio
- renderer não decide semântica

## Objetivos da fundação
1. Build reproduzível
2. Configuração centralizada
3. Contratos base por camada
4. Gate arquitetural do Face Engine antes de F1

## Fluxo de desenvolvimento
- Política de commits: `docs/process/commit-policy.md`
- Decisões arquiteturais: `docs/architecture/`
- Regras para agentes: `AGENTS.md`
