# Revisao de robustez de produto - stress/soak baseline

## Escopo desta etapa
- Stress test de barramento semantico e governanca de acoes.
- Soak test de Companion State com alta quantidade de ciclos de ingestao.
- Revisao de risco residual para operacao prolongada.

## Evidencias adicionadas
- `test/native/test_runtime_stress/test_main.cpp`
- `test/native/test_runtime_soak/test_main.cpp`

## Criterios avaliados
- Contabilidade de perda/entrega no EventBus sob carga.
- Integridade de lease e ownership no ActionGovernor sob churn de conflito.
- Integridade de revisao, limites semanticos e estado transitivo no CompanionStateStore sob longa execucao simulada.

## Resultado esperado para declarar baseline robusta
- Stress:
  - mensagens publicadas+dropped batem exatamente com volume tentado;
  - eventos despachados batem com handlers contabilizados;
  - governanca mantem lease valido e ownership coerente durante churn.
- Soak:
  - revisao do Companion State cresce de forma deterministica;
  - clamp emocional permanece dentro de limites;
  - estado transiente permanece consistente apos milhares de ciclos.

## Riscos remanescentes (nao cobertos integralmente aqui)
- Ausencia de soak em hardware real com camera/audio/servo concorrentes por horas.
- Falta de metricas de heap fragmentation e latencia p99 em execucao prolongada.
- Falta de fault injection fisico (brownout real, ruido de alimentacao, perda de periferico em runtime).

## Proximos passos recomendados
1. Rodar soak de bancada de 2-4h com monitoramento de heap livre e watchdog.
2. Adicionar fault injection controlado para energia e perifericos criticos.
3. Registrar budget de latencia por subsistema para detectar regressao de tempo de resposta.
