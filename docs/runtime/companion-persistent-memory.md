# Companion Persistent Memory

## Objective

This checkpoint introduces a persistent companion-memory record that stays separate from short-session memory.

It exists to keep a small, auditable layer of long-lived context without turning the product into a raw event logger.

## What Persists

The record is split into three sections:
- preferences
- habits
- marked events

## Preferences

Preferences represent bounded long-lived tendencies:
- social warmth preference
- response energy preference
- stimulus sensitivity
- preferred attention channel

These are not personality traits.
They are bounded operating preferences that may adapt later without redefining the companion identity.

## Habits

Habits represent aggregate repeated patterns, not a session transcript:
- touch engagement affinity
- repeat engagement affinity
- calm recovery affinity
- preferred engagement window
- reinforced session count

This keeps the data useful for continuity while staying compact and auditable.

## Marked Events

Marked events store only a few notable anchors:
- last user event
- last companion event
- last environment event

Each anchor stores:
- semantic kind
- semantic target
- channel
- salience
- reinforcement count
- last revision token

This is deliberately not a timeline database.

## Separation From Session Memory

Session memory remains in `CompanionSnapshot.session` and is still:
- volatile
- warm-context oriented
- tied to the active runtime window

Persistent companion memory is instead:
- local storage backed
- cross-reboot
- resettable on user/factory reset
- schema-versioned and checksum-protected
- stored in a dual-slot local layout with a legacy migration path

## Defaults and Integrity

The baseline safe profile is intentionally neutral:
- preference percents default to `50`
- habits default to `50`
- preferred engagement window defaults to `unknown`
- marked events default to `none`

All fields are sanitized and clamped before becoming authoritative.

Current local layout:
- versioned A/B slots: `comp_mem_a` and `comp_mem_b`
- legacy migration key: `companion_mem`

## Reset Semantics

Recovery paths are now explicit:
- direct load from a valid versioned slot
- last-known-good fallback when the newest slot is corrupt
- legacy single-slot migration into the versioned layout
- reset-profile rebuild when no valid snapshot remains

`reset_profile()` restores only the persistent companion-memory record:
- preferences go back to neutral
- habits go back to baseline
- marked events are cleared

This reset does not:
- change fixed character identity
- restore or persist session memory
- alter runtime-config storage

## Trade-offs

Accepted trade-offs:
- compact aggregate memory instead of raw logs
- no export/import path yet for companion memory
- no long-term adaptive policy yet
- no overlap with short-session state

This keeps the subsystem small, product-real, and safe for future operational stages.

## Integracao com perfil adaptativo

A memoria persistente do companion agora alimenta o perfil adaptativo apenas como baseline bounded e rastreavel.

Regras oficiais desta etapa:
- preferencias e habitos persistidos modulam apenas os biases adaptativos centrais
- a influencia persistente entra no Companion State, nao direto em behavior, face ou motion
- o estado separa baseline persistente de ajuste contextual de sessao
- behavior, face e motion continuam lendo apenas o envelope efetivo central

Isso preserva identidade, evita duplicacao de parametros e mantem rastreabilidade do que veio da memoria persistente versus do contexto curto.


## Validacao entre sessoes

Cenarios oficiais validados nesta etapa:
- reboot limpo preserva preferencias e habitos recuperados
- uma sessao nova enxerga o ultimo update persistido, nao um snapshot antigo
- uma tentativa de update invalido nao apaga o ultimo perfil valido

Isso prova continuidade entre sessoes sem misturar memoria persistente com memoria curta de sessao.


## Modulacao comportamental

Mapa oficial desta etapa:
- rotina: historico social reforcado pode trocar o idle ambiente por `UserPresencePulse`
- atencao: quando o canal atual combina com o canal preferido persistido, `AttendUser` recebe um empurrao bounded
- iniciativa: historico persistente aumenta prioridade e TTL de rotina de forma pequena e previsivel
- frequencia de iniciativa: afinidade historica com usuario ou estimulo pode encurtar o cooldown de rotina dentro de um envelope bounded
- ritmo de interacao: contexto reforcado pode encurtar o cooldown curto de reengajamento sem eliminar a previsibilidade

Tudo continua filtrado pela personalidade base e pelos clamps do estado central; o historico nunca vira politica solta por servico.



## Modulacao em expressao

A memoria persistente agora tambem pode aparecer de forma sutil na expressao quando o companion esta em `IdleObserve` e nao existe contexto curto mais forte.

Regras oficiais desta etapa:
- face usa afinidade historica com usuario para segurar idle mais centrado e caloroso
- face usa afinidade historica com estimulo para deixar o idle um pouco mais curioso, sem trocar de identidade
- motion usa a mesma leitura central para uma postura idle levemente mais presente
- nenhum desses sinais sobrepoe `Sleep`, `EnergyProtect`, follow facial ativo ou contexto curto quente

Isso deixa a memoria longa visivel no produto sem criar leitura direta de storage em `face` ou `motion`.
