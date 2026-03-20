# Handoff - Fase 2

## Objetivo deste documento

Este documento existe para servir como contexto de continuidade para outra IA ou colaborador humano.

Ele resume:
- o que a Fase 2 consolidou
- quais decisoes ja foram tomadas
- quais checkpoints ja existem
- qual e o estado tecnico esperado ao final da fase
- o que ainda ficou de fora de proposito

## Resumo executivo

A Fase 2 consolidou o companion como produto real em tres camadas:
- autonomia basica previsivel
- memoria curta e memoria persistente separadas e auditaveis
- identidade/persona estavel com adaptacao bounded

Ao final da fase:
- `Companion State` e a fonte central de verdade
- memoria curta continua volatil e separada da persistencia
- personalidade base e fixa e central
- adaptacao continua pequena, bounded e rastreavel
- historico persistente modula comportamento e expressao apenas via helpers centrais
- `face` e `motion` nao leem storage direto

## Regras arquiteturais que ja estao valendo

Nao reabrir estas decisoes sem motivo real:
- nao criar subsistema paralelo de persona
- nao acoplar `behavior`, `face`, `motion` ou `routine` diretamente ao storage
- nao misturar memoria curta de sessao com memoria persistente
- nao permitir que adaptacao mude tracos fixos da identidade
- nao espalhar tuning local quando ja existe helper central no estado/contrato

## Fonte de verdade atual

Arquivos centrais para continuidade:
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_state_contracts.hpp)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\state\companion_state_store.cpp)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_personality_contracts.hpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\core\contracts\companion_personality_contracts.hpp)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\behavior\behavior_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\behavior\behavior_service.cpp)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\routine\routine_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\routine\routine_service.cpp)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\face\face_graphics_pipeline.cpp)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\motion\motion_service.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\services\motion\motion_service.cpp)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\storage\persistent_companion_memory_store.cpp](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\src\drivers\storage\persistent_companion_memory_store.cpp)

Documentacao de apoio:
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\companion-short-session-memory.md](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\companion-short-session-memory.md)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\companion-personality-baseline.md](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\companion-personality-baseline.md)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\companion-persistent-memory.md](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\companion-persistent-memory.md)
- [C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\storage-persistence-base.md](C:\Users\Tauser\Documents\PlatformIO\Projects\NBOS\docs\runtime\storage-persistence-base.md)

## Linha do tempo da Fase 2

### 16.x - autonomia basica

#### 16.3 - validar previsibilidade da autonomia basica
- endureceu transicoes `sleep -> wake -> idle`
- validou preempcao por `EnergyProtect`
- validou retorno limpo ao basal
- checkpoint: `58f7aaf` `test(companion): validar previsibilidade da autonomia`

### 17.x - memoria curta e continuidade de sessao

#### 17.1 - consolidar memoria curta de sessao
- criou camada explicita de sessao curta
- separou memoria curta de estado instantaneo
- manteve tudo efemero no `CompanionStateStore`
- checkpoint: `5cf20bd` `feat(companion): consolidar memoria curta de sessao`

#### 17.2 - consolidar historico recente de interacao
- adicionou ultimo estimulo relevante
- adicionou engajamento recente
- adicionou contexto imediato da interacao
- checkpoint: `deebc3c` `feat(companion): consolidar historico recente de interacao`

#### 17.3 - operacionalizar continuidade contextual
- memoria curta passou a modular `behavior`, `face` e `motion`
- evitou respostas frias entre interacoes proximas
- checkpoint: `8796d33` `feat(companion): operacionalizar continuidade contextual`

#### 17.4 - validar continuidade de sessao
- testou cenarios reais de sessao curta
- separou retencao total da janela perceptivel
- checkpoint: `68eaeff` `test(companion): validar continuidade de sessao`

### 18.x - identidade base do personagem

#### 18.1 - consolidar tracos-base da personalidade
- oficializou:
  - calor afiliativo
  - curiosidade serena
  - compostura premium
  - iniciativa moderada
  - assertividade moderada
- checkpoint: `042f04a` `feat(companion): consolidar tracos-base da personalidade`

#### 18.2 - operacionalizar expressao, energia e sociabilidade
- traduziu os tracos para `face`, `motion` e timing curto
- centralizou os parametros no contrato de personalidade
- checkpoint: `5b15925` `feat(companion): operacionalizar expressao e sociabilidade`

#### 18.3 - consolidar personalidade base no Companion State
- moveu a personalidade oficial para o `Companion State`
- `behavior`, `face`, `motion` e `emotion` passaram a ler o mesmo baseline
- checkpoint: `2672fa3` `refactor(companion): consolidar personalidade no estado central`

