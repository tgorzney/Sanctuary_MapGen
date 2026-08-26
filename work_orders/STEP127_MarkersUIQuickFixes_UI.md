# STEP127 — Markers UI: dead field removal, rename+default, header-row spacing bug

**Layer:** UI. **Domain:** `MarkersTab_ManualLayers_UI.h/.cpp`, `MarkersTab_Globals_UI.cpp`,
`GlobalMarkerSettings_PARAMS.h`, `DraggableListWidget_RowLayout_UI.h`,
`DraggableListWidget_RowAffordances_UI.h`. **Sequence:** independent of the Round 2 design round
in flight; no dependency, no blocker.

Ratifies `work_orders/BRIEF_MarkersUICorrectionRound2_R1.md` items 2, 3, 8, 9 — all confirmed by
direct investigation before this ticket was drafted (see the brief for the full root-cause trace).
No design/ARCH round needed for any of these: 2 and 3 are field-level edits with a confirmed dead
field and a confirmed literal default; 8/9 is an arithmetic bug in existing shared widget code, fixed
by correcting the formula, not by adding new capability.

## Item 2 — delete dead "Layer Icon Scale" control

**Confirmed:** `DrawManualMarkerLayerBlockSettings` (`MarkersTab_ManualLayers_UI.cpp:52-59`) writes
only to `ManualMarkerLayersState::layerIconScale`. Grep-confirmed zero readers anywhere in the
codebase. The real per-layer scale is `Params::MarkerInstanceLayer::iconScale`, already wired
end-to-end since STEP122.

**Fix:**
- Delete the `DrawSliderScalar("Layer Icon Scale", ...)` call in `DrawManualMarkerLayerBlockSettings`.
- Delete `ManualMarkerLayersState::layerIconScale` and `layerIconScaleToggle` (and `iconScaleRange` if
  nothing else in this struct uses it — confirm by grep before deleting; `state.iconScaleRange` may
  be shared with another control in the same struct, do not remove it if so).
- Confirm no other file reads `layerIconScale`/`layerIconScaleToggle` before deleting (repeat the
  grep the brief already ran, to catch anything added since).

## Item 3 — rename "Item Scale" + new default

**Confirmed:** `MarkersTab_Globals_UI.cpp:73`, label literal `"Item Scale"`, bound to
`GlobalMarkerSettings::scaleAlloy/scalePlasma/scaleSpawn`, current default `0.17f`
(`GlobalMarkerSettings_PARAMS.h:21-23`).

**Fix:**
- Change the label string to `"Icon Scale (Global)"`.
- Change `scaleAlloy`/`scalePlasma`/`scaleSpawn`'s default initializers from `0.17f` to `0.50f`.
- **This changes visible marker size for every existing `.sanmap` that never explicitly wrote
  `MarkerScaleAlloy`/`Plasma`/`Spawn`** (i.e. every file authored before STEP124) — the import path
  has no such files today since the field always exported (STEP119+), so this only affects the
  in-memory default for a BRAND NEW recipe, not existing saved files. Confirm this understanding is
  correct by checking `ReadGlobalMarkerSettingsJson`'s own behavior on a missing key before treating
  it as fully safe — if a legacy pre-STEP119 `.sanmap` (no `GlobalMarkerSettings` object at all) is
  imported, its markers would jump from the old rendered size to whatever `0.50f` produces. Flag in
  the Verify section; do not silently assume this is inconsequential.
- Note directly connected to this ticket, not solved by it: `MarkerGlobalScaleRow`'s own local
  scratch UI fields, if any still exist duplicating this value, must stay in sync — confirm current
  code reads `GlobalMarkerSettings` directly (per STEP121, it should) and does not carry its own
  stale scratch copy of the default.

## Items 8/9 — header-row affordance-strip overlap/gap bug

