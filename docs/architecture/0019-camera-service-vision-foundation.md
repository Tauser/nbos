# 0019 - Camera Service e Foundation de Vision

## Decisão
Introduzimos `camera_service` em `src/services/vision` com contrato explícito (`interfaces/vision/camera_port.hpp`) e estado de runtime (`core/contracts/vision_runtime_contracts.hpp`).

## Motivação
- Evitar acesso de câmera espalhado em boot/runtime.
- Criar trilho incremental para ingestão de frames sem antecipar pipeline completo de vision.
- Preservar separação entre driver (`drivers/camera`) e orquestração (`services/vision`).

## Escopo desta etapa
- `CameraLocalPort` conectado ao `CameraBringup` existente.
- `CameraService` com init/tick e telemetria mínima de captura.
- Integração no `FirmwareEntrypoint` para participação no ciclo de runtime.

## Fora de escopo
- Pipeline de inferência/visão computacional.
- Armazenamento de buffer de frame no estado agregado.
- Decisão comportamental com base em visão.
