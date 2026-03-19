# Companion Basic State Machine

## Objetivo

Consolidar uma maquina de estados pequena e oficial do companion em cima do `CompanionSnapshot`, sem criar uma arquitetura paralela.

## Estados principais

- `kBooting`
  - Runtime ainda nao foi iniciado ou nao ficou pronto.
- `kIdleObserve`
  - Estado base do produto em runtime nominal, sem atencao dominante nem resposta ativa.
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
6. `kIdleObserve`

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
  - retorno de `kAttendUser` para `kIdleObserve` quando o estimulo cessa.
- `kStimulusObserved`
  - entrada em `kAlertScan` por foco no mundo.
- `kCompanionResponding`
  - entrada em `kResponding` quando o turno e do companion.
- `kEnergyGuard`
  - entrada em `kEnergyProtect` por limitacao energetica.
- `kRecoveryToIdle`
  - retorno geral para `kIdleObserve` fora do caso especifico de soltura do usuario.

## Envelope de produto

- `Face` e `Motion` podem ler o `product_state` sem precisar reinterpretar sinais fisicos.
- O estado continua derivado do snapshot oficial; nao existe writer novo para a maquina.
- `presence_mode` permanece como leitura resumida e agora segue o estado de produto, nao apenas quantidade de tarefas no scheduler.
