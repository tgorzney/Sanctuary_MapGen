# STEP130 — bSymmetryEnabled field + Symmetry toggle + Bundle-tree Color Override via new slot

**Layer:** PARAMS, IO, UI. **Domain:** `MarkerInstance_PARAMS.h`, `MapExporter_Markers_IO.cpp`,
`MapImporter_MarkerGroups_IO.cpp`, `MarkerDragGesture_UI.cpp`, `MarkerSymmetryFixCommand_UI.cpp`,
`MarkersTab_ManualLayerRowBody_UI.cpp/.h`, `MarkersTab_Bundles_UI.cpp`,
`MarkersTab_ManualLayers_UI.cpp`. **Sequence:** depends on STEP129 (landed — the
`TreeListWidget_UI` header-extra slot). Independent of STEP128/134.

Ratifies `ARCH_19_24_SymmetryEnabledField.md` (item 10) and
`work_orders/DESIGN_MarkersUICorrectionRound2_R1.md` item 7(b) (Bundle-tree Color Override wiring +
body-copy removal) together, per the design's own recommendation — both touch the same row-header
composition, avoid touching it twice.

## Part A — `bSymmetryEnabled` field (ARCH §19.24)

**Fix:**
1. `MarkerInstanceLayer` (`MarkerInstance_PARAMS.h`) gains `bool bSymmetryEnabled = true;` beside
   `symmetry`, mirroring `bColorOverrideEnabled`'s exact shape in the same struct.
2. Wire key `"SymmetryEnabled"` — exporter (`MapExporter_Markers_IO.cpp`) and importer
   (`MapImporter_MarkerGroups_IO.cpp`), same `ReadJsonBoolean`/`json[...] = ...` pattern as
   `bColorOverrideEnabled`. Additive, no version bump.
3. **Binding consumer gate, per ARCH §19.24 — not optional:** every current reader of
   `MarkerInstanceLayer::symmetry` must check `bSymmetryEnabled` FIRST and force the effective mask to
   `SymmetryAxis::None` when false, never reading `symmetry`'s raw bitmask directly:
   - `ResolveEffectiveMarkerSymmetry`'s call sites in `MarkerDragGesture_UI.cpp`.
   - `MarkerSymmetryFixCommand_UI.cpp`'s repair walk.
   Grep for every other reader of `.symmetry` on a `MarkerInstanceLayer` before considering this
   complete — the ARCH ruling is "every current reader," not just the two named above; those are the
   ones confirmed at design time, not necessarily exhaustive.

## Part B — Symmetry toggle control

