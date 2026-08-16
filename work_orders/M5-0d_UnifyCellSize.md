# Work-Order M5-0d — unify cell-size on `Geometry::worldUnitsPerCell`

*Constitution §7. Milestone M5 prerequisites. Small ARCH-ruled fix. Touches Mask + its
constants — run alone (short). Executor: SanGen Coder.*

## Root problem
Mask bakes `slope` over `Proc::MaskConstants::cellSize`, while Placement's emitted
positions use `Params::Geometry::worldUnitsPerCell`. Both default to `1.0`, so they agree
today — but they are the **same physical quantity** (the world size of one heightfield
cell) held in **two places**. If a project ever sets `worldUnitsPerCell != 1`, the baked
slope silently stops matching the world scale Placement uses. One quantity, one owner.

## ARCH ruling (why)
`Geometry::worldUnitsPerCell` is the single source of truth for cell world-size (§7.1:
geometry owns map scale). Mask must read it; a private `MaskConstants::cellSize` is a
rival source and is retired. This intentionally supersedes the M5-0a/M5-0c guard against
touching the gate's cell-size wiring — that guard existed to keep those two work-orders
behavior-neutral, not to freeze the duplication permanently.

## Target files
- `src/proc/Mask_PROC.*` — read `Geometry::worldUnitsPerCell` for the slope gradient.
- `src/proc/…MaskConstants…` — remove `cellSize`.

## Layer & accuracy
`PROC`. Mask stays the single writer of `slope`.

## Solution
Wire Mask's slope gradient to use `worldUnitsPerCell` from the `Geometry` on the recipe,
in place of `MaskConstants::cellSize`. Delete the now-orphaned constant.

## Acceptance
- **Parity at default:** with `worldUnitsPerCell == 1.0`, the baked `slope` and Placement
  candidate/accept counts + checksums are **byte-identical** to pre-change (the M5-0c
  A/B harness must still pass).
- **Correct scaling:** with `worldUnitsPerCell == 2.0`, `slope` scales as expected and
  matches the world scale Placement emits against (add a case).
- `MaskConstants::cellSize` no longer exists; builds clean; 37/37 (+new) tests pass.

## Out of scope
Any other constant relocation; the slope unit itself (pinned, MASKING_SPEC 1.8).
