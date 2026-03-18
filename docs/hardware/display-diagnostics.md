# Diagnostico de Display e Pipeline

Objetivo: separar ghosting do painel ST7789 de defeito do pipeline de render, sem reabrir a arquitetura central.

## O que foi adicionado

- Analise formal de render em `src/services/display/display_pipeline_analysis.*`
- Runner de hardware em `src/services/display/display_diagnostics.*`
- Integracao opt-in no `FaceGraphicsPipeline`
- Testes nativos em `test/native/test_display_pipeline/test_main.cpp`

## Modos de diagnostico

Configurar em `src/config/system_config.hpp` no campo `display_diagnostics_mode`.

Modos disponiveis:
- `kOff`
- `kStaticPrimaries`
- `kStaticClipGrid`
- `kHorizontalSweepFullRedraw`
- `kEyeTrailFullRedraw`
- `kEyeTrailDirtyRect`
- `kSpriteWindowTrail`
- `kPanelPolarityFlip`

## Protocolo de validacao em hardware

Rodar nesta ordem:
1. `kPanelPolarityFlip`
2. `kHorizontalSweepFullRedraw`
3. `kEyeTrailFullRedraw`
4. `kEyeTrailDirtyRect`
5. `kSpriteWindowTrail`

## Leitura esperada

- Artefato no `kPanelPolarityFlip`: suspeita sobe para painel, alimentacao ou timing fisico.
- Artefato no `kHorizontalSweepFullRedraw` e no `kEyeTrailFullRedraw`: suspeita sobe para painel ou refresh geral, nao apenas dirty rect.
- Artefato so no `kEyeTrailDirtyRect`: suspeita sobe para clipping/dirty rect/limpeza parcial.
- Artefato so no `kSpriteWindowTrail`: suspeita sobe para limpeza de sprite, push de janela ou sobreposicao parcial.

## Sinais do pipeline

O renderer agora registra:
- motivo de full redraw
- dirty rect calculado
- dirty rect clipado por borda do painel

Isso ajuda a distinguir entre:
- full redraw forcado ou exigido por geometria/fundo
- redraw parcial coerente
- dirty rect valido, mas recortado pelo limite do display

## Perfil do painel

As capacidades e quirks consolidados do painel ficam registrados em docs/hardware/display-panel-profile.md e centralizados no codigo em src/drivers/display/panel_capability_profile.*.

