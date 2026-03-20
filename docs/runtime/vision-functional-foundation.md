# Vision Functional Foundation

## Objetivo

Fechar o escopo util da visao local do NC-OS sem abrir um pipeline de visao computacional maior do que o produto precisa nesta fase.

A fundacao oficial da visao fica restrita a:

- presenca visual basica
- saliencia de estimulo visual recente
- contribuicao para atencao do companion
- inferencia visual basica de usuario somente quando o sinal ficar estavel

## Base real reaproveitada

Arquivos centrais:

- [camera_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\vision\camera_service.cpp)
- [perception_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\vision\perception_service.cpp)
- [vision_runtime_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\vision_runtime_contracts.hpp)
- [perception_runtime_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\perception_runtime_contracts.hpp)
- [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)

## Sinais visuais oficiais

A visao funcional do produto agora fica consolidada em sinais oficiais pequenos e auditaveis:

1. `visual_presence_confidence_percent`
2. `visual_signal_active`
3. `visual_user_inference_active`
4. `consecutive_visual_frames`
5. `visual_hold_until_ms`
6. `AttentionChannel::kVisual`

Interpretacao pratica:

- a camera informa que existe presenca/saliencia visual suficiente
- a camera nao afirma identidade
- a camera so promove para atencao basica de usuario quando a presenca visual for forte e estavel por frames consecutivos
- fora dessa janela estavel, o alvo visual continua sendo tratado como `kStimulus`

## O que a visao faz nesta fase

A visao pode:

- manter o companion atento ao ambiente
- alimentar `recent_stimulus`
- sustentar `AlertScan`/curiosidade visual
- aquecer expressao e rotina orientadas a estimulo
- promover `AttentionTarget::kUser` no canal visual quando a presenca se sustenta por uma janela curta e coerente

## O que a visao nao faz nesta fase

A visao nao faz:

- deteccao de rosto
- identificacao de usuario
- tracking persistente
- classificacao de objeto
- segmentacao
- interpretacao semantica de cena

## Regra de produto consolidada

Regra principal:

- `touch` e `voice` continuam sendo os canais que legitimam melhor a presenca do usuario
- `camera` entra primeiro como sinal de estimulo/presenca visual
- `camera` so sobe para presenca basica de usuario quando houver estabilidade curta suficiente

Consequencia:

- `PerceptionService` visual aponta para `kStimulus` no primeiro sinal visual relevante
- `PerceptionService` visual pode apontar para `kUser` quando a presenca visual ficar estavel por pelo menos duas capturas frescas
- `PerceptionService` auditivo/touch podem apontar para `kUser`

## Regra de estabilizacao

Para nao gerar flicker nem falso positivo bruto, a inferencia visual usa tres travas simples:

1. limiar de presenca visual forte
2. confirmacao por frames consecutivos
3. hold curto apos a ultima confirmacao forte

Interpretacao operacional:

- um frame visual bom nao basta para abrir sessao visual de usuario
- dois frames frescos e fortes ja permitem atencao basica de usuario no canal visual
- se o sinal cair logo em seguida, a promocao ainda fica viva por uma janela curta
- se essa janela expirar sem nova confirmacao, o estado volta para `kStimulus`

## Resultado util

Isso fecha um escopo pequeno, mas realmente util:

- o companion percebe movimento/presenca visual suficiente para olhar e manter atencao
- o sistema ja consegue tratar presenca visual estavel como atencao basica de usuario
- o sistema nao exagera e nao promete reconhecimento que nao existe
- o estado central continua coerente com a capacidade real do produto
