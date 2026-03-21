# Manual Tecnico Do Sistema NC-OS

## 1. Visao Geral Do Sistema

### 1.1 Objetivo do sistema

Status: observado

O NC-OS e um firmware embarcado para um robo desktop companion premium. No codigo atual, isso aparece como um sistema orientado a:

- bootstrap controlado de hardware
- runtime principal por ticks
- estado central do companion
- servicos especializados por subsistema
- contratos explicitos entre dominio, governanca e infraestrutura
- operacao offline-first com cloud como extensao opcional

Arquivos-base que confirmam isso:

- [main.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\main.cpp)
- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [system_manager.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\system_manager.cpp)
- [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)
- [system_config.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\system_config.hpp)

### 1.2 Arquitetura geral observada

Status: observado

Camadas principais encontradas em `src/`:

- `app`: bootstrap, ciclo de vida e wiring
- `core`: contratos, estado, bus, scheduler, governanca, runtime
- `services`: orquestracao de comportamento, percepcao, emocao, face, motion, cloud, update, telemetria
- `drivers`: implementacoes concretas de hardware e storage
- `interfaces`: portas entre servicos e infraestrutura
- `config`: perfil de build, pinagem, configuracoes globais
- `models`: tipos compactos de dominio, principalmente emocao e face

### 1.3 Responsabilidades macro por camada

Status: observado

- `app` sobe o sistema e dirige o loop principal
- `core` concentra o estado canonico e a arbitragem
- `services` traduzem sinais, produzem propostas e controlam saidas
- `drivers` acessam hardware real
- `interfaces` impedem acoplamento direto entre regra de produto e periferico
- `config` define perfil de build, watchdogs, tempo de probes e parametros de runtime

### 1.4 Leitura arquitetural curta

Status: inferido com alta confianca

O formato real do sistema hoje e:

- `FirmwareEntrypoint` como composicao manual do sistema
- `SystemManager` como nucleo de runtime
- `CompanionStateStore` como blackboard central
- servicos como produtores e consumidores do snapshot do companion
- `ActionGovernor` como arbitro de decisoes ativas
- stack facial moderna com estado governado, nao apenas sprites ou presets soltos

## 2. O Que Foi Observado Vs Inferido

### 2.1 Confirmacoes observadas

- Existe um unico ponto de entrada de firmware em [main.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\main.cpp)
- O loop principal chama `run()` uma vez e `tick()` continuamente
- `FirmwareEntrypoint` instancia diretamente `SystemManager` e os servicos
- `BootFlow` executa bring-up de hardware antes do runtime
- `SystemManager` contem scheduler, event bus, governanca, state store, health, fault history e safe mode
- `CompanionStateStore` deriva estado do produto, memoria curta e adaptacao
- `BehaviorService`, `RoutineService`, `EmotionService`, `PerceptionService`, `VoiceService`, `PowerService`, `MotionService` e `FaceService` estao todos ligados no loop
- A face em runtime e controlada por `FaceGraphicsPipeline`
- O display real da face usa `DisplayDriver` baseado em LovyanGFX e painel ST7789

### 2.2 Itens explicitamente nao observados

- `AppFactory` nao foi encontrado no codigo nem em `docs/`
- uma classe unica de scheduler global fora de `SchedulerBase` nao foi observada
- um subsistema facial legado ativo, separado do `FaceGraphicsPipeline`, nao foi confirmado nesta leitura

### 2.3 Inferencias com alta confianca

- o sistema usa execucao cooperativa baseada em tick principal e nao threads independentes por servico
- o companion inteiro depende da integridade do snapshot central
- o semantic bus funciona como camada secundaria de injecao semantica, nao como motor primario de runtime
- a stack facial nova ja esta acima do limiar de "preset simples", porque inclui ownership por camada, clip, gaze, modulation, safety, dirty rect e fallback visual

### 2.4 Inferencias com baixa confianca

- podem existir componentes de face antigos ainda mantidos no repositorio, mas nao confirmados como ativos no caminho principal
- a figura de `AppFactory` pode existir como intencao de arquitetura futura, mas nao como implementacao real

## 3. Fluxo De Inicializacao

### 3.1 Ponto de entrada

Status: observado

Arquivo:

- [main.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\main.cpp)

Fluxo:

- `app_main()` cria `static FirmwareEntrypoint g_entrypoint`
- chama `g_entrypoint.run()`
- entra em loop infinito
- em cada iteracao chama `g_entrypoint.tick()`
- espera 20 ms com `vTaskDelay`

