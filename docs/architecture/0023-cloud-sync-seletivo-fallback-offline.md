# 0023 - Sync seletivo de cloud com fallback offline seguro

## Objetivo
Permitir extensao de nuvem sem quebrar o comportamento local offline-first do NC-OS.

## Decisao
- `CloudSyncService` passa a orquestrar sincronizacao remota de estado agregado.
- Cloud permanece opcional por politica (`cloud_sync_enabled=false` por default).
- Sync e seletivo por semantica:
  - sempre envia `runtime`, `governance` e `energetic`;
  - envia `attentional`, `interactional` e `emotional` apenas quando relevantes.
- Falhas consecutivas de cloud ativam `offline_fallback_active` sem bloquear caminho local.
- Recuperacao limpa automaticamente o estado degradado quando o envio volta a funcionar.

## Resultado arquitetural
- cloud complementa o produto sem virar dependencia principal.
- caminho local continua governando estado, face, motion e comportamento mesmo com cloud indisponivel.
- degradacao fica explicita e observavel via `CloudSyncRuntimeState`.

## Fora de escopo
- transporte real de rede (TLS, broker, filas distribuidas)
- controle de schema/versionamento remoto
- retry distribuido multi-endpoint
