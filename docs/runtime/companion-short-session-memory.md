# Memoria curta de sessao do companion

## Objetivo

A memoria curta de sessao guarda continuidade contextual de curtissimo prazo sem confundir isso com estado instantaneo do companion. Ela existe para manter o companheiro "aquecido" por alguns segundos depois de um gatilho, resposta ou interacao recente.

## O que entra

- ancora de atencao recente (`anchor_target` e `anchor_channel`)
- ultimo estimulo relevante (`recent_stimulus`) com alvo, canal, confianca e timestamp
- tom emocional recente (`anchor_tone`)
- estado marcante mais recente (`anchor_state`)
- contexto imediato da interacao (`recent_interaction`) com fase, dono de turno e `response_pending`
- engajamento recente (`engagement_recent_percent`) e seu ultimo timestamp
- ultimo dono de turno (`last_turn_owner`)
- contagem curta de gatilhos do usuario e respostas do companion
- timestamps da abertura, ultima atividade e janela de retencao

## O que nao entra

- estado instantaneo atual de runtime, attention, emotion ou interaction
- persistencia em NVS
- exportacao para nuvem
- historico longo ou memoria adaptativa

## Envelope oficial

- retencao curta: `18 s` apos a ultima atividade relevante
- janela perceptivel de continuidade para reengajamento com usuario: `3,2 s`
- janela perceptivel de continuidade para estimulo de mundo: `2,4 s`
- depois dessas janelas, a memoria curta continua existindo como contexto de sessao, mas face/motion/behavior voltam gradualmente ao idle basal
- limpa ao entrar em `EnergyProtect`
- limpa quando o runtime nao estiver inicializado ou iniciado
- continua separada do `product_state`: o companion pode voltar para `IdleObserve` e ainda permanecer com memoria curta aquecida

## Leitores autorizados

A memoria curta fica disponivel para runtime, behavior, face, motion, voz e diagnostico. Ela e redatada para superficies que nao precisam desse contexto, como bridge de nuvem.

## Leitura de produto

- `recent_stimulus` responde "o que chamou a atencao ha pouco"
- `engagement_recent_percent` responde "quao engajado o companion esteve ha pouco"
- `recent_interaction` responde "em que tipo de troca ele acabou de estar"

## Cenarios reais validados

- toque do usuario -> idle curto -> novo toque dentro de `3,2 s`: o companion reengaja sem parecer frio
- resposta do companion -> idle curto: face e motion ainda seguram calor contextual por alguns instantes
- estimulo de mundo -> idle: curiosidade residual existe por pouco tempo e nao prende o companion por toda a retencao
- sessao ainda aquecida, mas fora da janela perceptivel: o contexto continua disponivel para diagnostico e estado, mas a expressao volta ao basal
