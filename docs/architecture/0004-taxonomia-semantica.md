# ADR-0004: Taxonomia Formal de Eventos, Comandos, Intents e Reacoes

- Status: Accepted
- Data: 2026-03-14

## Problema
Sem separacao formal, sinais de observacao, ordem, intencao e resposta transitoria tendem a colidir.
Isso gera conflito de autoria entre services e acopla Action Governance, Companion State e runtime.

## Definicoes formais
- **Evento**: observacao de algo que ocorreu. Nao ordena atuacao.
- **Comando**: ordem explicita para executar uma acao concreta.
- **Intent**: intencao derivada a partir de contexto/eventos, ainda sem atuacao direta.
- **Reacao**: resposta transitora de curto prazo, causada por comando/intencao.

## Taxonomia minima adotada
- `EventTopic`: `world_observation`, `system_observation`, `user_observation`
- `CommandTopic`: `motion_execute`, `face_render_execute`, `audio_output_execute`, `power_mode_set`
- `IntentTopic`: `attend_user`, `acknowledge_user`, `inspect_stimulus`, `preserve_energy`
- `ReactionTopic`: `blink_pulse`, `gaze_micro_shift`, `earcon_chirp`, `led_feedback_pulse`

## Contratos base
- `SignalHeader` com `kind`, `trace_id`, `timestamp_ms`
- mensagens tipadas por classe semantica
- validadores por tipo (`is_valid`) para impedir mistura semantica
- versao da taxonomia (`kSemanticTaxonomyVersion`)

## Relacao com EventBus v2
EventBus v2 deve ter quatro trilhas explicitamente separadas:
- publish/subscribe de eventos
- publish/subscribe de comandos
- publish/subscribe de intents
- publish/subscribe de reacoes

`SemanticBusPort` define essa fronteira para impedir roteamento ambiguo.

## Relacao com runtime
O runtime passa a carregar a versao da taxonomia e pode usar os validadores para gate em tempo de execucao.
`trace_id` habilita encadear observacao -> intencao -> comando -> reacao sem perder autoria.

## Relacao com Action Governance
Action Governance arbitra `ActionProposal` (origem comando/intencao) e devolve `GovernanceDecision`.
A arbitragem acontece antes da atuacao final, reduzindo conflito entre services.

## Limites desta etapa
- Nao implementa EventBus v2 completo.
- Nao implementa engine de arbitragem completa.
- Nao acopla Face/Behavior nesta fase.