Conclusao:

- o tick base do sistema, observado no ponto de entrada, e de 20 ms

### 3.2 Bootstrap principal

Status: observado

Arquivos:

- [firmware_entrypoint.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.hpp)
- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)

Responsabilidade:

- wiring completo do runtime
- sequenciamento de inicializacao
- execucao do tick principal

Campos importantes de `FirmwareEntrypoint`:

- `lifecycle_`
- `system_manager_`
- instancias de todos os servicos principais
- timers de observabilidade como `next_polish_review_ms_`
- flags de fault report para energia e OTA

### 3.3 Validacao de configuracao

Status: observado

Antes de subir runtime, `FirmwareEntrypoint::run()`:

- verifica `ncos::config::kConfigReady`
- loga `build_profile`
- loga `board_name`
- loga dados da board em `kGlobalConfig.board`
- loga versao de taxonomia semantica

Arquivos relevantes:

- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [system_config.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\system_config.hpp)
- [build_profile.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\build_profile.hpp)

### 3.4 Lifecycle de boot

Status: observado

Arquivos:

- [system_lifecycle.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\lifecycle\system_lifecycle.hpp)
- [system_lifecycle.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\lifecycle\system_lifecycle.cpp)

Estados observados:

- `kPowerOn`
- `kBooting`
- `kRunning`
- `kDegraded`
- `kFaulted`

Transicao observada:

- `start_boot()` coloca em `kBooting`
- `finish_boot(required_failures, warnings)` fecha em `running`, `degraded` ou `faulted`
- `mark_fault()` move para `faulted`

### 3.5 BootFlow

Status: observado

Arquivos:

- [boot_flow.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\boot_flow.hpp)
- [boot_flow.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\boot_flow.cpp)

Etapas observadas na execucao:

- `step_config_gate()`
- `step_display()`
- `step_audio()`
- `step_touch()`
- `step_imu()`
- `step_camera()`
- `step_sd()`
- `step_ttlinker()`

Detalhes relevantes:

- display e tratado como etapa required
- SD no perfil dev e explicitamente adiado
- audio, touch, imu, camera e TTLinker podem gerar warning sem travar boot total
- `step_display()` usa `try_display_once()` em task dedicada do FreeRTOS para o bring-up

### 3.6 Inicializacao do SystemManager

Status: observado

Arquivo:

- [system_manager.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\system_manager.cpp)

`initialize(...)` faz:

- armazena ponteiros de lifecycle e config
- bind do `CompanionStateAxis`
- ligacao entre `EventBusV2`, `CompanionStateStore` e `ActionGovernor`
- prepara o runtime para ser iniciado

`start(now)` faz:

- reset de health
- registro de tarefas do `SchedulerBase`
- publicacao de evento `kSystemObservation`
- inicializacao estrutural e de personalidade do estado do companion
- sincronizacao do estado de runtime para dentro do snapshot central

### 3.7 Carga de memoria persistente

Status: observado

Ainda em `FirmwareEntrypoint::run()`:

- instancia `PersistentCompanionMemoryStore`
- chama `load_with_recovery(...)`
- se status for `kOk`, converte o record persistido em `CompanionPersistentMemorySignal`
- injeta o sinal no `SystemManager`

### 3.8 Ordem observada de inicializacao dos servicos

Status: observado

Sequencia encontrada em [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp):

- `audio_service_`
- `behavior_service_`
- `routine_service_`
- `emotion_service_`
- `voice_service_`
- `touch_service_`
- `imu_service_`
- `camera_service_`
- `perception_service_`
- `power_service_`
- `update_service_`
- `cloud_bridge_service_`
- `telemetry_service_`
- avaliacao de politica OTA de boot
- `motion_service_`
- `led_service_`
- `face_service_`

### 3.9 Runtime readiness

Status: observado

Arquivos:

- [runtime_readiness.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\runtime_readiness.hpp)
- [runtime_readiness.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\runtime_readiness.cpp)

Campos avaliados:

- `config_valid`
- `board_profile_bound`
- `lifecycle_allows_runtime`
- `runtime_initialized`
- `runtime_started`
- `scheduler_has_minimum_tasks`
- `safe_mode_inactive`
- `no_faults_recorded`
- `governance_stable`

Saidas:

