# ADR-0006: Action Governance com Ownership e Prioridades

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Criar uma base minima de governanca de acoes para impedir conflito de autoria entre services, sem acoplar indevidamente Companion State ou Behavior.

## Modelo minimo adotado
- **Quem pede**: service emissor via `ActionProposal` (`requester_service`, `domain`, `priority`, `ttl_ms`, `preemption_policy`).
- **Quem decide**: `ActionGovernor`.
- **Quem pode interromper**: novo proponente, apenas sob politica de preempcao explicita.

## Ownership por dominio
A governanca opera por `ActionDomain`:
- `face`
- `motion`
- `audio`
- `power`
- `led`

Cada dominio possui um lease ativo (`DomainLease`) com owner, prioridade e expiracao.

## Prioridade e preempcao
- `kForbid`: nunca preempta owner atual.
- `kAllowIfHigherPriority`: preempta apenas se `priority` for maior que o lease ativo.
- `kAllowAlways`: permite preempcao independentemente da prioridade.

Decisoes possiveis:
- `kAllow`
- `kPreemptAndAllow`
- `kReject`
- `kDefer`

## Relacao com EventBus e runtime
- EventBus v2 continua transporte semantico.
- Action Governance arbitra propostas antes da atuacao final.
- Runtime integra o governor como componente de orquestracao, sem acoplamento circular.

## Limites desta etapa
- Nao implementa Companion State.
- Nao implementa politica comportamental completa.
- Nao resolve fairness global multi-dominio; apenas ownership local por dominio.
