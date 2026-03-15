# 0015 - Behavior engine base com prioridades e preempcao

## Objetivo
Criar uma camada comportamental inicial que:
- consome Companion State
- emite propostas comportamentais
- delega arbitragem para Action Governance

Sem duplicar governanca e sem acoplar diretamente em detalhes internos de Face/Motion.

## Decisao
Behavior nasce como `BehaviorService` com fronteira clara:
1. Seleciona perfil comportamental candidato por contexto (`energy_protect`, `alert_scan`, `attend_user`).
2. Materializa `ActionProposal` com prioridade/ttl/preemption policy.
3. Recebe `GovernanceDecision` do runtime e atualiza telemetria local.

## Regras de prioridade
- `energy_protect` (power) prioridade 10, `AllowAlways`.
- `alert_scan` (motion) prioridade 7, `AllowIfHigherPriority`.
- `attend_user` (face) prioridade 6, `AllowIfHigherPriority`.

## Propriedade e limites
- Behavior nao decide lease final de dominio (papel do `ActionGovernor`).
- Behavior nao chama internals de renderer/driver.
- Behavior nao substitui Companion State nem Action Governance.
- Sem maquina de estados grande nesta fase; apenas perfis iniciais com cooldown curto.

## Trade-offs
- Pro: base pequena e testavel para evolucao incremental.
- Pro: preempcao e prioridades ja nascem compatíveis com governanca existente.
- Contra: ainda sem executor dedicado para aplicar cada proposta nos subsistemas (fase seguinte).
