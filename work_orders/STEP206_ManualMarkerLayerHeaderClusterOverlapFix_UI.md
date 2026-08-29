# STEP206 — Manual marker layer header buttons overlap (ungrouped layer row's [Icon][Grid][SYM][COL] cluster)

**Layer:** UI. **Domain:** `src/ui/MarkersTab_ManualLayers_UI.cpp` (the actual fix),
`src/ui/MarkersTab_ManualLayers_UI_Test.cpp` (acceptance test extension). Pure CPU/imgui layout math —
no GPU or compute-dispatch involvement, no accuracy-class concern.

## Root problem
The offending function is `DrawRightAlignedSymmetryColorOverrideCluster`, which lives in
`src/ui/MarkersTab_ManualLayers_UI.cpp:80-108` (the ungrouped/flat Manual layer list's own header-extra
callback, `DrawLayerList`, calls it at line 166). It draws the row header's `[Icon Size][Grid Size]
[SYM][COL][swatch]` cluster, right-aligned so it sits flush against `DraggableList`'s own built-in
`[o]/[L]/[X]` affordance strip.

The function computes a `clusterWidth` budget as the bare SUM of five control-width constants, then
pushes the cursor right by `kMarkerLayerHeaderExtraCombinedWidthPixels - clusterWidth` (line 99-100)
before drawing the five controls sequentially with plain `ImGui::SameLine()` calls. **This omits FIVE
real gaps that ARE actually drawn**, each equal to the live `ImGui::GetStyle().ItemSpacing.x` (imgui's
default `SameLine()` argument), confirmed by direct read of every control function in
`src/ui/MarkersTab_ManualLayerRowBody_UI.cpp`:
1. Icon Size control → `SameLine()` → Grid Snap control (external gap, `MarkersTab_ManualLayers_UI.cpp:101-102`)
2. **Inside** `DrawMarkerLayerGridSnapHeaderControl` itself: the "GRID" toggle → `SameLine()` (line 217)
   → the Grid Size slider — not budgeted by `kMarkerLayerGridSizeControlWidthPixels`, a bare sum with
   no spacing term
3. Grid Snap control → `SameLine()` → Symmetry toggle (external gap, `MarkersTab_ManualLayers_UI.cpp:103-104`)
4. Symmetry toggle → `SameLine()` → Color Override control (external gap, `MarkersTab_ManualLayers_UI.cpp:105-106`)
5. **Inside** `DrawManualMarkerLayerColorOverrideHeaderControl` itself: the "COL" toggle → `SameLine()`
   (`MarkersTab_ManualLayerRowBody_UI.cpp:164`) → its own color swatch

Because `clusterWidth` is undercounted by `5 × ItemSpacing.x` (40px at imgui's default 8px spacing), the
cursor is pushed further right than it should be, and the cluster's real drawn width exceeds its
budgeted zone by exactly that same amount — landing its right edge that far past
`kMarkerLayerHeaderExtraCombinedWidthPixels`'s own boundary, i.e. on top of `DraggableList`'s built-in
`[o]/[L]/[X]` strip. This matches the reported symptom exactly, and is confirmed live/reproducing on a
Manual layer (e.g. an auto-created "Alloys" layer), not on the ungrouped Procedural layer header.

An existing regression test already covers a narrower version of this function
(`src/ui/MarkersTab_ManualLayers_UI_Test.cpp:222-250`,
`RunUngroupedClusterDoesNotOverlapAffordanceStripCheck`, STEP145) but only asserts the cluster's overall
right edge does not overlap the strip. Run it BEFORE making any change and record the result; if it is
currently failing, this ticket is exactly what turns it green.

**Verified NOT the same defect class — the Bundle-tree Manual leaf branch is out of scope.**
`DrawMarkerGroupLeafHeaderExtra`'s Manual branch (`src/ui/MarkersTab_BundleHeaderExtras_UI.cpp:250-287`,
e.g. a grouped Manual layer) draws its five controls sequentially via plain `ImGui::SameLine()` with no
pre-computed width-budget subtraction at all — only the trailing "X" delete button right-aligns itself,
via `DrawRightAlignedDeleteButton` (`MarkersTab_BundleHeaderExtras_UI.cpp:38-44`), which computes its
push from the LIVE `ImGui::GetContentRegionAvail().x` after wherever the cursor was actually left —
self-correcting regardless of how many gaps preceded it. No fix is needed there; do not touch that
function under this ticket.

## Fix approach
In `DrawRightAlignedSymmetryColorOverrideCluster` (`MarkersTab_ManualLayers_UI.cpp:80-108`), add the
five missing gaps to `clusterWidth`, reading the LIVE style value (never a hardcoded literal, so a
future theme change with different `ItemSpacing` stays correct automatically):
```cpp
const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
const float clusterWidth = kMarkerLayerIconSizeControlWidthPixels
                          + kMarkerLayerGridSizeControlWidthPixels
                          + kMarkerLayerSymmetryButtonWidthPixels
                          + kMarkerLayerColorOverrideButtonWidthPixels
                          + kMarkerLayerColorOverrideSwatchWidthPixels
                          + 5.0f * itemSpacing;
```
No change to the draw calls themselves — only the width accounting that determines the left-push amount
changes.

## Explicit out-of-scope
- `DrawMarkerGroupLeafHeaderExtra`'s Manual branch — verified above to not share this defect class.
- `DrawRightAlignedProceduralLayerCluster` and the ungrouped Procedural header
  (`MarkersTab_RuleLayers_UI.cpp:122-134`) — neither reproduces; the ungrouped Procedural header is a
  single-widget cluster with no gap to omit, and the Bundle Procedural cluster already accounts for
  spacing correctly.
- No change to `kMarkerLayerHeaderExtraCombinedWidthPixels` itself.
- No change to `DrawMarkerLayerIconSizeHeaderControl`'s or `DrawMarkerLayerSymmetryToggleHeaderControl`'s
  own bodies — neither has an internal `SameLine()` to account for.

## Acceptance test
Run `RunUngroupedClusterDoesNotOverlapAffordanceStripCheck` first, unmodified, and record whether it is
currently red or green. After the fix, extend that same test with a second run of the identical geometry
check under an artificially large, non-default `ImGui::GetStyle().ItemSpacing.x` (e.g. push
`ImGuiStyleVar_ItemSpacing` to `(20.0f, 4.0f)` before the frame, pop after) to prove the fix reads the
live style value rather than a hardcoded constant — the `clusterMax.x <= stripMin.x + 0.5f` invariant
must hold at BOTH the default and the exaggerated spacing. Full solo rebuild + `ctest -C Debug`:
previously-passing suite stays green (or, if the existing check was previously red, it now passes).