#### 18.4 - validar consistencia da identidade
- alinhou a mesma regua de continuidade curta entre canais
- removeu contradicoes perceptivas obvias
- checkpoint: `7a8b2f6` `test(companion): validar consistencia da identidade`

### 19.x - adaptacao bounded

#### 19.1 - consolidar parametros adaptaveis
- separou:
  - identidade fixa
  - envelope adaptavel estreito
- parametros adaptaveis oficiais:
  - `adaptive_social_warmth_bias_percent`
  - `adaptive_response_energy_bias_percent`
  - `adaptive_continuity_window_bias_ms`
- checkpoint: `9f1594a` `feat(companion): consolidar parametros adaptaveis`

#### 19.2 - consolidar regras de adaptacao bounded
- store central ganhou regras de passo, convergencia e decay
- `Sleep` e `EnergyProtect` puxam para perfis conservadores
- checkpoint: `c2f3c53` `feat(companion): consolidar regras de adaptacao bounded`

#### 19.3 - operacionalizar adaptacao em comportamento e expressao
- adaptacao passou a aparecer em `behavior`, `face` e `motion`
- tudo continuou vindo do envelope central
- checkpoint: `2983043` `feat(companion): operacionalizar adaptacao em comportamento e expressao`

#### 19.4 - validar que a adaptacao nao vira ruido
- validou perfis distintos e overshoot bounded
- provou que a identidade continua reconhecivel
- checkpoint: `369373a` `test(companion): validar adaptacao sem ruido`

### 20.x - persistencia segura

#### 20.1 - definir politica de persistencia, retencao e dados
- oficializou o que persiste e o que continua volatil
- checkpoint: `a9e5dc2` `feat(storage): definir politica de persistencia`

#### 20.2 - consolidar schema versionado e escrita atomica
- adicionou schema versionado, checksum e slots A/B
- preservou `last-known-good`
- checkpoint: `254805c` `feat(storage): endurecer persistencia com escrita atomica`

#### 20.3 - operacionalizar fallback e recuperacao
- `load_with_recovery()`
- `reset_profile()`
- fallback para snapshot valido anterior
- checkpoint: `f23a9c8` `feat(storage): operacionalizar fallback de persistencia`

#### 20.4 - validar persistencia segura
- testou reboot inesperado, corrupcao simulada e export/import do que ja existia
- checkpoint: `822641f` `test(storage): validar persistencia segura`

### 21.x - memoria persistente do companion

#### 21.1 - consolidar memoria persistente de preferencias e habitos
- criou memoria persistente propria do companion
- estrutura:
  - preferencias
  - habitos agregados bounded
  - eventos marcantes compactos
- checkpoint: `684116e` `feat(storage): consolidar memoria persistente do companion`

#### 21.2 - consolidar storage persistente e versionado
- endureceu o store da memoria persistente
- migrou para fluxo robusto com recovery e legado
- checkpoint: `1d41458` `feat(storage): endurecer storage da memoria persistente`

#### 21.3 - operacionalizar memoria persistente no perfil adaptativo
- memoria longa passou a alimentar o baseline adaptativo central
- storage continuou fora dos servicos de produto
- checkpoint: `8f8ef15` `feat(companion): operacionalizar memoria persistente no perfil`

#### 21.4 - validar consistencia entre sessoes
- provou continuidade entre boots/logical sessions
- validou preferencias recuperadas e updates saneados
- checkpoint: `5fd24a2` `test(storage): validar consistencia entre sessoes`

### 22.x - modulacao longa em comportamento e expressao

#### 22.1 - consolidar onde o historico modula o comportamento
- mapeou influencia do historico em:
  - rotina
  - atencao
  - iniciativa
- checkpoint: `93355ef` `feat(companion): modular historico em rotina e atencao`

#### 22.2 - operacionalizar modulacao de rotina e iniciativa
- modulou cooldowns e cadencia de iniciativa
- checkpoint: `ea23dee` `feat(companion): modular cadencia de rotina e iniciativa`

#### 22.3 - consolidar modulacao comportamental em face e motion
- historico longo passou a aparecer tambem em `face` e `motion`
- somente em idle e sempre abaixo de contexto curto/governanca forte
- checkpoint: `c267294` `feat(companion): modular historico em face e motion`

#### 22.4 - validar previsibilidade do comportamento adaptativo
- reduziu o peso do historico longo
- saturacao agora acontece mais cedo
- adicionou testes para provar:
  - perfis distintos continuam distinguiveis
  - diferencas entre perfis continuam pequenas
  - a adaptacao nao parece outro personagem
