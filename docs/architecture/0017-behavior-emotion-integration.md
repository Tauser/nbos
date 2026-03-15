# 0017 - Integracao behavior/emotion com face, motion e Companion State

## Objetivo
Fechar o ciclo de coerência entre estado interno e expressão externa sem colapsar fronteiras.

## Decisão
- `BehaviorService` continua responsável por proposta comportamental.
- `EmotionService` foi introduzido para derivar sinal emocional a partir de `CompanionSnapshot` + estado de behavior/routine.
- `SystemManager` recebeu portas explícitas de ingestão de sinais (`emotional`, `attentional`, `interactional`, `energetic`) para manter o Companion State como eixo central.
- `FaceMultimodalInput` passou a carregar contexto semântico (arousal, social_engagement, behavior_activation), mantendo renderer como executor visual.
- `MotionService` segue consumindo `MotionCompanionSignal`, agora enriquecido por estado emocional e janela comportamental ativa.

## Fluxo integrado
1. Behavior propõe ação e passa por `Action Governance`.
2. Em decisões permitidas, comportamento injeta sinais sintéticos no `Companion State`.
3. Emotion deriva novo estado emocional e publica no `Companion State`.
4. Face e Motion consomem snapshots do `Companion State` e ajustam expressão/embodiment.

## Fronteiras preservadas
- Governança continua separada de estado agregado.
- Face e Motion não escrevem estado emocional diretamente.
- Entrypoint apenas orquestra; sem lógica de render ou arbitragem de domínio.

## Trade-offs
- Pro: coerência sistêmica visível já nesta fase, com baixo acoplamento estrutural.
- Pro: caminho claro para evoluir políticas de emoção/behavior sem reescrever Face/Motion.
- Contra: mapeamentos iniciais (behavior->sinais) ainda heurísticos e devem ser calibrados por tuning.
