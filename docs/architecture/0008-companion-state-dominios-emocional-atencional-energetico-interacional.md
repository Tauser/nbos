# ADR-0008: Companion State com dominios emocional, atencional, energetico e interacional

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Expandir o Companion State central com semantica de alto valor para suportar Face, Motion, Behavior, Voice e Power sem multiplicar fontes paralelas de estado.

## Dominios adotados
- `CompanionEmotionalState`: tom emocional, nivel de arousal, intensidade e estabilidade.
- `CompanionAttentionalState`: alvo de atencao, canal sensorial dominante, confianca de foco e lock.
- `CompanionEnergeticState`: modo energetico, bateria, carga termica e alimentacao externa.
- `CompanionInteractionState`: fase da interacao, ownership de turno, sessao ativa e resposta pendente.

## Contratos e fronteiras
- Os quatro dominios entram no `CompanionSnapshot` e sao atualizados por sinais dedicados:
  - `CompanionEmotionalSignal`
  - `CompanionAttentionalSignal`
  - `CompanionEnergeticSignal`
  - `CompanionInteractionSignal`
- `CompanionStatePort` expoe ingestao explicita por dominio para evitar acoplamento implicito.
- Companion State continua sendo agregador semantico; nao arbitra politica de acao.

## Coerencia minima desta etapa
- Safe mode do runtime tem primazia:
  - forcado `EnergyMode::kCritical`
  - `InteractionPhase::kIdle`
  - `response_pending = false`
- Runtime nao iniciado zera sessao interacional ativa e ownership de turno.
- Atencao ao usuario com confianca minima ativa sessao interacional.

## Trade-offs
- Beneficio: base semantica forte para crescimento de subsistemas.
- Custo: aumento de superficie de contratos e necessidade de disciplina de ownership dos sinais.
- Decisao: manter regras de coerencia pequenas nesta fase para nao antecipar politica comportamental.

## Limites desta etapa
- Nao implementa politica completa de Behavior.
- Nao cria state machine global de interacao.
- Nao define persistencia historica dos dominios.
