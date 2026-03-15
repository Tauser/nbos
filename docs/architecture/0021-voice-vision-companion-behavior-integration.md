# 0021 - Integracao de voice e vision ao Companion State e Behavior

## Objetivo
Garantir que percepcao de voz e visao participe do eixo sistemico (Companion State -> Behavior), sem ficar isolada em services locais.

## Decisao
- `PerceptionService` (vision) e `VoiceService` continuam emitindo sinais formais de atencao/interacao.
- `SystemManager` permanece como ponto unico de ingestao de estado agregado.
- `FirmwareEntrypoint` foi reordenado para que `BehaviorService` leia snapshot apos ingestao de perception+voice no mesmo tick.

## Resultado arquitetural
- voz e visao deixam de ser apenas observacao local e passam a influenciar decisao comportamental no mesmo ciclo.
- `BehaviorService` agora reconhece contexto auditivo com `response_pending` para elevar prioridade de `attend_user`.

## Fora de escopo
- politica comportamental completa para multimodalidade
- memoria temporal longa de percepcao
