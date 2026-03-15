# 0022 - Estrategia de OTA, rollback e fallback operacional

## Objetivo
Criar um trilho seguro de resiliencia para atualizacao de firmware, evitando OTA remoto sem recuperacao e garantindo comportamento previsivel quando a imagem nova nao confirma estabilidade.

## Decisao
- `UpdateService` passa a ser o orquestrador de politica de atualizacao, separado de driver e runtime core.
- `UpdatePort` define contrato explicito de boot info, confirmacao de imagem e rollback.
- OTA remoto so e permitido quando `rollback_supported=true`; caso contrario, o sistema bloqueia politicamente e entra em fallback seguro.
- Imagem em `pending_verify` recebe janela de confirmacao por uptime (`ota_confirm_uptime_ms`).
- Se a janela expira em `safe_mode`, o sistema entra em `fallback operacional` (modo degradado, sem assumir imagem como valida).
- Confirmacao de imagem so acontece quando runtime esta iniciado e fora de `safe_mode`.

## Resultado arquitetural
- OTA deixa de ser fluxo implcito e passa a ter ownership formal (`UpdateService`).
- rollback/fallback ficam observaveis por estado (`UpdateRuntimeState`) e por fault history.
- runtime preserva separacao de papeis: `UpdateService` decide politica de update, `SystemManager` aplica resposta operacional.

## Trilho seguro inicial
- base local preparada para confirmar/rollback via ESP-IDF (`esp_ota_ops`).
- OTA remoto permanece desligado por default em `dev` e `prod` ate pipeline assinado.
- fallback operacional prioriza continuidade segura do robo antes de ativar qualquer canal remoto.

## Fora de escopo
- distribuicao remota de artefatos OTA
- assinatura/verificacao criptografica de pacotes
- rollout progressivo por coorte
