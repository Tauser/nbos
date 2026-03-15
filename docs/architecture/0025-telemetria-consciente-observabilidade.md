# 0025 - Telemetria consciente e observabilidade madura

## Objetivo
Amadurecer observabilidade operacional sem tornar coleta invasiva, mantendo NC-OS offline-first.

## Decisao
- Introduzir `TelemetryService` com contrato explicito de coleta (`TelemetryCollectionSurface`).
- Separar politica de coleta no `RuntimeConfig`:
  - `telemetry_enabled`
  - `telemetry_export_off_device`
  - `telemetry_collect_interactional`
  - `telemetry_collect_emotional`
  - `telemetry_collect_transient`
- Adotar coleta minima por default:
  - runtime, governanca, energetic e estado cloud
  - sem dados interactional/emotional/transient por default
- Tornar exportacao remota opt-in (`telemetry_export_off_device=false` por default).

## Resultado arquitetural
- observabilidade evolui com semantica clara e superficie revisavel.
- telemetria deixa de ser "log solto" e passa a ser contrato testavel.
- coleta sensivel permanece desativada ate habilitacao explicita.

## Riscos e trade-offs
- Mais uma camada de service/contrato para manter.
- Sem backend remoto real nesta etapa (driver local stub).
- Coleta minima pode limitar diagnostico de casos profundos ate politica ser ajustada.

## Fora de escopo
- PII, payload de audio/video cru, historico textual de interacoes.
- pipeline remoto completo de retenção/consulta.
- analise estatistica cloud-side.
