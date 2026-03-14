# ADR-0010: Integracao Companion State, EventBus v2 e Action Governance

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Criar eixo sistemico basico entre sinais semanticos, arbitragem de acao e estado agregado sem acoplamento circular fragil.

## Decisao
Introduzido `CompanionStateAxis` em `core/runtime` com papel de orquestracao fina:
- assina lanes do EventBus (`event`, `intent`, `command`, `reaction`)
- traduz sinais em atualizacoes do Companion State por contratos de ownership
- consulta Action Governance somente para `command` e persiste decisao no dominio transiente

## Limites de responsabilidade
- EventBus transporta sinal; nao armazena estado.
- Action Governance arbitra proposta; nao armazena estado agregado.
- Companion State agrega contexto semantico; nao decide politica de arbitragem.
- `CompanionStateAxis` apenas integra os tres papeis, sem substituir nenhum deles.

## Fluxos desta etapa
1. `CommandMessage` -> `ActionProposal` -> `ActionGovernor::evaluate` -> `CompanionState::ingest_governance_decision`
2. `IntentMessage` -> `CompanionState::ingest_interactional` ou `ingest_energetic`
3. `EventMessage` -> `CompanionState::ingest_attentional`
4. `ReactionMessage` -> `CompanionState::ingest_emotional`

## Trade-offs
- Beneficio: trilho formal e testavel para coerencia sistemica inicial.
- Custo: mapeamentos semanticos ainda heurísticos e intencionalmente simples.
- Decisao: manter mapeamentos minimos agora; evoluir com Companion State/Behavior sem quebrar contratos.

## Riscos abertos
- Sem namespace de payload rico por topico ainda.
- Sem observabilidade detalhada de falhas de ingestao por writer no eixo.
- Mapeamentos podem ser refinados conforme Face Gate e subsistemas premium avancarem.
