# ADR-0012: Ownership explicito das camadas faciais

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Formalizar ownership das camadas faciais (base, blink, gaze, modulation, transient, clip) para reduzir conflitos cedo e preparar compositor futuro.

## Decisao
`FaceRenderState` passa a operar em seis camadas com ownership explicito por papel:
- `base` -> `kBaseOwner`
- `blink` -> `kBlinkOwner`
- `gaze` -> `kGazeOwner`
- `modulation` -> `kModulationOwner`
- `transient` -> `kTransientOwner`
- `clip` -> `kClipOwner`

A policy de cada camada foi congelada em contrato via `face_layer_policy(layer)`.
Claims de camada exigem role compativel com a policy.

## Relacao entre base, modulation, transient e clip
- `base`: identidade estrutural/preset, estavel.
- `modulation`: ajustes semanticos de estado (ex.: emocao/energia) sem reescrever base.
- `transient`: efeitos curtos e reativos (micro-eventos).
- `clip`: sequencias temporais dirigidas.

A separacao por camada impede sobrescrita acidental direta (`modulation` em `base`, `clip` em `blink`, etc.).

## Regras executivas desta etapa
- Ownership nao pode ser implicito.
- Arbitragem fica no contrato/claim; nao em `if` de renderer.
- Clip nao nasce como trilho paralelo; nasce como camada oficial do mesmo estado.

## Trade-offs
- Beneficio: reduz conflito estrutural cedo e facilita compositor Premium++.
- Custo: mais disciplina de claim por role/camada.
- Decisao: manter composicao final fora de escopo neste passo.
