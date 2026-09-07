# SanGen — Project Router & Law (always loaded)

This repo is the SanGen map generator, rebuilding to **v3** against a single
authoritative architecture (`ARCH.md` — an index over per-section `ARCH_NN_*.md`
files), maintained by an AI "expert team." The full design lives in the Setup
Plan; this file is the thin, always-loaded router that points at the law and
the experts.

## Non-negotiable law
- All AI agents are **read-only against program code**. No code is created or
  changed without the human's explicit approval.
- The **SanGen ARCH Expert** is the ONLY writer of `ARCH.md`, every `ARCH_NN_*.md`
  section file, and `sangen_arch_pack/`. No other agent writes the ARCH.
- Whoever creates or modifies files for a ratified work-order commits them — the
  human is not the commit bottleneck. See "Commit protocol" below for the
  required safety sequence; skipping it (especially the narrow-staging and
  build-gate steps) is not permitted.
- Code is split into the smallest reusable, hyper-specific units (minimal
  blast radius; AI-legible). Unless a work-order says otherwise, the
  highest-performance method for the target platform is used.
- Authoritative architecture = `ARCH.md` (index) + the `ARCH_NN_*.md` section files
  it lists + `sangen_arch_pack/CONSTITUTION.md` (always-true law) + the specs named
  in `sangen_arch_pack/INDEX.md` — section files and specs are loaded on demand,
  never all at once.

## Commit protocol
Many sessions work this repo concurrently. This sequence exists because "just be
careful" already failed in practice (multiple sessions have landed uncommitted
work in the same files at once) — follow it exactly, every time, not as a
suggestion.

1. **Claim before writing.** Before a work-order's implementation starts, check
   active peer sessions (`ListAgents`, and/or the CCD session tools) and message
   every session whose title or known scope could plausibly overlap the
   work-order's "Files touched" list, asking for a conflict check. Don't start
   writing code until you've heard back from everyone contacted, or confirmed via
   `git status` that none of the target files are already mid-edit elsewhere.
2. **Stage narrowly.** `git add` only the exact files in the work-order's "Files
   touched" list — never `git add -A` / `git add .`. This is the single most
   important rule: broad staging is what turns "two sessions touched different
   files" into "one session's commit swallowed the other's unrelated work."
3. **Re-check right before staging.** Run `git status`/`git diff` on exactly
   those files immediately before staging. If any show changes you don't
   recognize as your own session's, STOP — do not stage or commit. Message peers
   again to find the owner and resolve it before proceeding. Never use a
   destructive git command (`reset --hard`, force-push, discarding local
   changes) to work around an unexpected collision — coordinate, don't overwrite.
4. **Build must succeed first.** Run the full project build after implementing
   the work-order. Only commit if it's clean. A failing build stays uncommitted
   — report/flag it, don't commit it. Caveat: concurrent sessions can collide on
   the *shared build output directory* (PDB locks etc.) even with fully disjoint
   source files — a build failure that looks like a file lock is inconclusive
   (peer contention), not proof the code is broken; retry once before concluding
   the build actually failed.
5. **One commit per ratified work-order.** Message format: a concise summary
   line, a blank line, `Implements work_orders/<FILENAME>.md`, a blank line, then
   the standard attribution line.

## Experts (consult when)
The ARCH is ratified; the full expert team is in place:
- **SanGen ARCH Expert** — `.claude/agents/sangen-arch-expert.md` — architecture,
  coding law, module boundaries, naming, the optimization pillars, the CPU/GPU
  dispatch standard, ARCH conformance. Sole writer of the ARCH.
- **SanGen Format Expert** — `.claude/agents/sangen-format-expert.md` — the
  `.sanmap` format, import/export, unit/prop/marker data, sanpack ingestion.
- **SanGen IO Architecture Expert** — `.claude/agents/sangen-io-architecture-expert.md`
  — how SanGen's own IO/BRIDGE code is structured (per-domain files, migration
  versioning). Distinct from the Format Expert's format-truth domain.
- **SanGen Generator Expert** — `.claude/agents/sangen-generator-expert.md` —
  the generation pipeline (noise, blend, erosion, thermal, flow, mask, placement, bake).
- **SanGen Compute Optimization Expert** — `.claude/agents/sangen-compute-optimization-expert.md`
  — MATH/SYS performance, SIMD, CPU/GPU dispatch, determinism.
- **SanGen UI Expert** — `.claude/agents/sangen-ui-expert.md` — UI framework,
  layouts, the widget library, tabs, preview/WYSIWYG design.
- **SanGen UI Optimization Expert** — `.claude/agents/sangen-ui-optimization-expert.md`
  — UI-side performance to the metal, 100k+ entity throughput.
- **SanGen Unit/Strategist Expert** — `.claude/agents/sangen-unit-strategist-expert.md`
  — in-game unit stats, balance, strategy — not SanGen's own code.
- **SanGen Coder** — `.claude/agents/sangen-coder.md` — the only agent that
  writes program code, strictly from ratified work-orders.

## Agent-pack consistency audit
Agent charters (`.claude/agents/*.md`) can go stale after an ARCH ratification —
run this on demand, or as the last step of any session that ratifies spec changes:
1. `grep -ohE '[A-Z_]+_SPEC\b' .claude/agents/*.md | sort -u` vs. the real filenames in
   `sangen_arch_pack/specs/` (`ls sangen_arch_pack/specs/*.md`) — flags a charter
   pointing at a spec that doesn't exist. Note: specs not suffixed `_SPEC` (e.g.
   `OPTIMIZATION_PILLARS.md`) won't match this pattern — check those by name.
2. `grep -in "<retired concept>" .claude/agents/*.md` for any concept a recent
   ratification just retired (e.g. a renamed field or removed key) — flags a charter
   still describing it as current.
3. Read, don't just grep, any file the checks above flag — a mention can be historical
   ("X is retired; Y replaced it") rather than stale.

## Constitution
See `sangen_arch_pack/CONSTITUTION.md`. It is intentionally short so it can be
carried in every conversation for free; deep per-module detail lives in the
specs reached through `sangen_arch_pack/INDEX.md`.
