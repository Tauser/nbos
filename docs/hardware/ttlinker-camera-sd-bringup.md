# Bring-up de TTLinker, Camera e SD

Data: 2026-03-14

## Objetivo
Validar bring-up inicial dos blocos TTLinker, camera e SD sem subir subsistemas completos de motion, vision ou storage.

## Escopo desta etapa
- TTLinker: validacao inicial de risco de integracao com console UART na pinagem atual.
- Camera: validacao de disponibilidade do componente de camera no build atual.
- SD: validacao da interface SDMMC no firmware atual.

## Implementacao
### TTLinker
- Driver: `src/drivers/ttlinker/ttlinker_bringup.*`
- Pinagem congelada:
  - TX MCU: GPIO43
  - RX MCU: GPIO44
- Nesta etapa, o probe ativo foi **adiado** para evitar conflito com console serial em GPIO43/44 na configuracao atual.

### Camera (onboard)
- Driver: `src/drivers/camera/camera_bringup.*`
- Probe implementado:
  - detecta se `esp_camera.h` esta disponivel na toolchain atual
  - se disponivel, tentaria init e captura de 1 frame

### SD (onboard)
- Driver: `src/drivers/storage/sd_bringup.*`
- Bring-up atual:
  - inicializa host SDMMC e slot
  - valida disponibilidade da interface
  - card init/filesystem foi adiado para etapa dedicada de storage (evitar bloqueio prematuro no kickoff)

## Resultado medido em bancada (serial)
Logs capturados em 2026-03-14:
- `Camera probe: OK (component=false init=false frame=false size=0x0 len=0)`
- `SD probe: OK (interface=true card_init=false card_mb=0)`
- `TTLinker probe adiado: pinos 43/44 compartilhados com console na configuracao atual`

## Riscos e limitacoes
- TTLinker e console compartilham GPIO43/44 no perfil atual de debug: probe ativo pode quebrar telemetria serial.
- Camera nao esta habilitada no build atual (componente indisponivel), entao nao houve init/captura real nesta etapa.
- SD validou somente interface (host/slot); leitura/escrita em cartao fica para trilho de storage dedicado.

## Decisao desta etapa
- Baseline de hardware foi consolidada com status explicito de cada bloco.
- Nenhum subsistema completo foi criado (motion, vision, storage), respeitando o escopo.
