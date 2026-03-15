# 0018 - Voice pipeline local base

## Objetivo
Abrir o trilho local de voz com captura e processamento inicial, sem antecipar ASR/TTS completo.

## Decisao
Criado `VoiceService` como camada de processamento leve sobre `AudioRuntimeState`:
- calcula energia de voz por janela
- detecta fala ativa (`listening`)
- gera `trigger_candidate` com janela de frames + cooldown
- publica sinais de atenção/interação auditiva no Companion State via runtime

## Escopo entregue
- Contrato `voice_runtime_contracts` com estado e estágios de voz.
- Serviço `VoiceService` com thresholds e anti-spam básico.
- Integração no `FirmwareEntrypoint` usando `SystemManager::ingest_*`.
- Testes nativos para listening, trigger e safe-mode.

## Limites desta etapa
- Sem wakeword real.
- Sem ASR/TTS.
- Sem pipeline semântico de transcrição.

## Caminho aberto
A próxima etapa pode conectar um detector de wake/trigger real em cima de `trigger_candidate`, preservando contratos e sem quebrar arquitetura em camadas.
