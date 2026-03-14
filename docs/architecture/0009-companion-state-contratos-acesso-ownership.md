# ADR-0009: Contratos de leitura, escrita e ownership do Companion State

- Status: Accepted
- Data: 2026-03-14

## Objetivo
Impedir que o Companion State vire objeto global mutavel por qualquer modulo e explicitar ownership de acesso por dominio.

## Decisao
Acesso ao Companion State passa a ser controlado por contratos formais:
- Escrita exige `CompanionStateWriter` explicito em cada operacao.
- Leitura exige `CompanionStateReader` explicito via `snapshot_for(...)`.
- Policy centralizada em contratos:
  - `can_writer_mutate_domain(writer, domain)`
  - `can_reader_observe_domain(reader, domain)`
  - `redact_snapshot_for_reader(snapshot, reader)`

## Ownership de escrita por dominio
- `kBootstrap` -> `kStructural`
- `kRuntimeCore` -> `kRuntime`, `kGovernance`
- `kGovernanceCore` -> `kTransient`
- `kEmotionService` -> `kEmotional`
- `kAttentionService` -> `kAttentional`
- `kPowerService` -> `kEnergetic`
- `kInteractionService` -> `kInteractional`

Escritas fora desse ownership retornam `false` e nao alteram snapshot.

## Contrato de leitura
Leitura nao e livre; cada reader recebe somente os dominios permitidos.
Dominios nao permitidos sao redigidos para default sem quebrar formato do snapshot.

## Integridade sistemica preservada
- Ownership explicito elimina escrita difusa e acoplamento implicito.
- Runtime continua podendo impor coerencia de seguranca (safe mode) como regra interna de integridade.
- Contrato continua evolutivo: novos dominios/readers/writers entram por extensao da policy central.

## Trade-offs
- Beneficio: disciplina arquitetural e rastreabilidade de autoria.
- Custo: mais parametros e necessidade de declarar origem em todos os pontos de escrita/leitura.
- Decisao: manter retorno booleano simples nesta fase para evitar framework pesado de autorizacao.
