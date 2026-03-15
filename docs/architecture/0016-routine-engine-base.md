# 0016 - Routine engine base com idle routines e attention modes

## Objetivo
Criar uma base formal para rotina local do robô que:
- evita aparência "morta" quando não há interação forte
- mantém alinhamento com Behavior, Emotion e Companion State
- continua submetida à Action Governance

## Decisão
Routine nasce como `RoutineService` com três responsabilidades:
1. Derivar `AttentionMode` a partir do `CompanionSnapshot`.
2. Selecionar `IdleRoutine` compatível com esse modo.
3. Emitir `ActionProposal` de baixa/média prioridade e passar pela governança.

## Attention modes iniciais
- `kAmbient`: variação leve de presença em idle.
- `kUserEngaged`: micro presença quando foco no usuário.
- `kStimulusTracking`: nudge de varredura para estímulo ambiental.
- `kEnergyConserve`: settle suave sob restrição energética/safe mode.

## Idle routines iniciais
- `kAmbientGazeSweep`
- `kUserPresencePulse`
- `kStimulusScanNudge`
- `kEnergySaveSettle`

## Fronteiras arquiteturais
- Routine não arbitra ownership de domínio: isso continua em `ActionGovernor`.
- Routine não chama internals de Face/Motion; apenas emite proposta semântica.
- Routine suprime emissão por janela curta quando Behavior acabou de assumir ação,
  reduzindo conflito e ruído.

## Trade-offs
- Pro: vida local surge cedo sem script rígido nem hacks por subsistema.
- Pro: attention/idle crescem incrementalmente com testes.
- Contra: ainda sem scheduler semântico rico para sequências longas de rotina.
