# Preset Library Oficial (Face Premium++)

Data de congelamento: 2026-03-14.

## Separacao oficial vs exploratorio
- Biblioteca exploratoria: continua ativa para pesquisa visual e tuning.
- Biblioteca oficial: congelada apenas para presets validados com composicao governada e clips recuperaveis.

## Presets oficiais congelados
- core_neutral (Tier A): baseline principal e leitura imediata.
- core_attend (Tier A): atencao direta forte sem quebrar estabilidade de clip.
- core_calm (Tier B): estado energetico baixo com legibilidade controlada.
- core_curious (Tier B): curiosidade sem perda de coerencia composicional.
- core_lock (Tier C): intencao forte com convivencia estavel sob preempcao.

## Criterios de congelamento aplicados
- ownership por camada respeitado via FaceCompositor.
- clip sempre entra pela camada kClip governada (sem bypass).
- cancelamento/recovery retorna ao baseline sem deixar ownership residual.
- contraste visual + estabilidade temporal (hold/cooldown) + coerencia de personagem.

## O que permanece exploratorio
- playful_lift
- social_glance
- idle_drift

Motivo: exigem mais validacao para momentos longos, transientes e arbitragem multimodal antes de promocao.
