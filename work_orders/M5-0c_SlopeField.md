# Work-Order M5-0c — baked `slope` field (single writer: Mask)

*Constitution §7. Milestone M5 prerequisites. Sequential (touches shared `MapFields` +
Mask + Placement + composite). Run after M5-0a (both touch Placement). Executor: SanGen
Coder.*

## Root problem
Slope colorization isn't wired because there is no baked slope field — and per ARCH_03_ModuleBoundaries.md §3.2
the composite must **sample** slope, not recompute it (that was the shadow-sim defect).
The Mask stage already computes slope for its gate; make that the one slope authority.

## Target files
- `src/data/MapFields_DATA.h` — add `FloatField slope`.
- `src/proc/Mask_PROC.*` — write `slope` (single writer) as it computes the gate.
- `src/proc/Placement_PROC.*` — sample the baked `slope` for its slope gate (stop
  recomputing).
- `src/ui/PreviewComposite_UI.*` — sample `slope` for slope colorization.

## Layer & accuracy
`DATA` field + `PROC` writer/reader + `UI` reader. Mask is the **single writer** (ARCH_03_ModuleBoundaries.md
§3.4).

## Solution
Add `slope` to `MapFields`; the Mask stage writes it (the gradient it already computes,
in the pinned unit — gradient magnitude, MASKING_SPEC 1.8). Placement and the composite
**read** it; neither recomputes slope. `MapFields::Resize` sizes it with the others.

## Acceptance
`slope` matches a known heightfield's analytic gradient at a spot cell; Placement's gate
result is unchanged from its previous self-computed slope (parity); the composite
colorizes slope through the M4-2 LUT; Mask is the only writer (Placement/composite leave
it byte-identical); builds clean; end-to-end test still green.

## Out of scope
Any change to the slope *unit* or the gate math (already pinned); other fields.