- `ready`
- `conditionally_ready`
- `not_ready`

## 4. Fluxo De Runtime

### 4.1 Tick principal

Status: observado

Arquivo:

- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)

Sequencia de alto nivel do tick:

- coleta `now_ms`
- atualiza `SystemManager`
- atualiza sensores primarios
- roda percepcao
- roda voz
- roda comportamento
- roda rotina
- roda emocao
- roda energia
- roda update/OTA
- roda face
- roda cloud
- roda telemetria
- roda motion
- roda observabilidade cruzada
- roda LED

### 4.2 Scheduler interno

Status: observado

Arquivos:

- [scheduler_base.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\scheduler_base.hpp)
- [scheduler_base.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\scheduler_base.cpp)

Comportamento real:

- registra ate 8 tarefas
- cada tarefa tem `period_ms`, `callback`, `context`, `next_run_ms`
- `tick(now_ms)` executa callbacks vencidos e avanca `next_run_ms`

### 4.3 Event bus semantico

Status: observado

Arquivos:

- [event_bus.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\messaging\event_bus.hpp)
- [event_bus.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\messaging\event_bus.cpp)
- [companion_state_axis.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\companion_state_axis.cpp)

Lanes observadas:

- `events`
- `commands`
- `intents`
- `reactions`

### 4.4 Papel do CompanionStateAxis

Status: observado

Mapeamentos observados:

- `EventMessage` gera `CompanionAttentionalSignal`
- `CommandMessage` gera `ActionProposal` e passa pela governanca
- `IntentMessage` pode gerar `CompanionEnergeticSignal` ou `CompanionInteractionSignal`
- `ReactionMessage` gera `CompanionEmotionalSignal`

### 4.5 Fluxo de entradas

Status: observado

Entradas diretas no tick:

- audio
- touch
- imu
- camera

Entradas derivadas:

- percepcao fusiona audio, touch e camera
- voz extrai trigger e atividade de fala do audio
- power extrai modo energetico e guardas
- memoria persistente entra como baseline adaptativo

### 4.6 Fluxo do snapshot central

Status: observado

O caminho canonico e:

- servico produz sinal
- `SystemManager` ingere
- `CompanionStateStore` atualiza dominio correspondente
- `CompanionStateStore` recalcula derivados
- servicos seguintes consomem `CompanionSnapshot`

## 5. Mapa Dos Modulos Principais

### 5.1 Modulo de bootstrap e ciclo de vida

Arquivos principais:

- [main.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\main.cpp)
- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [boot_flow.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\boot_flow.cpp)
- [system_lifecycle.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\lifecycle\system_lifecycle.cpp)

Classes principais:

- `FirmwareEntrypoint`
- `BootFlow`
- `SystemLifecycle`

### 5.2 Modulo de configuracao

Arquivos principais:

- [system_config.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\system_config.hpp)
- [build_profile.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\build_profile.hpp)

### 5.3 Modulo de runtime core

Arquivos principais:

- [system_manager.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\system_manager.cpp)
- [scheduler_base.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\scheduler_base.cpp)
- [runtime_readiness.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\runtime_readiness.cpp)

### 5.4 Modulo de estado do companion

Arquivos principais:

- [companion_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.hpp)
- [companion_state_contracts.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.cpp)
- [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)

### 5.5 Modulo de governanca

Arquivos:

- [action_governor.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\governance\action_governor.hpp)
- [action_governor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\governance\action_governor.cpp)

### 5.6 Modulos de comportamento

Arquivos:

- [behavior_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\behavior\behavior_service.cpp)
- [routine_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\routine\routine_service.cpp)
- [emotion_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\emotion\emotion_service.cpp)

### 5.7 Modulos de sensoriamento e percepcao

Arquivos principais:

- [audio_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\audio\audio_service.hpp)
- [touch_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\sensing\touch_service.hpp)
- [imu_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\sensing\imu_service.hpp)
- [camera_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\vision\camera_service.hpp)
- [perception_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\vision\perception_service.cpp)
- [voice_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\voice\voice_service.cpp)

### 5.8 Modulos de saida e extensao

Arquivos principais:

- [motion_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\motion\motion_service.cpp)
- [face_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_service.cpp)
- [power_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\power\power_service.hpp)
- [cloud_bridge_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\cloud\cloud_bridge_service.hpp)
- [update_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\update\update_service.hpp)
- [telemetry_service.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\telemetry\telemetry_service.hpp)

