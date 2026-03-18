# 0026 - Tooling final, polish premium e revisão cross-subsystem

## Status
Aceito

## Contexto
O NC-OS já possui múltiplos subsistemas ativos (Companion State, Face, Motion, Voice, Vision, Power, Cloud e Governança). O risco nesta fase é fechar produto com incoerências silenciosas entre sinais internos e expressão externa.

## Decisão
Adicionar uma revisão cross-subsystem leve e periódica no runtime, com:
- score de polish (0..100)
- flags explícitas de divergência
- export JSON para inspeção
- log periódico em baixa frequência

A revisão avalia:
- alinhamento de safe mode entre Companion State e Motion
- coerência em energia crítica (proteção comportamental)
- coerência de atenção entre percepção/voz e estado agregado
- coerência entre sinal facial e sinal enviado ao motion
- manutenção de autoridade offline-first no bridge de cloud

## Consequências
### Positivas
- aumenta observabilidade de integração real, não só de serviço isolado
- facilita tuning premium sem acoplamento circular
- cria checkpoint objetivo para regressões cross-subsystem

### Trade-offs
- score é heurístico e não substitui validação perceptiva em hardware
- adiciona pequeno custo de CPU/log (mitigado por periodicidade)

## Fora de escopo
- não transforma revisão em política de arbitragem
- não substitui Action Governance
- não redefine modelos de domínio dos subsistemas
