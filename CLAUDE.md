# SanGen — Project Router & Law (always loaded)

This repo is the SanGen map generator, rebuilding to **v2** against a single
authoritative architecture (`ARCH.md` — an index over per-section `ARCH_NN_*.md`
files), maintained by an AI "expert team." The full design lives in the Setup
Plan; this file is the thin, always-loaded router that points at the law and
the experts.

## Non-negotiable law
- All AI agents are **read-only against program code**. No code is created or
  changed without the human's explicit approval.
- The **SanGen ARCH Expert** is the ONLY writer of `ARCH.md`, every `ARCH_NN_*.md`
  section file, and `sangen_arch_pack/`. No other agent writes the ARCH.
- No agent commits to git. Agents write files into place; the human commits.
- Code is split into the smallest reusable, hyper-specific units (minimal
  blast radius; AI-legible). Unless a work-order says otherwise, the
  highest-performance method for the target platform is used.
- Authoritative architecture = `ARCH.md` (index) + the `ARCH_NN_*.md` section files
  it lists + `sangen_arch_pack/CONSTITUTION.md` (always-true law) + the specs named
  in `sangen_arch_pack/INDEX.md` — section files and specs are loaded on demand,
  never all at once.

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
