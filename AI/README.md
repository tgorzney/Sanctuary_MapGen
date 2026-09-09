# AI Orientation — SanGen Map Generator

Read this first if you are an AI (any AI, any tool) opening this project cold.
It is a map of where things live and why, not a duplicate of their content —
the content itself stays in its one authoritative location so it can never go
stale relative to a copy. The one exception is `memory_export/`, explained at
the bottom, which holds knowledge that genuinely does not exist as a file
anywhere else in this project.

This folder is a living index. If the project's structure changes, update
this file rather than letting it drift out of date.

## What this project is

SanGen is a standalone map generator for the RTS "Sanctuary: Shattered Sun."
It is mid-rebuild to **v3** against a single authoritative architecture
document set (the "ARCH"), produced and maintained by a team of specialized
AI agents plus a human who has final approval on all code changes.

## Read this first, always: `CLAUDE.md`

[`CLAUDE.md`](../CLAUDE.md) at the project root is the always-loaded router
and law file. It is short by design. It states:
- The non-negotiable rules (AI agents are read-only on program code; only the
  ARCH Expert writes the ARCH; who commits and how).
- The exact **commit protocol** (peer-conflict checks, narrow staging, build
  gate, one commit per ratified work order).
- The full **expert roster** — which agent owns which domain, when to consult
  each one, and their `.claude/agents/*.md` charter files.

Nothing in this AI/ folder overrides or restates that law — `CLAUDE.md` is
authoritative. This file exists to help you find the *rest* of the project
faster than by exploring blind.

## Where the architecture truth lives

- [`ARCH.md`](../ARCH.md) — the index. Start here for "is X already decided,
  and where." It lists and links every section file below.
- `ARCH_NN_*.md` (~150 files, project root) — individual ratified architecture
  decisions, one topic each. Numbered by area (e.g. `ARCH_01_*` = naming law,
  `ARCH_14_*` = data model/rendering, `ARCH_19_*` = marker layer system,
  `ARCH_22_*` = navmesh/modifier blockers). Load only the sections relevant to
  your task — never all of them at once; `ARCH.md` tells you which ones apply.
- [`sangen_arch_pack/CONSTITUTION.md`](../sangen_arch_pack/CONSTITUTION.md) —
  short, always-true law (the "constitution"), meant to be cheap enough to
  load in every conversation.
- [`sangen_arch_pack/INDEX.md`](../sangen_arch_pack/INDEX.md) — routes to the
  deep-dive spec files in `sangen_arch_pack/specs/` (format spec, IO migration,
  math/SIMD, noise/blend, placement/scatter, UI framework, determinism,
  optimization pillars, and more — see that directory listing for the full
  set). Specs are loaded on demand, not preloaded.
- [`sangen_arch_pack/_RESUME.md`](../sangen_arch_pack/_RESUME.md) — session
  resume notes for the ARCH ratification process itself.

## Where the expert agents live

`.claude/agents/*.md` — one charter file per domain expert (ARCH, Format, IO
Architecture, Generator, Compute Optimization, UI, UI Optimization, Unit/
Strategist, and the Coder). Each charter states that agent's exact scope and
what it defers to other experts. `CLAUDE.md`'s "Experts (consult when)"
section is the short version of this same list — check there first.

## Where implementation history lives

`work_orders/` (project root, ~200 files) — ratified, schema-valid work
orders the Coder has implemented or is implementing, plus design briefs
(`BRIEF_*`), bugfix tickets (`BUGFIX_*`), and ARCH amendment drafts
(`ARCH_AMENDMENT_DRAFT_*` / `ARCH_CORRECTION_DRAFT_*`). A `shipped/`
subfolder holds completed ones. This is the project's paper trail — useful
for "why does this code look like this" questions — not a queue to re-derive
from scratch each session.

## Where the actual code lives

- `src/` — the live v3 source tree, split by ARCH module boundary:
  `data/`, `io/`, `math/`, `params/`, `pipeline/`, `proc/`, `sys/`, `ui/`.
- `core/`, `gui/` — pre-v3 code, being migrated into `src/` per the rebuild
  order in `ARCH_06_RebuildOrder.md`. Treat as legacy/reference unless a
  work order says otherwise.
- `shaders/`, `resources/` — GPU shaders and non-code assets.
- `build/` — build output. Shared across concurrent sessions; see
  `CLAUDE.md`'s commit protocol note on build-directory contention before
  assuming a build failure means broken code.

**All AI agents are read-only against this code.** No code is created or
changed without the human's explicit approval, and only the Coder agent
writes it, strictly from ratified work orders. See `CLAUDE.md` for the exact
rule.

## Legacy / reference material (not the live project)

These root-level directories are historical or reference-only, not part of
the live v3 build — worth knowing about so you don't mistake them for current
source: `Sanctuary-Map-Generation-develop/`, `Sanctuary_MapGen-master (Broken)/`,
`Sanctuary_MapGen-master (Old)/`, `TGUE_Reference/`, `Auto-NavMesh Design/`,
`Map Format Debate/`, `forum_posts/`, `map_scripts_backup/`.

## `memory_export/` — knowledge that isn't a file anywhere else in this repo

The Claude Code sessions that work this repo build up a persistent,
file-based memory on the human's machine, **outside this project directory**
(under that machine's Claude Code config, keyed to this project path). It
holds things no repo file records: the user's working preferences, feedback
on how to approach tasks in this codebase, and project context/decisions that
never got written into an ARCH doc or work order.

`memory_export/` in this folder is a **point-in-time snapshot** of that
store, copied in so it survives even if you (or a different AI, or a future
session on a different machine) can't reach that external location. It can
go stale — if you have access to the live memory store, prefer it over this
copy. Re-export periodically (or after major sessions) to keep it current.
See `memory_export/MEMORY.md` for its own index.
