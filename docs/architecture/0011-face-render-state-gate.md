# ADR-0011: Face Render State e modelos centrais do gate arquitetural

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Abrir formalmente o gate arquitetural do Face Engine com um contrato central de render e modelos semanticos minimos, antes de Geometry V2, Gaze premium e clips longos.

## Decisao
Foi introduzido `FaceRenderState` como contrato oficial de render facial em `core/contracts`.
A semantica base foi separada em `models/face` para impedir renderer com decisao de produto.

Estrutura base:
- `face_models.hpp`: tipos semanticos de preset, gaze, blink, mouth, brow e layers.
- `face_render_state_contracts.hpp/.cpp`: contrato de estado, baseline seguro, validacao e ownership de camada.

## Regras de gate aplicadas
- `FaceRenderState` nao e saco de floats; usa enums + percentuais com validacao.
- Ownership por camada e explicito (`FaceLayerOwnership`, `FaceLayerClaim`).
- Composicao futura preparada por `FaceCompositionState` com `mode` e `composition_locked`.
- Renderer permanece consumidor do estado; semantica continua no contrato/modelos.

## Baseline seguro
`make_face_render_state_baseline()` define:
- preset neutro
- gaze central
- blink aberto
- mouth/brow neutros
- composicao single-preset sem owner inicial
- `FaceRenderSafetyMode::kSafeFallback`

## Trade-offs
- Beneficio: fundacao robusta e testavel para Face Premium++.
- Custo: ainda sem geometria rica e sem sistema completo de composicao.
- Decisao: manter escopo estritamente de gate para evitar monolito precoce.

## Limites desta etapa
- Nao implementa Geometry V2.
- Nao implementa gaze premium/saccades.
- Nao implementa clips longos.
- Nao cria FaceService completo ainda.
