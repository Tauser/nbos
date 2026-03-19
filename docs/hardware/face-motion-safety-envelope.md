# Face Motion Safety / Visual Envelope

## Objetivo

O painel ST7789 desta montagem preserva bem os movimentos oculares moderados, mas entra em uma zona visual pior quando combinamos:
- amplitude alta de olhar
- transições bruscas entre direções distantes
- diagonais fortes
- blink parcial com gaze ainda muito aberto

Este envelope oficial trata isso como restrição real de produto, não como bug de semântica do Face.

## Envelope seguro oficial

### Movimentos liberados
- olhar lateral moderado e sustentado
- olhar vertical moderado
- blink normal e microblink
- laterais com leitura premium do personagem
- diagonais suaves e curtas

### Movimentos limitados
- foco ocular lateral acima de `62%`
- foco ocular vertical acima de `54%`
- foco ocular diagonal acima de `44%`
- transições abruptas entre direções distantes nos primeiros `160 ms`
- gaze forte durante `closing`, `opening` e `closed`

## Guardrails aplicados

### Amplitude
- lateral: cap progressivo em `62%`
- vertical: cap progressivo em `54%`
- diagonal: cap progressivo em `44%`

### Velocidade
- transições curtas (`distance <= 1`) seguem livres
- transições mais bruscas entram em rampa segura por até `160 ms`
- o ganho de foco por frame fica limitado de forma progressiva, sem travar o rosto

### Diagonal
- diagonal continua permitida
- recebe menos foco efetivo e menor envelope geométrico do que lateral pura
- a geometria também comprime o topo de amplitude diagonal

### Blink + gaze
- `closing`: gaze comprimido para a zona `20-28%`
- `opening`: gaze comprimido para a zona `18-26%`
- `closed`: foco ocular colapsa para `0%`
- aberturas parciais abaixo de `72%` também reduzem gaze máximo

## Modos de segurança
- modo nominal: movimentos dentro do envelope seguro sem fallback pesado
- modo seguro contextual: quando o guardrail entra para conter amplitude/velocidade/blink+gaze
- fallback degradado continua existindo como trilha separada para degradação forte de render

## Arquivos que implementam o envelope
- `src/services/face/face_motion_safety.hpp`
- `src/services/face/face_motion_safety.cpp`
- `src/services/face/face_graphics_pipeline.cpp`
- `src/services/face/face_geometry.cpp`

## Intenção de produto
- esconder o teto do painel com elegância
- preservar a leitura premium do personagem
- limitar apenas a zona visual ruim
- manter laterais, blink e estados principais vivos e legíveis