## 6. Area Especial Da Face / Expressoes / Animacoes

### 6.1 Visao geral da stack facial

Status: observado

O caminho facial real do runtime e:

- [face_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_service.cpp)
- [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
- [face_render_state_contracts.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.cpp)
- [face_frame_composer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_frame_composer.cpp)
- [face_display_renderer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_display_renderer.cpp)
- [display_driver.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\display\display_driver.cpp)

Interpretacao observada:

- `FaceService` e so fachada
- o orquestrador real e `FaceGraphicsPipeline`
- o estado facial oficial e `FaceRenderState`
- a face e governada por camadas com ownership
- o display nao desenha diretamente o estado, ele desenha um `FaceFrame`

### 6.2 Contrato central da face

Arquivos:

- [face_render_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.hpp)
- [face_render_state_contracts.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.cpp)
- [face_models.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\models\face\face_models.hpp)

Tipos mais importantes:

- `FaceRenderStateVersion`
- `FaceCompositionMode`
- `FaceRenderSafetyMode`
- `FaceLayerOwnerRole`
- `FaceShapeGeometry`
- `FaceLayerPolicy`
- `FaceLayerOwnership`
- `FaceCompositionState`
- `FaceRenderState`

Campos relevantes de `FaceRenderState`:

- `version`
- `revision`
- `updated_at_ms`
- `safety_mode`
- `preset`
- `geometry`
- `eyes`
- `lids`
- `mouth`
- `brows`
- `composition`
- `active_trace_id`
- `owner_service`

### 6.3 Camadas faciais observadas

- `kBase`
- `kBlink`
- `kGaze`
- `kModulation`
- `kTransient`
- `kClip`

Politicas observadas:

- `kBase` prioridade default 2
- `kBlink` prioridade default 6
- `kGaze` prioridade default 5
- `kModulation` prioridade default 4
- `kTransient` prioridade default 7
- `kClip` prioridade default 8

### 6.4 FaceCompositor

Arquivos:

- [face_compositor.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_compositor.hpp)
- [face_compositor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_compositor.cpp)

Responsabilidade:

- arbitrar claims de camada
- manter `hold_until_ms`
- manter `cooldown_until_ms`
- validar se um writer pode escrever numa layer

### 6.5 Presets oficiais e exploratorios

Arquivos:

- [face_preset_library.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_preset_library.hpp)
- [face_preset_library.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_preset_library.cpp)

Presets oficiais observados:

- `kCoreNeutral`
- `kCoreAttend`
- `kCoreCalm`
- `kCoreCurious`
- `kCoreLock`

### 6.6 Input multimodal para a face

Arquivos:

- [face_multimodal_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_multimodal_contracts.hpp)
- [face_multimodal_contracts.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_multimodal_contracts.cpp)

### 6.7 FaceMultimodalSync

Arquivos:

- [face_multimodal_sync.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_multimodal_sync.hpp)
- [face_multimodal_sync.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_multimodal_sync.cpp)

### 6.8 Gaze controller

Arquivos:

- [face_gaze_controller.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_gaze_controller.hpp)
- [face_gaze_controller.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_gaze_controller.cpp)
- [face_gaze_target.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\models\face\face_gaze_target.hpp)

### 6.9 Clip player

Arquivos:

- [face_clip_player.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_clip_player.hpp)
- [face_clip_player.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_clip_player.cpp)
- [face_clip.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\models\face\face_clip.hpp)

### 6.10 Geometria facial

Arquivos:

- [face_geometry.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_geometry.hpp)
- [face_geometry.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_geometry.cpp)

### 6.11 Frame composer

Arquivos:

- [face_frame.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_frame.hpp)
- [face_frame_composer.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_frame_composer.hpp)
- [face_frame_composer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_frame_composer.cpp)

### 6.12 Renderizacao para display

Arquivos:

- [face_display_renderer.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_display_renderer.hpp)
- [face_display_renderer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_display_renderer.cpp)
- [display_pipeline_analysis.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\display\display_pipeline_analysis.hpp)
- [display_pipeline_analysis.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\display\display_pipeline_analysis.cpp)

### 6.13 Driver de display e BSP

Arquivos:

- [display_runtime.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\display\display_runtime.cpp)
- [display_driver.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\display\display_driver.hpp)
- [display_driver.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\display\display_driver.cpp)
- [display_platform_bsp.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\display\display_platform_bsp.cpp)

### 6.14 Motion safety facial

Arquivos:

- [face_motion_safety.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_motion_safety.hpp)
- [face_motion_safety.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_motion_safety.cpp)

### 6.15 Visual fallback

Arquivos:

- [face_visual_fallback.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_visual_fallback.hpp)
- [face_visual_fallback.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_visual_fallback.cpp)
- [face_tooling.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_tooling.hpp)
- [face_tooling.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_tooling.cpp)

### 6.16 Pipeline completo da face

Status: observado

Em [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp), o fluxo observado por tick e:

- verificar modo de diagnostico do display
- `compositor_.tick(now_ms)`
- escolher preset oficial por contexto atual
- opcionalmente iniciar clip de assinatura idle
- `clip_player_.tick(...)`
- agendar alvo de gaze
- `gaze_controller_.tick(...)`
- `multimodal_sync_.apply(...)`
- `apply_face_motion_safety(...)`
- `composer_.compose(...)`
- `renderer_.render(...)`
- coletar telemetria da face
- avaliar entrada/saida de fallback visual
- atualizar `preview_snapshot_`
- atualizar `motion_signal_`

### 6.17 Quais metodos realmente mudam a face na tela

- `apply_official_face_preset(...)`
- `FaceClipPlayer::tick(...)`
- `FaceGazeController::tick(...)`
- `FaceMultimodalSync::apply(...)`
- `apply_face_motion_safety(...)`
- `apply_face_visual_fallback(...)`
- `make_face_geometry_layout(...)`
- `FaceFrameComposer::compose(...)`
- `analyze_render_plan(...)`
- `FaceDisplayRenderer::render(...)`

### 6.18 Quais arquivos pertencem claramente a stack nova

- [face_render_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.hpp)
- [face_compositor.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_compositor.hpp)
- [face_clip_player.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_clip_player.hpp)
- [face_gaze_controller.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_gaze_controller.hpp)
- [face_multimodal_sync.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_multimodal_sync.hpp)
- [face_motion_safety.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_motion_safety.hpp)
- [face_visual_fallback.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_visual_fallback.hpp)
- [face_frame_composer.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_frame_composer.hpp)
- [face_display_renderer.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_display_renderer.hpp)

### 6.19 Estado da face e movimento

Status: observado

O elo face -> motion aparece em:

- `FaceGraphicsPipeline::motion_signal()`
- `make_motion_companion_signal(...)` em [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- `motion_service_.update_face_signal(...)` no tick principal

## 7. Mapa Dos Arquivos Mais Importantes

### 7.1 Prioridade maxima para entender o sistema

- [main.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\main.cpp)
- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [boot_flow.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\boot_flow.cpp)
- [system_manager.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\system_manager.cpp)
- [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)
- [companion_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.hpp)

### 7.2 Prioridade maxima para entender comportamento

- [behavior_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\behavior\behavior_service.cpp)
- [routine_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\routine\routine_service.cpp)
- [emotion_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\emotion\emotion_service.cpp)
- [perception_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\vision\perception_service.cpp)
- [voice_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\voice\voice_service.cpp)
- [action_governor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\governance\action_governor.cpp)

### 7.3 Prioridade maxima para entender face

- [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
- [face_render_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.hpp)
- [face_compositor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_compositor.cpp)
- [face_preset_library.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_preset_library.cpp)
- [face_multimodal_sync.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_multimodal_sync.cpp)
- [face_gaze_controller.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_gaze_controller.cpp)
- [face_clip_player.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_clip_player.cpp)
- [face_motion_safety.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_motion_safety.cpp)
- [face_frame_composer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_frame_composer.cpp)
- [face_display_renderer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_display_renderer.cpp)
- [display_pipeline_analysis.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\display\display_pipeline_analysis.cpp)
- [display_driver.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\display\display_driver.cpp)

## 8. Mapa Dos Metodos Mais Importantes

### 8.1 Sistema

- `app_main`
- `FirmwareEntrypoint::run`
- `FirmwareEntrypoint::tick`
- `BootFlow::execute`
- `SystemManager::initialize`
- `SystemManager::start`
- `SystemManager::tick`

### 8.2 Estado do companion

- `CompanionStateStore::initialize`
- `CompanionStateStore::ingest_runtime`
- `CompanionStateStore::ingest_emotional`
- `CompanionStateStore::ingest_attentional`
- `CompanionStateStore::ingest_energetic`
- `CompanionStateStore::ingest_interactional`
- `CompanionStateStore::ingest_governance_decision`
- `CompanionStateStore::ingest_persistent_memory_signal`
- `CompanionStateStore::snapshot_for`
- `CompanionStateStore::refresh_session_memory`
- `CompanionStateStore::refresh_adaptive_personality`
- `CompanionStateStore::refresh_derived_runtime_state`

### 8.3 Comportamento e rotina

- `BehaviorService::tick`
- `BehaviorService::propose_energy_protect`
- `BehaviorService::propose_alert_scan`
- `BehaviorService::propose_attend_user`
- `RoutineService::tick`
- `EmotionService::tick`

### 8.4 Face

- `FaceGraphicsPipeline::initialize`
- `FaceGraphicsPipeline::tick`
- `FaceGraphicsPipeline::motion_signal`
- `FaceCompositor::request_layer`
- `FaceCompositor::release_layer`
- `FaceClipPlayer::play`
- `FaceClipPlayer::tick`
- `FaceGazeController::set_target`
- `FaceGazeController::tick`
- `FaceMultimodalSync::apply`
- `make_face_geometry_layout`
- `FaceFrameComposer::compose`
- `analyze_render_plan`
- `FaceDisplayRenderer::render`
- `apply_face_motion_safety`
- `apply_face_visual_fallback`

## 9. Ordem Recomendada De Estudo

1. Ler [main.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\main.cpp)
2. Ler [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
3. Ler [boot_flow.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\boot_flow.cpp)
4. Ler [system_manager.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\system_manager.cpp)
5. Ler [companion_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.hpp)
6. Ler [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)
7. Ler [action_governor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\governance\action_governor.cpp)
8. Ler `behavior`, `routine`, `emotion`, `perception`, `voice`
9. Entrar na face por [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
10. Percorrer `preset -> clip -> gaze -> multimodal -> safety -> compose -> render`

## 10. Pontos Criticos De Manutencao

- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [system_manager.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\system_manager.cpp)
- [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)
- [action_governor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\governance\action_governor.cpp)
- [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
- [face_render_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.hpp)
- [face_display_renderer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_display_renderer.cpp)
- [system_config.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\system_config.hpp)

## 11. Resumo Operacional Final

Arquivos obrigatorios para entender o sistema:

- [main.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\main.cpp)
- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [boot_flow.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\boot_flow.cpp)
- [system_manager.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\runtime\system_manager.cpp)
- [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)
- [action_governor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\governance\action_governor.cpp)

Arquivos obrigatorios para entender a face:

- [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
- [face_render_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.hpp)
- [face_compositor.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_compositor.cpp)
- [face_preset_library.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_preset_library.cpp)
- [face_multimodal_sync.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_multimodal_sync.cpp)
- [face_gaze_controller.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_gaze_controller.cpp)
- [face_clip_player.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_clip_player.cpp)
- [face_frame_composer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_frame_composer.cpp)
- [face_display_renderer.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_display_renderer.cpp)
- [display_driver.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\display\display_driver.cpp)

Metodos mais importantes:

- `FirmwareEntrypoint::run`
- `FirmwareEntrypoint::tick`
- `SystemManager::start`
- `SystemManager::tick`
- `CompanionStateStore::ingest_*`
- `CompanionStateStore::refresh_derived_runtime_state`
- `BehaviorService::tick`
- `RoutineService::tick`
- `FaceGraphicsPipeline::tick`
- `FaceClipPlayer::tick`
- `FaceGazeController::tick`
- `FaceMultimodalSync::apply`
- `FaceFrameComposer::compose`
- `FaceDisplayRenderer::render`

Se a meta for mexer em estados:

- comece por [companion_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.hpp)
- depois [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)
- depois [behavior_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\behavior\behavior_service.cpp) e [routine_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\routine\routine_service.cpp)

Se a meta for mexer em faces, expressoes e animacoes:

- comece por [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
- em seguida estude [face_render_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\face_render_state_contracts.hpp)
- depois percorra `preset -> clip -> gaze -> multimodal -> safety -> compose -> render`

## Complemento A - Leitura Profunda Do Estado Do Companion

### A.1 Estrutura do snapshot central

Status: observado

O contrato principal em [companion_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.hpp) divide o companion em blocos estaveis. Os mais importantes para estudo sao:

- `CompanionStructuralState`
- `CompanionPersonalityState`
- `CompanionRuntimeState`
- `CompanionGovernanceState`
- `CompanionEmotionalState`
- `CompanionAttentionalState`
- `CompanionEnergeticState`
- `CompanionInteractionState`
- `CompanionTransientState`
- `CompanionSessionMemoryState`
- `CompanionSnapshot`

Leitura recomendada do snapshot:

- `structural`: capacidades e baseline estrutural do companion
- `personality`: identidade fixa e envelope adaptativo
- `runtime`: estado de produto observado no momento
- `governance`: saude e ultima decisao de arbitragem
- `emotional`: vetor emocional atual
- `attentional`: alvo e canal de atencao
- `energetic`: modo energetico e sinais de bateria/termica
- `interaction`: fase de conversa e dono de turno
- `transient`: marcadores de curtissimo prazo ligados a runtime
- `session`: memoria curta de continuidade contextual

### A.2 Writers, readers e redacao

Status: observado

Em [companion_state_contracts.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.cpp) existem tres funcoes centrais:

- `can_writer_mutate_domain(...)`
- `can_reader_observe_domain(...)`
- `redact_snapshot_for_reader(...)`

Isso significa, de forma pratica:

- nem todo escritor pode alterar qualquer dominio
- nem todo leitor pode observar todos os dados internos
- o snapshot entregue a cada subsistema pode ser redatado

Impacto arquitetural:

- personalidade, sessao e transient nao sao universalmente expostos
- cloud bridge, por exemplo, nao necessariamente recebe tudo que o runtime core recebe

### A.3 Estados de produto do companion

Status: observado

O produto state atual e derivado em [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp). Estados observados:

- `kBooting`
- `kIdleObserve`
- `kAttendUser`
- `kAlertScan`
- `kResponding`
- `kSleep`
- `kEnergyProtect`

Ordem observada de prioridade na derivacao:

- se runtime nao iniciou, `Booting`
- se protecao energetica ativa, `EnergyProtect`
- se ha resposta ativa, `Responding`
- se ha atencao a estimulo, `AlertScan`
- se ha atencao a usuario, `AttendUser`
- se houve timeout para sono, `Sleep`
- caso contrario, `IdleObserve`

Essa ordem e extremamente importante para manutencao, porque pequenas mudancas nela mudam toda a semantica do produto.

### A.4 Tempos e janelas importantes do estado

Status: observado

Constantes observadas em [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp):

- `AttendUserHoldMs = 1400`
- `AlertScanHoldMs = 1600`
- `RespondingHoldMs = 900`
- `IdleToSleepMs = 12000`
- `SessionMemoryRetentionMs = 18000`

Interpretacao:

- o companion nao troca de estado apenas por flags booleanas
- existem janelas temporais que estabilizam ou seguram o estado
- isso afeta diretamente percepcao de continuidade e "vontade" do robo

### A.5 Memoria curta de sessao

Status: observado

A sessao curta mora em `CompanionSessionMemoryState` e e atualizada por `refresh_session_memory(...)`.

Campos observados na sessao:

- `warm`
- `last_activity_ms`
- `last_engagement_ms`
- `engagement_recent_percent`
- `recent_stimulus`
- `recent_interaction`
- `last_turn_owner`

Leitura pratica:

- essa camada responde pela sensacao de continuidade contextual
- ela impede que o companion "esfrie" imediatamente entre interacoes proximas
- `behavior`, `routine`, `face` e `motion` ja leem efeitos derivados dessa camada

### A.6 Personalidade fixa vs adaptativa

Status: observado

Em `CompanionPersonalityState`, os tracos-base persistem como identidade e os parametros adaptativos entram como bias bounded.

O que foi observado como parte fixa da identidade:

- calor social base
- curiosidade base
- compostura base
- iniciativa base
- assertividade base

O que foi observado como envelope adaptativo:

- bias social/warmth
- bias de energia de resposta
- bias de janela de continuidade
- baseline vindo da memoria persistente

Leitura pratica:

- a identidade nao deveria mudar drasticamente em runtime
- a adaptacao existe, mas fica contida dentro de clamps e passos pequenos

### A.7 Relacao entre sinais e estado

Status: observado

Cada servico principal escreve em um dominio diferente:

- percepcao e voz alimentam atencao/interacao
- behavior influencia transient e, por governanca, o estado runtime percebido
- routine influencia autonomia e ritmo de idle
- emotion alimenta o dominio emocional
- power alimenta energetico
- storage alimenta baseline persistente

Isso significa que o estado final do companion nao nasce de um unico servico. Ele e sempre uma composicao em torno do `CompanionStateStore`.

## Complemento B - Leitura Profunda Da Face

### B.1 Face como pipeline, nao como widget

Status: observado

A face atual nao e so um renderer de olhos. Ela e um pipeline com estagios observaveis:

- selecao de preset oficial
- animacao curta por clip
- direcionamento temporal de gaze
- modulacao multimodal
- limitacao por seguranca visual
- composicao do frame
- render adaptativo no display
- telemetria e fallback visual

Essa leitura vem diretamente de [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp).

### B.2 Como a face decide o baseline visual

Status: observado

O baseline da face e escolhido por preset oficial. Em termos de produto, isso define a "atitude" facial dominante antes das modulacoes finas.

Arquivos-chave:

- [face_preset_library.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_preset_library.cpp)
- [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)

O pipeline usa o contexto multimodal e o snapshot do companion para decidir entre presets como:

- neutral
- attend
- calm
- curious
- lock

Depois disso, o estado ainda pode ser mexido por layers superiores.

### B.3 Como a face incorpora contexto social e de sessao

Status: observado

A face recebe em `FaceMultimodalInput` dados como:

- `session_warm`
- `recent_interaction_phase`
- `recent_turn_owner`
- `social_engagement_percent`
- `behavior_activation_percent`
- `recent_engagement_percent`
- `personality`
- `companion_product_state`

Isso mostra que a expressao atual nao depende so de sensor cru. Ela depende tambem de:

- contexto social recente
- estado do produto
- baseline de personalidade
- nivel de ativacao comportamental

### B.4 Como blink e abertura de olhos sao produzidos

Status: observado

Blink nao foi observado como animacao separada por sprite. O comportamento atual vem principalmente de:

- `FaceMultimodalSync::apply(...)`
- `resolve_blink_pattern(...)`
- `state->lids.phase`
- `state->lids.openness_percent`

Padroes observados:

- maior engajamento muda timing de blink
- audio e ativacao podem induzir microvariaoes
- `BlinkPhase` passa por `kClosing`, `kClosed`, `kOpening`, `kOpen`

### B.5 Como o olhar e produzido

Status: observado

O olhar final resulta da combinacao de:

- `FacePreset` inicial
- `FaceGazeController`
- ajustes laterais/parallax no `FaceFrameComposer`
- limitacao posterior por `FaceMotionSafety`

Ou seja:

- primeiro decide-se para onde olhar
- depois o controller transforma isso em movimento temporal
- depois o frame composer transforma em geometria final
- por fim a seguranca pode achatar ou limitar a amplitude

### B.6 Como animacoes curtas convivem com o baseline

Status: observado

`FaceClipPlayer` resolve esse problema de forma explicita:

- faz claim da layer `kClip`
- fotografa o baseline atual
- aplica keyframes do clip enquanto ele estiver ativo
- ao final interpola de volta para o snapshot-base
- libera a layer com cooldown

Esse desenho e importante porque evita que uma animacao curta deixe residuos na face depois que termina.

### B.7 Como a face vira pixels

Status: observado

O caminho tecnico completo observado e:

- `FaceRenderState`
- `FaceGeometryLayout`
- `FaceFrame`
- `DisplayRenderPlan`
- operacoes reais do `DisplayDriver`

Papel de cada etapa:

- `FaceRenderState`: semantica e ownership
- `FaceGeometryLayout`: medidas intermediarias
- `FaceFrame`: payload final de desenho
- `DisplayRenderPlan`: estrategia de redraw
- `DisplayDriver`: flush real no ST7789

### B.8 Onde investigar quando a face parece parada

Status: inferido com alta confianca

Se a face estiver sempre "olho parado que pisca", os pontos de inspeção mais provaveis sao:

- `select_official_preset_for_input(...)` em [face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
- `FaceMultimodalSync::apply(...)`
- `FaceGazeController::set_target(...)`
- `FaceClipPlayer::play(...)`
- `analyze_render_plan(...)`
- `FaceDisplayRenderer::render(...)`
- `make_face_multimodal_input(...)`

Porque esses sao os lugares onde a face deixa de ser so um baseline estatico e comeca a reagir.
