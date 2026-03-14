# ADR-0005: EventBus v2 Semantico

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Criar um barramento melhor que um pipe generico, mantendo simplicidade e sem absorver governanca completa.

## Decisao
Implementar `EventBusV2` com quatro trilhas semanticas separadas:
- eventos
- intents
- comandos
- reacoes

Cada trilha possui:
- fila dedicada
- subscribers dedicados
- validacao por tipo antes de aceitar mensagem

## Prioridade e tipos
- Tipo entra no barramento via contratos semanticos (`EventMessage`, `CommandMessage`, `IntentMessage`, `ReactionMessage`).
- Prioridade entra apenas na trilha de comando (`priority`), ordenando despacho dentro da fila de comandos.

## Limites claros
- EventBus transporta sinais e aplica validacao minima.
- EventBus nao arbitra conflito de autoria nem decide politicas de execucao.
- Action Governance continua responsavel por arbitrar `ActionProposal`/`GovernanceDecision`.

## Integracao inicial
- `SystemManager` passa a possuir uma instancia de `EventBusV2`.
- Runtime publica evento de `system_observation` no start.
- Runtime drena o barramento em cada tick com budget fixo por trilha.
- `RuntimeStatus` passa a expor contadores agregados do barramento.

## Consequencias
- Melhora legibilidade semantica e reduz acoplamento difuso.
- Cria base para Companion State e Action Governance crescerem sobre contratos claros.
- Mantem custo de runtime previsivel (filas fixas, sem alocacao dinamica).