**Confirmed root cause (single bug, both symptoms):** `RenderCollapsibleRow`
(`DraggableListWidget_RowLayout_UI.h:24-47`) positions the header-extra control at:
```
offsetA = rowAvailWidthPixels - kAffordanceStripWidthPixels - extraButtonWidthPixels - headerExtraWidthPixels
```
then calls `DrawRowAffordances` with `rowAvailWidthPixels - headerExtraWidthPixels` as its width
argument. Inside `DrawRowAffordances` (`DraggableListWidget_RowAffordances_UI.h:46-50`), the strip's
own start offset is:
```
offsetB = (rowAvailWidthPixels - headerExtraWidthPixels) - kAffordanceStripWidthPixels - extraButtonWidthPixels
```
`offsetA` and `offsetB` are algebraically identical — both land at the same absolute X. The
affordance strip (lock toggle, delete button) draws ON TOP of the header-extra control (currently
Color Override's checkbox+swatch) instead of to its right, AND the strip's own right edge falls
`headerExtraWidthPixels` short of the row's true right margin, leaving that gap unused at the actual
row edge.

**Fix:** `DrawRowAffordances`'s start offset must be computed relative to where the header-extra
control's OWN drawing actually finishes (i.e., the strip should start at
`rowAvailWidthPixels - kAffordanceStripWidthPixels - extraButtonWidthPixels`, using the FULL row
width, with the header-extra control occupying the space to the LEFT of that — not both formulas
independently re-deriving a position from a pre-shrunk width). Concretely: `RenderCollapsibleRow`
should draw the header-extra control first (as it does now, at `offsetA`), then call
`DrawRowAffordances` with the FULL `rowAvailWidthPixels` (not the reduced one) so the strip
right-aligns against the row's true edge, independent of `headerExtraWidthPixels`. Verify this
doesn't reintroduce a different overlap by checking `offsetA`'s own right edge
(`rowAvailWidthPixels - kAffordanceStripWidthPixels - extraButtonWidthPixels`) stays ≤ the strip's
new left edge — i.e., the header-extra control's reserved `headerExtraWidthPixels` must itself be
wide enough to contain what it draws (already true today per STEP123's own sizing, but confirm).

This is shared widget code — the fix affects every `headerExtraWidthPixels` caller present and
future, not just the Manual Marker Layers list. Currently the only caller is `DrawLayerList`
(`MarkersTab_ManualLayers_UI.cpp:89`, `kMarkerLayerColorOverrideHeaderWidthPixels = 90.0f`).

## Verify

- **Item 2:** confirm `layerIconScale`/`layerIconScaleToggle` have zero remaining references after
  deletion (build failure would catch this, but grep first); confirm the Manual Marker Layers block
  still builds and its remaining controls (Use Group Color, etc.) are unaffected.
- **Item 3:** extend `GlobalMarkerSettings_PARAMS_Test.cpp`'s default-value assertions to expect
  `0.50f`, not `0.17f`, for `scaleAlloy/Plasma/Spawn`; extend whatever IO round-trip test currently
  checks these defaults. Add an explicit check/comment on the legacy-import-default-jump question
  raised above — if there's an existing "legacy default" test fixture (mirroring
  `CheckMergedParentBundleIdentifierLegacyDefault`'s shape) for `GlobalMarkerSettings`, extend it;
  otherwise note whether one should exist and flag if out of this ticket's scope.
- **Items 8/9:** a new synthetic-frame geometry test (mirroring
  `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`'s `HeadlessImguiSession`/`RunHeadlessFrame`
  harness) asserting: (a) the header-extra control's item rect and the affordance strip's own first
  item rect do NOT overlap on the X axis; (b) the affordance strip's own rightmost item rect ends
  within a small epsilon of the row's true right edge (not `headerExtraWidthPixels` short of it).
  Run against BOTH the 2-callback (`headerExtraWidthPixels == 0`) and 3-callback paths to confirm the
  existing no-header-extra callers are provably unaffected (regression, not just new coverage).
- Full existing suite stays green, in particular every `DraggableList<T>::Render` call site's own
  test (19+ sites per STEP123's own cross-check) — this fix touches shared layout code all of them
  depend on.

## Out of scope

- Everything else in `BRIEF_MarkersUICorrectionRound2_R1.md` (items 1, 4, 5, 7, 10, 11, 12, 13, 14) —
  routed through the separate design round already in flight.
