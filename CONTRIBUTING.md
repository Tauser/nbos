# CONTRIBUTING.md

## Purpose

This document defines the practical development workflow for NC-OS contributors, including humans and coding agents.

NC-OS must evolve as a professional embedded product codebase:
- incrementally
- with clear checkpoints
- with explicit architectural intent
- with controlled Git history

---

## Development principles

Contributors must prefer:
- small, meaningful increments
- explicit contracts
- low coupling
- reversible changes
- clear review boundaries
- safe evolution over clever shortcuts

Avoid:
- giant refactors without necessity
- hidden architectural changes
- mixing many unrelated concerns in one change
- committing half-finished work

---

## Prompt-to-commit workflow

NC-OS uses a prompt-driven execution model for Codex-assisted development.

### Standard rule
Each completed prompt should result in at least one Git commit.

A prompt is considered completed only when:
- the requested scope was addressed
- the result is coherent
- the state is reviewable
- the project is not left in an obviously broken state
- build/test validation was performed when applicable

### Practical workflow
For each prompt:
1. inspect current repository state
2. explain the proposed approach
3. list files to create or change
4. implement only the intended scope
5. validate build/tests where applicable
6. summarize result, risks and pending items
7. commit the checkpoint

---

## When one prompt should become multiple commits

A single prompt may be split into multiple commits if it contains clearly separable steps.

Typical split:
1. contracts / models
2. orchestration / integration
3. tests / docs

This is preferred when:
- the prompt touches architecture-sensitive areas
- the prompt changes both contracts and runtime behavior
- the prompt introduces tests that are easier to review separately
- the prompt risks becoming too large

### Example
A Face Engine architectural prompt may reasonably be split into:
- commit 1: `refactor(face): introduce face_render_state contract`
- commit 2: `refactor(face): route facial state through FaceService facade`
- commit 3: `test(face): add face gate contract coverage`

---

## When not to commit

Do not commit if:
- the implementation is incomplete in a misleading way
- the project is left structurally broken without explanation
- the prompt produced uncontrolled scope creep
- unrelated changes were mixed together
- validation that should have happened was skipped

In these cases:
- either fix the checkpoint first
- or explicitly stop and report the blocking issue before committing

---

## Commit message policy

NC-OS uses concise conventional-style commit messages.

### Preferred format
`type(scope): short description`

### Common types
- `chore`
- `docs`
- `build`
- `feat`
- `refactor`
- `test`
- `fix`

### Examples
- `chore(repo): bootstrap layered project skeleton`
- `docs(repo): add readme and agents guidance`
- `build(core): establish initial board profile foundation`
- `feat(display): add st7789 bring-up smoke test`
- `feat(core): add system manager and scheduler skeleton`
- `refactor(face): introduce layer ownership contracts`
- `test(core): add runtime readiness checks`

---

## Scope discipline

Each commit should ideally represent one of the following:
- one contract addition
- one integration step
- one subsystem checkpoint
- one validation step
- one documentation checkpoint

Avoid combining in one commit:
- hardware baseline changes
- architecture refactors
- behavior logic
- rendering logic
- tests
- docs

unless the prompt is explicitly tiny and the combination is naturally inseparable.

---

## Validation before commit

Before committing, validate what is appropriate for that prompt.

### Typical validation
- build passes, if applicable
- targeted tests pass, if applicable
- no obvious regression in architecture boundaries
- no accidental pin/config drift
- no silent violation of AGENTS.md rules

### Required summary before commit
Every completed prompt should end with a short summary of:
- what was completed
- what remains open
- what trade-offs were accepted
- what risks still exist

---

## Branching and checkpoints

If working in a longer session:
- keep commits small
- checkpoint often
- prefer clean review boundaries
- avoid accumulating many prompts before saving progress

A checkpoint commit is better than a large unreviewable batch.

---

## Special rule for architecture-sensitive phases

The following phases deserve stricter commit discipline:
- Action Governance
- EventBus v2
- Companion State
- Face Engine Gate
- Face Engine Premium++
- Motion / Embodiment
- Power / Robustness

For these phases, strongly prefer:
- smaller commits
- explicit rationale
- separate tests/docs when helpful

---

## Final guideline

The goal is not “one prompt, one automatic commit no matter what”.

The real rule is:

> one completed prompt should produce a technically meaningful Git checkpoint.

That checkpoint may be:
- one commit
- or a small sequence of commits
as long as the result stays clear, reviewable and architecturally disciplined.
