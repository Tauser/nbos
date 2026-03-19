# Companion Basic State Machine

## Objetivo

Consolidar uma maquina de estados pequena e oficial do companion em cima do `CompanionSnapshot`, sem criar uma arquitetura paralela.

## Estados principais

- `kBooting`
  - Runtime ainda nao foi iniciado ou nao ficou pronto.
- `kIdleObserve`
  - Estado base do produto em runtime nominal, sem atencao dominante nem resposta ativa.
- `kSleep`
  - Estado de descanso previsivel, com runtime nominal e sem estimulo relevante por tempo suficiente.
- `kAttendUser`
  - Usuario em foco com lock, confianca suficiente ou sessao ativa.
- `kAlertScan`
  - Estimulo do mundo em foco com confianca alta.
- `kResponding`
  - Companion conduzindo a resposta atual.
- `kEnergyProtect`
  - Safe mode, energia critica ou energia restrita severa.

## Prioridade de derivacao

A maquina e derivada nesta ordem:

1. `kBooting`
2. `kEnergyProtect`
3. `kResponding`
4. `kAlertScan`
5. `kAttendUser`
6. `kSleep` quando ja estava dormindo e nao ha wake relevante
7. `kIdleObserve`
8. `kSleep` por decay de idle estavel

## Timers oficiais

- `Responding`: permanencia minima de `900 ms` apos a ultima atividade de resposta.
- `AlertScan`: permanencia minima de `1600 ms` apos a ultima observacao forte de estimulo.
- `AttendUser`: permanencia minima de `1400 ms` apos a ultima atencao valida ao usuario.
- `IdleObserve -> Sleep`: entra em `sleep` apos `12000 ms` de idle estavel sem estimulo forte.

## Eventos oficiais de entrada e saida

- `kBootstrap`
  - entrada inicial no boot.
- `kRuntimeStarted`
  - transicao de boot para idle nominal.
- `kRuntimeStopped`
  - runtime deixou de estar iniciado.
- `kUserTrigger`
  - entrada em `kAttendUser` por touch, voz ou atencao ao usuario.
- `kUserRelease`
  - retorno de `kAttendUser` para `kIdleObserve` quando a permanencia expira sem novo trigger.
- `kStimulusObserved`
  - entrada em `kAlertScan` por foco no mundo.
- `kCompanionResponding`
  - entrada em `kResponding` quando o turno e do companion.
- `kEnergyGuard`
  - entrada em `kEnergyProtect` por limitacao energetica.
- `kRecoveryToIdle`
  - retorno geral para `kIdleObserve` fora do caso especifico de soltura do usuario.
- `kIdleDecayToSleep`
  - entrada em `kSleep` por permanencia longa em idle estavel.

## Criterios de permanencia

- Estados ativos nao caem imediatamente quando o sinal cru some; eles respeitam o `state_hold_until_ms`.
- `Sleep` so entra a partir de `IdleObserve`; nunca substitui `Responding`, `AlertScan`, `AttendUser` ou `EnergyProtect` enquanto esses estiverem validos.
- Qualquer trigger forte de usuario, estimulo ou resposta acorda o companion diretamente para o estado ativo correspondente.

## Envelope de produto

- `Face` e `Motion` podem ler o `product_state` sem precisar reinterpretar sinais fisicos.
- O estado continua derivado do snapshot oficial; nao existe writer novo para a maquina.
- `presence_mode` permanece como leitura resumida e agora segue o estado de produto, nao apenas quantidade de tarefas no scheduler.
