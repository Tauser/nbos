# ADR-0007: Companion State Central

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Definir uma fonte central e oficial de verdade do estado do robo para evitar estados fragmentados entre subsistemas.

## Modelagem adotada
O Companion State foi dividido em quatro blocos semanticos:
- `CompanionStructuralState`: identidade estrutural estavel (offline-first, versao semantica, board).
- `CompanionRuntimeState`: estado operacional agregado do runtime.
- `CompanionGovernanceState`: saude agregada de governanca (allow/preempt/reject + health).
- `CompanionTransientState`: transicoes curtas em curso (trace, dominio, owner, timestamp).

O snapshot oficial e `CompanionSnapshot`, versionado por `revision`.

## Fronteiras
- Companion State agrega estado; nao arbitra politica.
- Action Governance arbitra decisoes.
- EventBus transporta sinais semanticos.
- Runtime sincroniza sinais agregados para o Companion State.

## Integracao minima desta etapa
- `CompanionStateStore` implementado em `core/state`.
- `SystemManager` inicializa estado estrutural e sincroniza estado de runtime/governanca a cada tick.
- `RuntimeStatus` expoe `companion_state_revision` para observabilidade de sincronizacao.

## Limites desta etapa
- Nao integra Face/Motion/Behavior diretamente ainda.
- Nao cria politicas de comportamento no Companion State.
- Nao introduz persistencia em armazenamento nesta fase.
