[← ARCH index](ARCH.md) · SanGen ARCH §13. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 13. Radial N-fold symmetry — `SymmetryAxis::Radial` + `radialSymmetryRepeatCount` (ARCH ruling, amends `Symmetry_PARAMS.h`, `SANMAP_FORMAT_SPEC` Correction 4)

- **New bit:** `constexpr int Radial = 1 << 4;` in the `SymmetryAxis` namespace
  (`src/params/Symmetry_PARAMS.h`). Confirmed by direct code read
  (`src/proc/Placement_Symmetry_PROC.h`'s `BuildSymmetryOrbit`) that every set bit in the mask is
  already composed independently in sequence (`MirrorAcrossX` → `MirrorAcrossZ` →
  `RotateHalfTurn` → `QuarterTurns`, each via its own `AppendTransformedSet`/`AppendQuarterTurns`
  call) — arbitrary combination of `Radial` with the existing bits is therefore already
  structurally supported by the orbit builder's shape; **no PROC combination-logic change is
  needed for this ratification.** `Radial`'s own orbit-generation function (the N-way analog of
  the existing `AppendQuarterTurns` helper, generalized from its hardcoded 3 turns to a
  designer-chosen turn count) is new PROC work for a future coder work-order — not designed here.
- **Companion count field**, a flat sibling wherever `symmetryMask` already lives (NOT a wrapper
  struct — matches the existing `bSymmetryUseGlobal`/`symmetryMask` flat-sibling convention):
  ```cpp
  int radialSymmetryRepeatCount = 3;
  ```
  Each independently-overridable mask needs its own `N`: `MapRecipe::globalSymmetryMask`,
  `MarkerRule::symmetryMask`, `PropRule::symmetryMask`, `UnitRule::symmetryMask`, and — once
  Defect 1 below is fixed — `DecalRule::symmetryMask`, plus the future `HeightmapStack`
  `GeoLayer`/`Layer` override (`SANMAP_FORMAT_SPEC` Correction 3). A local override with
  `bSymmetryUseGlobal = false` but no local count would otherwise silently inherit the global `N`,
  defeating the point of a local override.
- **JSON key `RadialSymmetryRepeatCount`, PascalCase** — matches the confirmed-live
  `SymmetryMask` key convention in `MapExporter_Rules_IO.cpp`. Lands in `SANMAP_FORMAT_SPEC`
  Correction 4's `Symmetry` global-section field list beside `GlobalSymmetryMask`, and as a
  per-rule sibling of `SymmetryMask` on each `MarkersStack`/`PropsStack`/`DecalsStack`/
  `UnitsStack` rule entry.
- **Default axis change:** `MapRecipe::globalSymmetryMask`'s default becomes
  `SymmetryAxis::RotateHalfTurn` (was `SymmetryAxis::None`, `MapRecipe_PARAMS.h:31`) — the
  existing "Point" bit; no new bit needed for this default.
- **Default blend — forward-attached requirement on a standing reservation, not built now.**
  `Params::SymAlgorithm` does not exist in `src/` yet (confirmed zero matches) — it remains
  `SANMAP_FORMAT_SPEC` Correction 4's own reserved, deferred field. Whichever future work-order
  defines `Params::SymAlgorithm{Fold, Blur, CrossFade, Superposition, Cylinder3D, Torus3D, ...}`
  **must default it to `Superposition`.** Recorded here and in `SANMAP_FORMAT_SPEC` Correction 4
  so the requirement is not lost between now and that work-order.

**Two defects recorded this session, not fixed here** (out-of-scope code gaps for a future coder
work-order; full detail in `SANMAP_FORMAT_SPEC` Correction 4 and `PLACEMENT_SCATTER_SPEC`'s
"Known issues" addendum):
1. **`DecalRule` has no `bSymmetryUseGlobal`/`symmetryMask` pair at all**
   (`src/params/ScatterRule_PARAMS.h`) — contradicted `SANMAP_FORMAT_SPEC` Correction 4's prior
   claim that the pattern was "already live and tested on `MarkerRule`/`PropRule`/`DecalRule`";
   that claim was factually wrong for `DecalRule` and is corrected in this same session.
   `AppendDecalRules` (`src/proc/Placement_Rules_PROC.cpp`) also never calls `ResolveSymmetryMask`
   for decals, so decals currently generate with **no symmetry at all**, not even the global
   default — a real functional gap, not merely a missing field.
2. **Symmetry-clone buffer overflow risk.** `Params::symmetryOrbitMaximum = 16`
   (`Symmetry_PARAMS.h`) backs a fixed-size stack array (`SymmetryOrbitPoint orbit[16]`,
   `src/proc/Placement_Accept_PROC.cpp:33`) sized for the old maximum combination (mirror X ×
   mirror Z × quarter turns). A designer-chosen `radialSymmetryRepeatCount` combined with mirrors
   can now exceed 16 (e.g. 8-fold × MirrorX × MirrorZ → up to 32), and the buffer **silently drops
   excess clones** rather than erroring — a real correctness gap this ratification creates by
   making a larger orbit reachable from PARAMS/UI. Raising the cap and/or adding a loud validated
   clamp on the designer-facing `N` (Constitution §6) is PROC/buffer-sizing work for a future
   Compute Optimization Expert or Generator Expert work-order — not sized here.

**UI finding, not ARCH's or this ratification's to fix — flagged for the UI Expert.**
`src/ui/SymmetryTab_UI.h`'s existing `SymmetryAxisOption::Radial` option (part of the plan's five
exclusive choices — Point/X/Z/XY/Radial) currently maps to `Params::SymmetryAxis::QuarterTurns`
(confirmed by code read, `SymmetryAxisMaskOfOption`) — a stand-in used because no true N-fold bit
existed yet. **This mapping is now stale**: it must be remapped to the real
`SymmetryAxis::Radial` bit this ratification adds, or the tab's "Radial" checkbox will keep
producing 4-fold `QuarterTurns` instead of the designer's chosen N-fold repeat. This is separate
from, and additional to, the already-known finding that `SymmetryTab_UI.h`'s exclusive-checkbox
row (unlike `PlacementRuleSections_UI.h`'s per-rule OR-able tick boxes) cannot express combined
axes at all — both are UI-layer reconciliation work for the UI Expert, not decided here.
