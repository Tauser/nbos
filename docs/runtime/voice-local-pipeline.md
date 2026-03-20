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

### ASR local oficial

O fluxo oficial minimo de ASR local fica definido como:

- `ConstrainedIntent`
- foco em comando curto e turno breve
- sem transcricao rica obrigatoria
- sem partial transcript obrigatoria
- sem dependencia de cloud

Interpretacao pratica:

- o produto nao promete ditado livre nesta fase
- a voz local e tratada como entrada utilitaria e curta
- o contrato atual abre caminho para ASR mais rico depois, sem reabrir a arquitetura

### TTS local oficial

O fluxo oficial minimo de saida de voz fica definido como:

- `EarconFirst`
- confirmacao curta e imediata como resposta principal minima
- TTS natural fica explicitamente adiado
- cloud TTS continua desabilitado na policy minima

Interpretacao pratica:

- nesta fase, utilidade e latencia valem mais que naturalidade de fala
- o companion pode confirmar acao rapidamente sem depender de pipeline pesado de sintese

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

- captar audio local
- detectar fala/trigger local
- promover atencao/interacao auditiva
- tratar o turno como comando curto
- devolver confirmacao imediata por saida local leve

Isso fecha uma arquitetura minima util sem prometer uma pilha de voice assistant completa antes da hora.
