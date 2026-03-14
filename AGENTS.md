# AGENTS.md

## Identidade do projeto
NC-OS e um firmware embarcado para um robô desktop companion premium.

Direção do produto:
- offline-first por padrão
- nuvem como extensão opcional
- arquitetura em camadas, sustentável e testável
- evolução incremental, sem atalhos estruturais

## Regras duráveis para agentes
1. Não criar monólitos.
2. Não misturar regra de negócio com acesso a hardware.
3. Não misturar rendering com decisão semântica.
4. Não misturar driver com comportamento de produto.
5. Toda abstração deve ter motivo real.
6. Preferir contratos explícitos a acoplamento implícito.
7. Toda decisão relevante deve declarar trade-offs.
8. Não fazer refactor massivo sem necessidade.
9. Manter checkpoints pequenos e revisáveis.
10. Preservar build saudável sempre que razoável.

## Stack-base congelada
- ESP-IDF como framework principal
- PlatformIO para build/orquestração
- LovyanGFX para display ST7789

Se surgir necessidade de compatibilidade Arduino, tratar como integração isolada, não como base arquitetural.

## Arquitetura oficial (em `src/`)
- `app`: bootstrap e wiring
- `core`: estado/eventos/contratos centrais
- `interfaces`: portas entre domínio e infraestrutura
- `drivers`: implementações de periféricos
- `hal`: fronteira de hardware/board
- `services`: orquestração de subsistemas
- `ai`: inferência e políticas locais
- `models`: modelos e tipos de domínio
- `utils`: utilitários compartilhados
- `config`: pinagem, perfis e feature flags

## Hardware congelado
Base oficial:
- Freenove ESP32-S3-WROOM CAM N16R8
- ST7789, OV2640, MAX98357A, INMP441, MPU6050, TTLinker, touch capacitivo
- pinagem de referência documentada em `docs/architecture/0003-engineering-principles-offline-first.md`

Atenção: mapeamento provisório do MPU6050 em GPIO0/GPIO19 exige cautela de boot/debug; não mudar silenciosamente.

## Fluxo padrão de execução
Para tarefas não triviais:
1. ler estado atual
2. propor menor mudança segura
3. listar arquivos a alterar/criar
4. explicar riscos e trade-offs
5. implementar escopo pedido
6. validar (build/teste quando aplicável)
7. consolidar checkpoint

## Política de commit
Seguir `docs/process/commit-policy.md`.

Padrão:
- 1 prompt concluído = 1 commit significativo (ou mais de um quando separação técnica fizer sentido)
- mensagem: `tipo(escopo): descrição curta`
- idioma: `pt_BR`

## Face Engine (regra estratégica)
Antes de F1, respeitar gate arquitetural:
- estado de render explícito
- propriedade clara de camadas
- arbitragem de composição governada
- FaceService como fachada/orquestrador (não monólito)

Sem esse gate, não avançar para geometria/presets complexos.
