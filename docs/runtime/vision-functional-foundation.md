# Vision Functional Foundation

## Objetivo

Fechar o escopo util da visao local do NC-OS sem abrir um pipeline de visao computacional maior do que o produto precisa nesta fase.

A fundacao oficial da visao fica restrita a:

- presenca visual basica
- saliencia de estimulo visual recente
- contribuicao para atencao do companion

## Base real reaproveitada

Arquivos centrais:

- [camera_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\vision\camera_service.cpp)
- [perception_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\vision\perception_service.cpp)
- [vision_runtime_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\vision_runtime_contracts.hpp)
- [perception_runtime_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\perception_runtime_contracts.hpp)
- [companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)

## Sinais visuais oficiais

A visao funcional do produto agora fica consolidada em tres sinais oficiais:

1. `visual_presence_confidence_percent`
2. `visual_signal_active`
3. `AttentionTarget::kStimulus` com `AttentionChannel::kVisual`

Interpretacao pratica:

- a camera informa que existe presenca/saliencia visual suficiente
- a camera nao afirma por si so que o alvo e um usuario identificado
- a camera nao abre sozinha uma sessao de interacao de usuario

## O que a visao faz nesta fase

A visao pode:

- manter o companion atento ao ambiente
- alimentar `recent_stimulus`
- sustentar `AlertScan`/curiosidade visual
- aquecer expressao e rotina orientadas a estimulo

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
- `camera` entra como sinal de estimulo/presenca visual, nao como identidade do alvo

Consequencia:

- `PerceptionService` visual deve apontar para `kStimulus`
- `PerceptionService` auditivo/touch podem apontar para `kUser`

## Resultado util

Isso fecha um escopo pequeno, mas realmente util:

- o companion percebe movimento/presenca visual suficiente para olhar e manter atencao
- o sistema nao exagera e nao promete reconhecimento que nao existe
- o estado central continua coerente com a capacidade real do produto
