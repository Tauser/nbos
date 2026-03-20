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
- profile base: `audio_probe_interval_ms = 120`
- profile `prod`: `audio_probe_interval_ms = 160`
- `VoiceService` so considera a entrada utilizavel quando a captura e:
  - recente
  - valida
  - com janela minima de amostras

Protecoes minimas oficiais desta etapa:

- captura antiga nao abre `listening`
- captura com poucas amostras nao abre `listening`
- trigger rapido exige energia um pouco mais alta
- cooldown continua impedindo rearme imediato

### Responsividade minima oficial

O caminho minimo de usabilidade desta fase fica assim:

- `TriggerSpeechFrames = 2`
- `TriggerThresholdPercent = 30`
- freshness window curta para evitar reaproveitar audio velho
- o `VoiceRuntimeState` passa a registrar:
  - idade da captura aceita
  - latencia do trigger
  - latencia de resposta pronta
  - duracao do earcon escolhido

Alvo pratico desta fase:

- o canal local deve reagir em cerca de uma janela curta de fala, nao em uma sequencia longa de probes
- a resposta deve ficar pronta no mesmo tick do trigger
- a experiencia deve soar curta, distinta e previsivel

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

- `kAttendUser` -> `WakeChirp` (`760 Hz / 60 ms`)
- `kAcknowledgeUser` -> `AcknowledgeChirp` (`980 Hz / 75 ms`)
- `kInspectStimulus` -> `StimulusChirp` (`560 Hz / 105 ms`)
- `kPreserveEnergy` -> `EnergySoftChirp` (`392 Hz / 85 ms`)

Interpretacao pratica:

- o canal agora responde mais rapido
- os earcons ficaram mais separados entre si
- a resposta continua curta o suficiente para nao travar o turno

## Prioridades oficiais

A ordem oficial de prioridade para a voz local minima e:

1. `TriggerResponsiveness`
2. `ShortTurnUtility`
3. `RichTranscriptFidelity`

## Base real reaproveitada

Arquivos relevantes:

- [voice_runtime_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\voice_runtime_contracts.hpp)
- [voice_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\voice\voice_service.cpp)
- [audio_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\audio\audio_service.cpp)
- [audio_local_port.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\audio\audio_local_port.cpp)
- [firmware_entrypoint.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\app\boot\firmware_entrypoint.cpp)
- [system_config.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\config\system_config.hpp)

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
- detectar fala/trigger local com metrica explicita de latencia
- mapear contexto curto para uma intent minima util
- tocar resposta local leve, curta e distinta
- promover atencao/interacao auditiva e resposta do companion no estado central

Isso fecha uma arquitetura minima util sem prometer uma pilha de voice assistant completa antes da hora.
