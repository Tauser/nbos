# Runtime Readiness Review - Fase 2

## Escopo revisado
- Boot flow (`BootFlow`)
- Lifecycle (`SystemLifecycle`)
- Runtime core (`SystemManager`, `SchedulerBase`, `Health`, `Diagnostics`, `SafeMode`, `FaultHistory`)
- Configuracao centralizada integrada (`GlobalConfig`)

## Criterios explicitos de readiness
1. `config_valid`: configuracao centralizada valida (`config_ready=true`).
2. `board_profile_bound`: board profile populado com nome e pinos base do runtime.
3. `lifecycle_allows_runtime`: lifecycle em `running` ou `degraded`.
4. `runtime_initialized`: `SystemManager` inicializado com lifecycle + config.
5. `runtime_started`: runtime iniciado.
6. `scheduler_has_minimum_tasks`: ao menos 1 tarefa registrada (watchdog).
7. `safe_mode_inactive`: sem fallback ativo.
8. `no_faults_recorded`: historico de falhas vazio.

## Regra de classificacao
- `ready`: todos os criterios acima verdadeiros.
- `conditionally_ready`: gate core verdadeiro (`1..6`), mas com `safe_mode_inactive=false` ou `no_faults_recorded=false`.
- `not_ready`: qualquer falha no gate core (`1..6`).

## Checks minimos implementados
- `RuntimeStatus` em `SystemManager` para snapshot agregada do runtime.
- `RuntimeReadinessReport` + `evaluate_runtime_readiness(...)` com classificacao explicita.
- Log de readiness no final do boot (entrypoint) com o resultado e flags.
- Testes nativos cobrindo casos `ready`, `conditionally_ready` e `not_ready`.

## Resultado atual da Fase 2
- Estado declarado: `conditionally_ready` esperado em cenarios com warning de boot.
- Justificativa: runtime sobe com governanca base, mas pode manter warnings/falhas nao criticas no baseline atual.

## Riscos abertos
- Boot atual marca warning em TTLinker por adiamento (impacta lifecycle para `degraded`).
- Falhas nao criticas no boot ainda nao possuem arbitragem mais fina por subsistema.
- Readiness ainda nao persiste historico entre boots (somente RAM nesta fase).

## Proximos gates recomendados
- Gate R3: politica clara de transicao de `degraded` para `running` apos estabilizacao.
- Gate R4: persistencia minima de fault history para diagnostico pos-reboot.
