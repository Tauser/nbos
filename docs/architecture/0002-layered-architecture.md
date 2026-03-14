# ADR-0002: Arquitetura em Camadas

- Status: Accepted
- Data: 2026-03-14

## Contexto
O projeto precisa crescer por subsistemas (Face, Emotion, Behavior, Motion, Vision, Voice, Power, Cloud Bridge) sem virar monolito.

## Decisao
Adotar arquitetura em camadas com fronteiras explicitas:

- `app`: bootstrap, wiring, ciclo de vida
- `core`: estado, eventos, contratos centrais
- `interfaces`: portas abstratas entre dominio e hardware
- `drivers`: implementacoes concretas de perifericos
- `hal`: acesso de baixo nivel a barramentos e pinos
- `services`: orquestracao de dominio
- `ai`: inferencia e politicas
- `models`: modelos de estado e dados do companion
- `utils`: logging, diagnostico e helpers
- `config`: pinos, perfis e feature flags

## Regras obrigatorias
1. Servicos nao acessam HAL diretamente; usam `interfaces`.
2. Drivers nao contem regra de negocio.
3. Rendering nao decide semantica de comportamento.
4. Acoplamento entre camadas deve ocorrer por contrato explicito.
5. Mudancas estruturais relevantes exigem ADR.

## Consequencias
- Mais disciplina no inicio.
- Menor risco de regressao e retrabalho nas fases premium do Face Engine.
