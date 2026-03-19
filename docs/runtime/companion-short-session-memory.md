# Memoria curta de sessao do companion

## Objetivo

A memoria curta de sessao guarda continuidade contextual de curtissimo prazo sem confundir isso com estado instantaneo do companion. Ela existe para manter o companheiro "aquecido" por alguns segundos depois de um gatilho, resposta ou interacao recente.

## O que entra

- ancora de atencao recente (`anchor_target` e `anchor_channel`)
- tom emocional recente (`anchor_tone`)
- estado marcante mais recente (`anchor_state`)
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
- limpa ao entrar em `EnergyProtect`
- limpa quando o runtime nao estiver inicializado ou iniciado
- continua separada do `product_state`: o companion pode voltar para `IdleObserve` e ainda permanecer com memoria curta aquecida

## Leitores autorizados

A memoria curta fica disponivel para runtime, behavior, face, motion, voz e diagnostico. Ela e redatada para superficies que nao precisam desse contexto, como bridge de nuvem.
