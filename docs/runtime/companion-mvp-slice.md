# Companion MVP Slice Oficial

Data: 2026-03-19

## Objetivo
Definir o fluxo oficial que prova o NC-OS como companion funcional em hardware real, sem depender de cloud, ASR completo ou investigacoes novas de painel.

## Fluxo oficial do slice
1. **Inicio / idle**
   - Boot completo.
   - Display funcional.
   - Face em estado de idle observacional.
   - Movimento de olhos vivo, mas dentro do envelope visual seguro do painel.
2. **Trigger oficial**
   - Trigger principal e obrigatorio: touch capacitivo local.
   - O touch deve gerar atencao ao usuario sem depender de internet, wake-word ou camera.
   - O trigger local usa histerese simples para evitar oscilacao visual e comportamental perto do limiar.
3. **Resposta oficial**
   - `PerceptionService` promove atencao para `AttentionTarget::kUser`.
   - `BehaviorService` emite `BehaviorProfile::kAttendUser`.
   - `FaceService` sai do neutro e responde com preset/presenca de atencao coerente.
   - `MotionService` acompanha a resposta **somente quando** o transporte TTLinker estiver disponivel e sem conflito.
4. **Retorno ao idle**
   - Soltar o touch e permanecer sem estimulo novo.
   - O sistema publica a transicao de retorno para idle no `Companion State`.
   - Face retorna ao estado basal do companion.
   - Motion retorna ao neutro quando disponivel.

## Escopo oficial do MVP slice

### Entra no slice
- boot local do firmware
- display com face viva e legivel
- trigger local por touch
- resposta visual de atencao ao usuario
- retorno ao idle sem reboot nem travamento
- motion como extensao opcional quando o transporte estiver habilitado

### Nao entra no slice
- conversa por voz completa
- wake-word
- ASR/NLU
- cloud sync
- camera como trigger obrigatorio
- memoria adaptativa
- SD/filesystem como requisito de demonstracao

## Justificativa do trigger oficial
O touch e o trigger mais confiavel para este slice porque:
- ja existe bring-up real em hardware
- nao depende de iluminacao, ruido acustico ou rede
- permite uma demonstracao repetivel
- reduz variaveis externas para provar o companion base

Voz e camera continuam como sinais validos do sistema, mas nao sao gate de aceite do MVP slice.

## Envelope oficial do slice

### Inicio
- companion entra com face ativa e legivel
- gaze de idle pode continuar vivo
- sem exigir motion, audio de resposta ou camera

### Trigger
- toque sustentado acima do limiar local
- `PerceptionService` deve priorizar `AttentionChannel::kTouch`
- `InteractionPhase` esperado: `kActing`

### Resposta
- `BehaviorProfile` esperado: `kAttendUser`
- `ActionDomain` esperado: `kFace`
- face deve ficar perceptivelmente mais atenta que o idle
- motion pode acompanhar, mas a ausencia de TTLinker nao invalida o slice

### Retorno
- sem touch, sem audio forte e sem camera dominante, o sistema deve voltar ao caminho de idle
- o retorno precisa ser suave e legivel, sem parecer travado
- a liberacao do trigger deve gerar uma atualizacao formal de estado, nao apenas silencio

## Janela temporal de referencia
- touch probe: conforme `RuntimeConfig::touch_probe_interval_ms`
- cooldown de behavior: `220 ms`
- atualizacao normal do face: conforme pipeline atual
- stale guard de motion: `1400 ms`
- desacoplamento curto na soltura do touch: `180 ms`

Essas janelas nao sao SLA de produto final; sao a referencia operacional do slice atual.

## Criterio oficial de aceite
O Companion MVP Slice fica aprovado quando, em hardware real:
- o boot chega ao rosto vivo em tela
- um toque local dispara atencao ao usuario
- a resposta facial e claramente diferente do idle
- o sistema volta ao idle ao fim do estimulo
- isso acontece de forma repetivel sem depender de cloud

## Procedimento oficial de validacao em hardware
1. subir o firmware em modo normal, sem diagnosticos ativos
2. aguardar o boot fechar no idle observacional
3. executar pelo menos 5 ciclos completos de:
   - toque local sustentado
   - resposta visual de atencao
   - soltura do touch
   - retorno ao idle
4. considerar o slice aprovado quando, nesses ciclos:
   - nao houver reboot, freeze ou travamento da face
   - o attend aparecer de forma perceptivel durante o toque
   - a liberacao do touch mantiver uma janela curta de desacoplamento, sem flicker
   - o retorno ao idle acontecer de forma limpa e previsivel
   - motion, quando disponivel, acompanhar o attend e voltar ao neutro no fim

## Envelope oficial de retorno ao idle
- a liberacao do touch nao corta a atencao instantaneamente; existe uma janela curta de desacoplamento
- essa janela serve para evitar flicker comportamental e visual na soltura
- sem novo estimulo dominante, o companion deve voltar para `idle`
- o `BehaviorService` deve encerrar o `kAttendUser` e voltar para `kIdleObserve`
- face e motion devem acompanhar esse retorno sem parecer presos no attend

## Trade-offs assumidos
- o slice prova o companion base, nao a experiencia completa de conversa
- touch foi escolhido por confiabilidade, nao por ambicao maxima de produto
- motion fica opcional por causa do transporte atual
- voz/camera seguem preservados no sistema, mas fora do caminho oficial de demonstracao
