# Commit Policy — NC-OS

## Objective

This document defines the official Git checkpoint policy for NC-OS.

The purpose is to ensure that:
- development remains incremental
- architecture stays reviewable
- prompt-driven work does not collapse into giant unstructured batches
- the repository history reflects real technical checkpoints

---

## 1. Core policy

### Official rule
Each completed prompt should generate at least one meaningful Git checkpoint.

A meaningful checkpoint is one that is:
- coherent
- reviewable
- technically intentional
- limited in scope
- safe enough to preserve

This does **not** mean “commit after every file edit”.
It means “commit after each completed logical unit of work”.

---

## 2. Prompt completion criteria

A prompt is considered complete only when all of the following are true:

- the requested scope was addressed
- the change is understandable in isolation
- the repository is not left in an uncontrolled broken state
- validation appropriate to the change has been been performed
- risks and pending issues are explicitly stated

If these conditions are not met, the prompt is not yet ready to commit.

---

## 3. Default mapping

### Default mapping
- **1 completed prompt = 1 commit**

This is the standard workflow when the prompt is small and well-bounded.

### Examples
- bootstrap repository skeleton
- add README and AGENTS
- freeze pin mapping
- add display smoke test
- add scheduler skeleton

---

## 4. Allowed exception: one prompt, multiple commits

A single prompt may become multiple commits when the work naturally separates into distinct checkpoints.

### Preferred multi-commit split
1. contracts / models
2. orchestration / integration
3. tests / docs

### When to use this exception
Use multiple commits when:
- the prompt modifies both architecture and runtime behavior
- the prompt adds both contracts and integration code
- tests deserve independent review
- the scope becomes too large for one clean commit
- the phase is architecture-sensitive

### Typical examples
#### Example A — Face Engine Gate
- `refactor(face): introduce face_render_state baseline`
- `refactor(face): define layer ownership and FaceService boundaries`
- `test(face): add architectural gate coverage`

#### Example B — EventBus v2
- `refactor(core): add event taxonomy and typed contracts`
- `feat(core): implement eventbus v2 routing`
- `test(core): add conflict and debounce coverage`

---

## 5. What must never happen

### Never do these
- do not group many prompts into one giant batch commit
- do not commit unrelated changes together
- do not commit broken intermediate states without explicit reason
- do not mix hardware pin changes with behavioral refactors in one checkpoint
- do not hide architectural changes inside “misc cleanup”
- do not use vague commit messages like:
  - `update files`
  - `fix stuff`
  - `changes`
  - `wip`

---

## 6. Commit message convention

NC-OS uses a conventional-style format:

`type(scope): short description`

### Approved types
- `chore`
- `docs`
- `build`
- `feat`
- `refactor`
- `fix`
- `test`

### Recommended scopes
- `repo`
- `core`
- `board`
- `display`
- `audio`
- `sensors`
- `motion`
- `face`
- `behavior`
- `vision`
- `voice`
- `power`
- `cloud`

### Examples
- `chore(repo): bootstrap layered project skeleton`
- `docs(repo): add readme and agent guidance`
- `build(core): establish initial board profile foundation`
- `config(board): freeze gpio map and hardware baseline`
- `feat(display): add st7789 smoke test path`
- `feat(core): add system manager and scheduler skeleton`
- `refactor(face): route facial state through FaceService facade`
- `test(face): add face composition gate coverage`

---

## 7. Validation policy before commit

Before any commit, validate what is appropriate for the scope.

### Validation examples
#### For repo/build/config changes
- project structure is still coherent
- build metadata is valid
- config remains centralized

#### For hardware baseline changes
- pin mapping is documented
- no accidental drift from frozen hardware assumptions

#### For runtime/core changes
- build passes where applicable
- lifecycle remains coherent
- no obvious architecture regression

#### For face/motion/behavior changes
- ownership remains explicit
- subsystem boundaries remain clear
- tests exist where contract-sensitive behavior changed

---

## 8. Review notes required at checkpoint

Before closing a prompt and committing, record a short review note with:

- what was completed
- what was intentionally left out
- what risks remain
- what the next expected step is

This may be in:
- the Codex response
- a development log
- PR description
- or structured notes in your workflow tool

The checkpoint is stronger when this context exists.

---

## 9. Special policy for architecture-sensitive work

The following areas require stricter commit discipline:

- Action Governance
- EventBus v2
- Companion State
- Face Engine Gate
- Face Engine Premium++
- Motion / Embodiment
- Power / Resilience

### For these areas, prefer:
- smaller commits
- explicit contract-first commits
- tests separated when useful
- docs/checkpoint notes when architecture meaning changes

---

## 10. Relationship with milestones and roadmap

Commits are not the roadmap.
Commits are the **implementation checkpoints** of roadmap items.

### Relationship
- roadmap phase = strategic stage
- epic = major body of work
- sub-epic = execution grouping
- prompt = local execution unit
- commit = technical checkpoint

This means:
- one phase contains many prompts
- one prompt usually creates one commit
- some prompts create multiple commits when needed

---

## 11. Recommended operational workflow

For each prompt:

1. read current repository state
2. restate the goal
3. propose the smallest safe implementation
4. implement only that scope
5. validate
6. summarize risks/pending items
7. commit the checkpoint

This is the standard NC-OS development loop.

---

## 12. Final rule

The official NC-OS rule is:

> Commit at the end of each completed prompt, but only when that prompt has produced a technically meaningful, reviewable checkpoint.

This preserves:
- development speed
- architectural clarity
- repository quality
- long-horizon maintainability