- estado de validacao:
  - `pio test -e native --filter native/test_companion_personality_baseline` passou
  - `pio test -e native --without-uploading --without-testing` passou para a suite nativa inteira
  - execucao isolada de `native/test_motion_service` teve ruido de ambiente Windows (`WinError 5`), mas o mesmo alvo compilou no build nativo completo

## Semantica consolidada ao fim da Fase 2

### Memoria curta
- vive em `CompanionSnapshot.session`
- e volatil
- serve para continuidade perceptivel de curto prazo
- nao deve ser persistida nesta fase

### Memoria persistente
- guarda preferencias, habitos e eventos marcantes compactos
- e local, versionada, auditavel e recuperavel
- nao e log bruto nem timeline longa

### Personalidade base
- e fixa
- define a identidade do companion
- nao deve ser alterada por adaptacao nem por historico

### Adaptacao
- e bounded
- deriva de sessao curta e baseline persistente
- modula envelope efetivo, nunca tracos fixos

### Historico longo
- modula comportamento e expressao apenas via helpers centrais
- nao deve virar politica solta em servico
- nao deve ter peso grande o suficiente para descaracterizar o personagem

## Estado tecnico esperado quando outra IA assumir

A outra IA deve partir das seguintes premissas:
- o sistema ja tem memoria curta funcional
- o sistema ja tem memoria persistente funcional
- o perfil adaptativo ja recebe baseline persistente
- `behavior`, `routine`, `face` e `motion` ja refletem historico longo de forma sutil
- os clamps e helpers centrais sao a regua oficial para qualquer novo ajuste

## O que ficou fora de proposito

Nao assumir que estas partes ja estao prontas:
- politica longa de aprendizado
- escrita automatica rica da memoria persistente a partir do runtime
- export/import aberto da memoria persistente do companion
- persistencia de memoria curta de sessao
- modulacao longa forte em estados ativos
- qualquer acoplamento direto de `face`/`motion`/`behavior` ao storage

## Riscos residuais conhecidos

- validacao nativa ampla no Windows as vezes sofre ruido de ambiente com lock/acesso em `.pio` ou `program.exe`
- build de alvo ESP pode sofrer problemas externos de toolchain/ambiente nao relacionados a esta fase
- existem mudancas pre-existentes no workspace que nao devem ser revertidas silenciosamente

## Regras para a proxima IA continuar sem quebrar a fase

1. Ler primeiro os contratos centrais e as docs de runtime.
2. Tratar a Fase 2 como base real, nao como espaco vazio.
3. Nao criar store paralelo para memoria ou personalidade.
4. Nao duplicar parametros locais se ja existir helper central.
5. Se precisar modular `face`, `motion`, `behavior` ou `routine`, passar pelo envelope central.
6. Se abrir adaptacao nova, manter bounded, com clamp e testes de previsibilidade.
7. Antes de expandir persistencia, preservar recovery, checksum, schema versionado e `last-known-good`.

## Prompt de handoff pronto para outra IA

Use o texto abaixo como prompt-base:

```text
Voce esta trabalhando no projeto NC-OS.

Considere a Fase 2 como concluida e real. Nao recrie arquitetura do zero.

Estado consolidado:
- `Companion State` e a fonte central de verdade
- memoria curta de sessao e volatil e separada da memoria persistente
- personalidade base e fixa e central
- adaptacao e bounded e nao altera tracos fixos
- memoria persistente do companion guarda preferencias, habitos e eventos marcantes compactos
- historico persistente modula comportamento e expressao apenas via helpers centrais
- `behavior`, `routine`, `face` e `motion` nao leem storage direto

Arquivos principais:
- src/core/contracts/companion_state_contracts.hpp
- src/core/state/companion_state_store.cpp
- src/core/contracts/companion_personality_contracts.hpp
- src/services/behavior/behavior_service.cpp
- src/services/routine/routine_service.cpp
- src/services/face/face_graphics_pipeline.cpp
- src/services/motion/motion_service.cpp
- src/drivers/storage/persistent_companion_memory_store.cpp

Documentacao principal:
- docs/runtime/companion-short-session-memory.md
- docs/runtime/companion-personality-baseline.md
- docs/runtime/companion-persistent-memory.md
- docs/runtime/storage-persistence-base.md
- docs/runtime/fase2-handoff.md

Regras:
- preservar arquitetura consolidada
- focar em consolidacao, endurecimento e produto real
- evitar duplicacao de parametros
- evitar acoplamento direto com storage
- toda modulacao nova precisa continuar bounded e previsivel
```

## Observacao final

Este handoff foi feito para continuidade tecnica, nao para auditoria completa de diff por arquivo.

Se outra IA for atuar na proxima fase, o caminho recomendado e:
- ler este documento
- ler os contratos centrais
- ler a documentacao de runtime
- so depois propor a menor mudanca segura
