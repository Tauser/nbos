# Pipeline Local De Voz

## Objetivo

Fechar a arquitetura minima viavel da voz local usando apenas a base real que ja existe no projeto:

- captura local de audio
- detector simples de fala/trigger em `VoiceService`
- integracao com `Companion State`
- reproducao local basica de audio

## Decisao oficial desta etapa

### Trigger oficial

O trigger oficial minimo da voz local passa a ser:

- primario: `EnergyTrigger` local em cima do pico de audio ja capturado pelo `AudioService`
- assistido: `TouchAssisted` como fallback de usabilidade e recuperacao de contexto
- sem wake-word obrigatoria nesta fase

Motivo:

- isso preserva a base real do produto hoje
- evita depender de wake-word completo sem hardware/calibracao fechados
- mantem baixa latencia para comecar uma interacao curta

### Captacao minima oficial

A captacao minima oficial do canal local de voz fica consolidada assim:

- `AudioService` continua fazendo capture de pico curto (`60 ms`)
- o runtime passa a fazer probe de audio em cadencia curta, propria para trigger local
- profile base: `audio_probe_interval_ms = 140`
- profile `prod`: `audio_probe_interval_ms = 180`
- `VoiceService` so considera a entrada utilizavel quando a captura e:
  - recente
  - valida
  - com janela minima de amostras

Protecoes minimas oficiais desta etapa:

- captura antiga nao abre `listening`
- captura com poucas amostras nao abre `listening`
- `trigger_candidate` continua exigindo fala forte por frames consecutivos
- cooldown continua impedindo rearme imediato

### ASR local oficial

O fluxo oficial minimo de ASR local fica definido como:

- `ConstrainedIntent`
- foco em comando curto e turno breve
- sem transcricao rica obrigatoria
- sem partial transcript obrigatoria
- sem dependencia de cloud

Operacionalizacao minima desta fase:

- o runtime ainda nao faz reconhecimento lexical
- a camada local converte contexto curto em intents basicas de produto
- intents oficiais desta etapa:
  - `kAttendUser`
  - `kAcknowledgeUser`
  - `kInspectStimulus`
  - `kPreserveEnergy`

Mapeamento minimo atual:

- sessao fria -> `kAttendUser`
- sessao quente/usuario ja engajado -> `kAcknowledgeUser`
- contexto recente de estimulo -> `kInspectStimulus`
- energia constrangida -> `kPreserveEnergy`

Interpretacao pratica:

- o produto ainda nao promete ditado livre
- a voz local passa a gerar resposta minima coerente com o contexto do runtime
- o contrato continua abrindo caminho para ASR mais rico depois

### TTS local oficial

O fluxo oficial minimo de saida de voz fica definido como:

- `EarconFirst`
- confirmacao curta e imediata como resposta principal minima
- TTS natural fica explicitamente adiado
- cloud TTS continua desabilitado na policy minima

Operacionalizacao minima desta fase:

- `VoiceService` prepara um `VoiceResponsePlan`
- o `FirmwareEntrypoint` usa o `AudioService` para tocar um earcon coerente
- ao tocar com sucesso, o estado e atualizado para resposta do companion

Mapa minimo de resposta:

- `kAttendUser` -> `WakeChirp`
- `kAcknowledgeUser` -> `AcknowledgeChirp`
- `kInspectStimulus` -> `StimulusChirp`
- `kPreserveEnergy` -> `EnergySoftChirp`

Interpretacao pratica:

- nesta fase, utilidade e latencia valem mais que naturalidade de fala
- o companion pode confirmar acao rapidamente sem depender de pipeline pesado de sintese
- a resposta local ja impacta o estado central e o comportamento seguinte

## Prioridades oficiais

A ordem oficial de prioridade para a voz local minima e:

1. `TriggerResponsiveness`
2. `ShortTurnUtility`
3. `RichTranscriptFidelity`

Consequencias praticas:

- e melhor responder rapido a um turno curto do que perseguir transcricao longa
- e melhor confirmar uma interacao de forma simples do que esperar TTS natural
- fidelidade rica de ASR/TTS fica fora do caminho critico desta fase

## Base real reaproveitada

Arquivos relevantes:

- [voice_runtime_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\voice_runtime_contracts.hpp)
- [voice_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\voice\voice_service.cpp)
- [audio_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\audio\audio_service.cpp)
- [audio_local_port.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\audio\audio_local_port.cpp)
- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [system_config.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\system_config.hpp)
- [0018-voice-pipeline-local-base.md](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\architecture\0018-voice-pipeline-local-base.md)
- [0021-voice-vision-companion-behavior-integration.md](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\architecture\0021-voice-vision-companion-behavior-integration.md)

## O que continua fora de escopo

- wake-word fonetico real
- ASR aberto de ditado livre
- TTS natural local
- streaming de fala
- politica multimodal longa de dialogo

## Resultado arquitetural

A voz local fica oficialmente consolidada como um pipeline de baixa latencia para turnos curtos:

- captar audio local em cadencia curta
- aceitar apenas captura valida e recente
- detectar fala/trigger local
- mapear contexto curto para uma intent minima util
- tocar resposta local leve e coerente
- promover atencao/interacao auditiva e resposta do companion no estado central

Isso fecha uma arquitetura minima util sem prometer uma pilha de voice assistant completa antes da hora.