New small control (mirrors `DrawManualMarkerLayerColorOverrideHeaderControl`'s shape exactly): a
plain checkbox bound to `layer.bSymmetryEnabled`, no swatch, placed in
`MarkersTab_ManualLayerRowBody_UI.cpp` beside the Color Override header control. Tooltip on hover:
`"Symmetry"` (mirrors the Color Override checkbox's own empty-label + hover-tooltip pattern,
`MarkersTab_ManualLayerRowBody_UI.cpp:90-91`).

**Placement, per ARCH §19.24:** `[Symmetry toggle][Color Override]`, in that order, sharing ONE
combined `headerExtraWidthPixels` (sum of both controls' own widths) — both drawn in sequence inside
the same header-extra callback, no further widget-library change.

- **Flat/ungrouped `DraggableList` rows** (`MarkersTab_ManualLayers_UI.cpp`): widen
  `kMarkerLayerColorOverrideHeaderWidthPixels`'s effective reservation (or add a second named
  constant, `kMarkerLayerSymmetryToggleWidthPixels`, and sum both when passing
  `headerExtraWidthPixels` to `DraggableList<T>::Render`) — this list already uses the STEP123 slot,
  just add the second control to the existing callback.
- **Bundle-tree rows** (`MarkersTab_Bundles_UI.cpp`): see Part C below — this is the FIRST real
  consumer of STEP129's new `TreeListWidget_UI` slot.

## Part C — Wire Bundle-tree leaf callback (item 7(b)) + delete body-copy

Now that STEP129 shipped the `drawLeafHeaderExtra` slot, wire it at the Bundle tree's
`TreeListWidget_UI<Params::MarkerLayerBundle, MarkerGroupLeafKey_UI>::Render` call site
(`MarkersTab_Bundles_UI.cpp`):

```cpp
drawLeafHeaderExtra = [&](const MarkerGroupLeafKey_UI& leaf) {
    if (leaf.kind != MarkerGroupLeafKind_UI::Manual) return;   // Rule leaves have no color/symmetry field
    // resolve the real Params::MarkerInstanceLayer& by leaf's index into instanceLayers, then:
    DrawManualMarkerLayerColorOverrideHeaderControl(layer, rootState.manualLayers, bAnyCommitted);
    ImGui::SameLine();
    // new symmetry-toggle control from Part B
};
headerExtraWidthPixels = <same combined width Part B computed>;
```

Confirm the exact leaf-kind check against `MarkerGroupLeafKey_UI`'s real field name/enum (read the
current struct — this ticket's own draft may not have the exact spelling; use whatever the live code
actually calls it, do not guess a name that doesn't compile).

**Once wired, delete `DrawLayerRowBody`'s body copy of Color Override**
(`MarkersTab_ManualLayerRowBody_UI.cpp`'s `if (!state.bUseGroupColor) { ... }` block that draws
`DrawCheckbox("Color Override", ...)` + `DrawColorSwatch("Color", ...)`) — its stated reason for
existing (STEP123's own comment: bundled layers reach `DrawLayerRowBody` only through the tree's
leaf-body callback, which had no header-extra mechanism) is now false for BOTH bundled and ungrouped
layers, since both paths now reach the header control. Per the human's own instruction ("Color
Override does not need to be in the list, it is already in the Marker Type Dropdown bar"), removing
the body copy is the explicit ask, not an optional cleanup.

## Verify

- **Part A:** extend the IO round-trip tests (mirroring `bColorOverrideEnabled`'s own coverage) for
  `bSymmetryEnabled`'s round-trip and legacy-default (`true`) behavior. Unit test:
  `ResolveEffectiveMarkerSymmetry` with `bSymmetryEnabled = false` returns `SymmetryAxis::None`
  regardless of `symmetry`'s own configured mask; re-enabling (`true`) restores the ORIGINAL
  `symmetry` value unchanged (not reset/cleared) — this is the specific non-destructive claim ARCH
  §19.24 makes, verify it directly, don't assume it falls out of "just gate the read."
- **Part B:** headless-frame test (mirroring `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`)
  — clicking the Symmetry toggle flips `bSymmetryEnabled`, does not touch `layer.symmetry`'s own
  fields; both controls (Symmetry + Color Override) render at their own distinct, non-overlapping X
  positions within the combined header-extra width.
- **Part C:** extend `MarkersTab_Bundles_UI_Test.cpp` — a Manual leaf inside a Bundle renders the
  header controls (Symmetry + Color Override) via the tree's `drawLeafHeaderExtra`; a Rule (Procedural)
  leaf does NOT (the `kind != Manual` guard). Confirm `DrawLayerRowBody`'s body no longer draws Color
  Override for EITHER bundled or ungrouped layers (grep for the deleted block; add/extend a headless
  test asserting the checkbox/swatch no longer appear in the row body's own draw).
- Existing suites (`MarkersTab_ManualLayers_UI_Test`, `MarkersTab_ManualLayerColorOverrideHeader_UI_Test`,
  `MarkersTab_Bundles_UI_Test`, `MarkerDragGesture_UI_Test`, `MarkerSymmetryFixCommand_UI_Test`,
  `MapImporter_IO_Test`, `MapExporter_IO_Test`) stay green.

## Out of scope

- Everything else in `BRIEF_MarkersUICorrectionRound2_R1.md` — separately ticketed. In particular,
  item 11 (selection unification) and item 13 (procedural instance listing) are NOT touched here.
