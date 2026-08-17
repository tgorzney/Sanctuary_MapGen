# SanGen — Project Router & Law (always loaded)

This repo is the SanGen map generator, rebuilding to **v2** against a single
authoritative architecture (`ARCH.md`), maintained by an AI "expert team." The
full design lives in the Setup Plan; this file is the thin, always-loaded
router that points at the law and the experts.

## Non-negotiable law
- All AI agents are **read-only against program code**. No code is created or
  changed without the human's explicit approval.
- The **SanGen ARCH Expert** is the ONLY writer of `ARCH.md` and
  `sangen_arch_pack/`. No other agent writes the ARCH.
- No agent commits to git. Agents write files into place; the human commits.
- Code is split into the smallest reusable, hyper-specific units (minimal
  blast radius; AI-legible). Unless a work-order says otherwise, the
  highest-performance method for the target platform is used.
- Authoritative architecture = `ARCH.md` + `sangen_arch_pack/CONSTITUTION.md`
  (always-true law) + the specs named in `sangen_arch_pack/INDEX.md` (loaded on
  demand, never all at once).

## Experts (consult when)
- **SanGen ARCH Expert** — `.claude/agents/sangen-arch-expert.md` — anything
  about SanGen architecture, coding law, module boundaries, naming, the
  optimization pillars, the CPU/GPU dispatch standard, or ARCH conformance.
  Sole writer of the ARCH.

*More experts — Map File Format, Unit/Prop Data, Optimization, UI Framework,
the Map Generator (SanGen) sub-experts, and the coder tier — are added after
the ARCH is ratified, per the Setup Plan build order.*

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
