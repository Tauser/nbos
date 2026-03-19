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

## Onde a personalidade opera hoje

A personalidade-base entra apenas onde ja existe efeito real de produto:

- `BehaviorService`
  - limita a permanencia operacional de `EnergyProtect`, `AlertScan` e `AttendUser`
- `EmotionService`
  - aplica pisos e tetos em valencia, arousal, social engagement, intensidade e estabilidade

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

## Trade-offs assumidos

- a personalidade ainda nao dirige rotina ou autonomia rica; ela so consolida o tom-base onde ja ha resposta operacional
- os limites sao deliberadamente conservadores para preservar a leitura premium do produto
- expansoes futuras devem continuar aproveitando a mesma fonte de verdade, em vez de criar um sistema novo de persona

## Mapeamento operacional atual

- `Face`
  - `IdleObserve` usa foco e saliencia moderados, com cadencia mais calma
  - `WarmUser` segura centro e calor visual por pouco tempo para evitar reengajamento frio
  - `AlertScan` acelera a cadencia e abre curiosidade lateral sem perder controle
  - `Responding` usa foco mais firme e saliencia mais alta, mas continua sem agressividade
- `Motion`
  - `Rest` inclina levemente para baixo com baixa energia
  - `AttendUser` levanta a postura de forma acolhedora
  - `AlertScan` observa com energia moderada, nao tensa
  - `Responding` usa a postura mais firme do baseline, ainda abaixo de um gesto brusco
- `Timing`
  - continuidade com usuario: `3,2 s`
  - continuidade com estimulo: `2,4 s`
  - reengajamento curto: `190 ms`
  - `AttendUser` e `AlertScan` continuam curtos por design, para nao deixar o companion pegajoso
