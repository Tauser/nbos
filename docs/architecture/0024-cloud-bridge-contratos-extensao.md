# 0024 - Cloud bridge e contratos de extensao controlada

## Objetivo
Introduzir um `cloud bridge` formal para extensoes remotas sem quebrar o principio offline-first do NC-OS.

## Decisao
- Criar `CloudBridgeService` como fronteira entre runtime local e cloud.
- Separar transporte de sync (`CloudSyncPort`) de extensao (`CloudExtensionPort`).
- Consolidar estado em `CloudBridgeRuntimeState` mantendo `offline_authoritative=true` mesmo quando cloud esta ativa.
- Habilitacao explicita por config:
  - `cloud_bridge_enabled`
  - `cloud_extension_enabled`
- Regras de seguranca de extensao:
  - request valido (`trace_id`, `ttl`)
  - bridge conectado e nao degradado
  - TTL nao expirado
  - fallback local preservado em qualquer falha

## Resultado arquitetural
- cloud passa a ser extensao controlada, nao dependencia principal.
- sincronizacao de estado e extensoes remotas ficam em contratos diferentes.
- runtime continua operando localmente sob degradacao de cloud.

## Trade-offs
- Mais um service e contrato para manter.
- Sem transporte real remoto nesta etapa (driver local stub).
- Extensao permanece conservadora por default (desligada), exigindo habilitacao explicita em ambiente de experimento.

## Fora de escopo
- politica de confianca/assinatura de payload remoto
- resolucao de conflitos semanticos avancada por dominio
- fila persistente de retries de extensao
