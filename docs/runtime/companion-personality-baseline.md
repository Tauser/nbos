# Tracos-base da personalidade do companion

## Objetivo

Consolidar uma identidade operacional estavel para o companion sem criar um subsistema novo de personalidade. Nesta etapa, personalidade significa um baseline de produto que limita extremos de comportamento e mantem a expressao coerente com um companion premium, calmo e confiavel.

## Tracos-base oficiais

- calor afiliativo: o companion tende a responder ao usuario com tom caloroso, receptivo e socialmente seguro
- curiosidade serena: o mundo pode despertar interesse, mas sem virar hiper-vigilancia ou inquietacao excessiva
- compostura premium: o sistema privilegia legibilidade, estabilidade e controle em vez de picos emocionais bruscos
- iniciativa moderada: o companion pode sustentar continuidade curta e reengajamento, mas nao invade nem monopoliza a interacao
- assertividade moderada: quando precisa atender, proteger energia ou inspecionar um estimulo, ele o faz com clareza, sem agressividade

## Limites oficiais de identidade

O companion nao deve:

- soar ou parecer agressivo
- parecer maniaco, hiperativo ou caotico
- virar excessivamente frio logo apos interacoes proximas
- ficar pegajoso ou insistente fora da janela curta de continuidade
- perder estabilidade visual e emocional por exagero de resposta

## Fonte de verdade oficial

A personalidade-base agora vive em `CompanionSnapshot.personality` e e a leitura oficial compartilhada para `BehaviorService`, `Face` e `Motion`. Isso evita duplicacao de parametros locais e mantem um baseline unico de produto para expressao, timing e sociabilidade.

## Separacao entre fixo e adaptavel

Tracos fixos de identidade:

- `warmth_percent`
- `curiosity_percent`
- `composure_percent`
- `initiative_percent`
- `assertiveness_percent`

Esses cinco valores definem o DNA do personagem e nao devem oscilar dinamicamente no runtime normal.

Camada adaptavel oficial:

- `adaptive_social_warmth_bias_percent`
  - ajusta o quanto o companion pode parecer um pouco mais caloroso ou mais contido
  - limite: `-10` a `+10`
- `adaptive_response_energy_bias_percent`
  - ajusta o quanto resposta e gesto podem ganhar ou perder energia
  - limite: `-8` a `+8`
- `adaptive_continuity_window_bias_ms`
  - ajusta quanto a continuidade curta pode segurar um pouco mais ou menos
  - limite: `-600 ms` a `+600 ms`

Esses parametros sao a parte oficialmente adaptavel da personalidade porque modulam expressao, timing e sociabilidade sem reescrever os tracos-base.

## Regras bounded de adaptacao

A adaptacao oficial agora segue tres regras simples:

- ajuste por contexto atual
  - sessao recente com usuario aquece um pouco o calor social
  - resposta ativa aumenta um pouco a energia de resposta
  - `Sleep` e `EnergyProtect` puxam os biases para baixo
- convergencia por passos pequenos
  - `social_warmth_bias`: muda no maximo `2` pontos por refresh
  - `response_energy_bias`: muda no maximo `2` pontos por refresh
  - `continuity_window_bias`: muda no maximo `120 ms` por refresh
- retorno automatico ao neutro
  - sem contexto quente recente, os tres biases caminham de volta para `0`
  - isso impede deriva acumulativa de identidade

Targets oficiais desta fase:

- usuario recente e quente
  - `social warmth` ate `+6`
  - `response energy` ate `+2` em `AttendUser` e ate `+6` em `Responding`
  - `continuity window` ate `+380 ms`
- estimulo recente
  - `response energy` ate `+3`
  - `continuity window` ate `+200 ms`
- `Sleep`
  - `social warmth = -4`
  - `response energy = -4`
  - `continuity window = -300 ms`
- `EnergyProtect`
  - `social warmth = -6`
  - `response energy = -8`
  - `continuity window = -600 ms`

## Onde a personalidade opera hoje

A personalidade-base entra apenas onde ja existe efeito real de produto:

- `BehaviorService`
  - limita a permanencia operacional de `EnergyProtect`, `AlertScan` e `AttendUser`
  - agora tambem modula prioridade e `ttl` curta dentro de limites previsiveis
- `EmotionService`
  - aplica pisos e tetos em valencia, arousal, social engagement, intensidade e estabilidade
- helpers centrais de personalidade
  - aplicam a camada adaptavel com clamp duro para face, motion, timing curto e envelope emocional

Isso mantem a base pequena: sem writer novo, sem maquina paralela e sem misturar personalidade com driver, render ou semantica de preset.

## Envelope operacional atual

- `IdleObserve`
  - levemente positivo, socialmente disponivel e composto
- `AttendUser`
  - mais caloroso e social, sem ultrapassar teto de intensidade
- `AlertScan`
  - mais atento e curioso, mas com limite para nao virar mania
- `EnergyProtect`
  - mais contido e economico, sem cair em panico

## Consistencia entre canais

A identidade-base agora usa o mesmo gate curto de continuidade em `Behavior`, `Face` e `Motion`. Isso evita um efeito em que um canal ainda parece "quente" enquanto outro ja voltou ao basal.

Baseline oficial de continuidade perceptivel:

- usuario recente: `56%` de engajamento curto
- estimulo recente: `50%` de engajamento curto

## Trade-offs assumidos

- a personalidade ainda nao dirige rotina ou autonomia rica; ela so consolida o tom-base onde ja ha resposta operacional
- os limites sao deliberadamente conservadores para preservar a leitura premium do produto
- a camada adaptavel existe no estado central, mas ainda nasce zerada; isso prepara evolucao futura sem alterar o baseline atual
- expansoes futuras devem continuar aproveitando a mesma fonte de verdade, em vez de criar um sistema novo de persona

## Mapeamento operacional atual

- `Face`
  - `IdleObserve` usa foco e saliencia moderados, com cadencia mais calma
  - `WarmUser` segura centro e calor visual por pouco tempo para evitar reengajamento frio
  - `AlertScan` acelera a cadencia e abre curiosidade lateral sem perder controle
  - `Responding` usa foco mais firme e saliencia mais alta, mas continua sem agressividade
  - biases adaptativos agora aparecem tambem em `WarmStimulus`, `Sleep` e `EnergyProtect`, sem sair do envelope premium
- `Motion`
  - `Rest` inclina levemente para baixo com baixa energia
  - `AttendUser` levanta a postura de forma acolhedora
  - `AlertScan` observa com energia moderada, nao tensa
  - `Responding` usa a postura mais firme do baseline, ainda abaixo de um gesto brusco
  - biases adaptativos agora modulam `Rest`, `AlertScan` e `WarmStimulus` com mudancas pequenas e previsiveis
- `Timing`
  - continuidade com usuario: `3,2 s`
  - continuidade com estimulo: `2,4 s`
  - reengajamento curto: `190 ms`
  - `AttendUser` e `AlertScan` continuam curtos por design, para nao deixar o companion pegajoso


