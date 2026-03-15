# 0020 - Percepcao basica de presenca e atencao

## Objetivo
Transformar sinais reais (camera, audio, touch) em saidas de percepcao normalizadas, sem introduzir inteligencia complexa cedo.

## Decisao
Criar `PerceptionService` em `services/vision` como camada semantica minima:
- calcula confianca de presenca (0..100)
- escolhe canal/target de atencao (touch, visual, auditory)
- publica sinais de atencao/interacao para o `SystemManager`

## Normalizacao
- todas as confidencias em `uint8_t` (0..100)
- `PerceptionStage`: `Dormant`, `PresenceDetected`, `AttentionLocked`
- fallback para `Dormant` em `safe_mode` ou energia `kCritical`

## Fora de escopo
- deteccao facial/objeto
- tracking persistente
- fusao multimodal avancada
