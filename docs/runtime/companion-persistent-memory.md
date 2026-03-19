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

## Defaults and Integrity

The baseline safe profile is intentionally neutral:
- preference percents default to `50`
- habits default to `50`
- preferred engagement window defaults to `unknown`
- marked events default to `none`

All fields are sanitized and clamped before becoming authoritative.

## Reset Semantics

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
