# STEP240 — Selected-icon-size dial controls + render-consumer wiring

**Layer:** UI. **Domain:** `MarkersTab_Globals_UI.h/.cpp`, and the render-consumer site TBD by
direct read (likely wherever `ARCH_19_18`/`ARCH_19_19`'s tint-priority logic resolves `bSelected`,
downstream of `MapCanvas_IconLayer_CullManual_UI.cpp:202`'s candidate-scale composition).
**Sequence:** depends on STEP236 (done) and STEP238 (done). Independent of STEP239 (disjoint
files).

Ratifies `ARCH_19_32_MarkerSelectedScaleFields.md`. See `DESIGN_MarkerLink_R1.md` §4.3–§4.4.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. `MarkersTab_Globals_UI.cpp`'s `DrawTypeSectionMarkerSettingsRow`: add a second `DrawDialCompact`
   control (STEP236) bound to `settings.scaleSelectedAlloy/Plasma/Spawn` (via
   `ResolveGlobalMarkerScaleRowFields`, extended to also resolve the selected-scale pointer),
   alongside the existing base-scale control. Both use a `[0.25, 2.0]` `DialRange` (new, narrower
   than the existing shared `{0.1, 10.0}` range — scoped to just this row, does not change the
   per-Manual-Layer `iconScale` control's own wider range).
2. Find the actual site where `bSelected` first becomes known for tint-priority purposes
   (downstream of `MapCanvas_IconLayer_CullManual_UI.cpp:202`'s scale composition, which runs
   BEFORE selection is resolved — do not fold the selected-scale multiply in at line 202, confirm
   the correct later site by direct read). Fold `ResolveMarkerGroupSelectedTypeScale`'s result into
   the rendered scale composition only for candidates where `bSelected == true`, multiplicatively
   alongside the existing `groupTypeScale * layerIconScale` composition.

## Verify

- New test: `DrawTypeSectionMarkerSettingsRow` renders both dial controls, each honoring the
  `[0.25, 2.0]` clamp and writing to the correct field.
- New test at the render-consumer site: a selected instance's candidate scale includes the
  type's `scaleSelected*` factor; a non-selected instance's does not.
- Existing `MarkersTab_Globals_UI_Test`, `MarkersTab_GlobalScaleRowFields_UI_Test`, and whichever
  suite covers `MapCanvas_IconLayer_CullManual_UI.cpp`'s candidate-scale composition stay green.

## Out of scope

- The Links UI tier (STEP239).
- Whether item 4's dual-input reading is per-Type-section (this ticket's assumption, per the
  ratified design) vs. tab-wide — already resolved by `ARCH_19_32`, not re-litigated here.
