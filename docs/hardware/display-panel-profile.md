# Panel Capability Profile

Data: 2026-03-18
Painel: ST7789 240x320 ligado ao Freenove ESP32-S3 CAM

## Leitura consolidada

O painel se mostrou estavel em alternancia estatica de polaridade, mas apresentou artefatos sob atualizacao dinamica com alto contraste e movimento.

## Capacidades reais

- Resolucao: 240x320
- Barramento: SPI
- Leitura de pixel: desabilitada
- Reset dedicado por GPIO: nao
- Reset compartilhado com `EN`: sim
- Inversao do painel: manter habilitada

## Limites praticos observados

- `PanelPolarityFlip`: limpo
- `HorizontalSweepFullRedraw`: artefatos em linhas dentro da area branca
- `EyeTrailFullRedraw`: artefatos diagonais dentro da area cinza
- `EyeTrailDirtyRect`: artefatos leves e localizados
- `SpriteWindowTrail`: flicker leve em toda a janela vermelha

## Implicacoes de flush e redraw

- `full redraw` funciona, mas pode introduzir artefatos em movimento de alto contraste
- `dirty rect` pequeno e localizado e aceitavel para movimento ocular leve
- `sprite window` nao deve ser caminho padrao do produto no estado atual

## Workarounds validados

- manter `invert=true`
- preferir primitivas diretas em vez de sprite-window no caminho de produto
- manter redraw parcial pequeno para movimento leve dos olhos
- evitar, quando possivel, grandes areas de alto contraste em movimento com redraw total continuo

## Encapsulamento

As capacidades e quirks do painel ficam centralizadas em:
- `src/drivers/display/panel_capability_profile.hpp`
- `src/drivers/display/panel_capability_profile.cpp`

O driver consome esse perfil em:
- `src/drivers/display/display_driver.cpp`

O restante do sistema nao deve codificar quirks do ST7789 diretamente.
