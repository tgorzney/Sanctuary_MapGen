# STEP106 — Marker layer lock (`bLocked`) + per-layer grid snap on `MarkerInstanceLayer`

**Layer:** PARAMS (fields), IO (wire), UI (gate wiring, controls). **Domain:** `Params::
MarkerInstanceLayer`, `MarkersTab_ManualLayers_UI.h/.cpp`, `MarkerDragGesture_UI.h/.cpp`,
`MarkersTab_ManualInstance_UI.cpp`. **Sequence:** no dependency on other undone work-orders.

**Two features, one ticket by ARCH Expert ruling this session**: Marker layer lock (§1) and
per-layer grid snap (§2) are bundled deliberately, not out of scope creep — they touch the
identical set of call sites (every place a `MarkerTransform`'s position gets written) and the
ARCH Expert flagged that shipping them as two separate tickets risks two sessions colliding
edits on the same functions (`BeginMarkerDragGesture`, `RepositionSymmetryGroupMember`,
`DrawSelectedMarkerInstance`'s Position sliders). Land both here, together.

**Sequencing note — do not implement concurrently with STEP107.** STEP107 is a separate ticket
that also edits `DrawLayerRowBody` (STEP110's row-based replacement for the pre-restructure
`DrawSelectedLayer` — commit `04750e4` moved the Manual Marker Layers block's per-item settings
into each list row's own inline expanded header) in `MarkersTab_ManualLayers_UI.cpp`. STEP106 must
land first; STEP107 rebases onto this ticket's output.

**Note on this revision — corrected post-`04750e4`.** This ticket was drafted before commit
`04750e4` (a peer session) moved `MarkersTab_ManualLayers_UI.cpp`'s and
`MarkersTab_ManualInstance_UI.cpp`'s per-item settings from a bottom-of-tab detail panel
(`DrawSelectedLayer`, drawn once for whatever `state.selectedLayerIndex` pointed at) into each
row's own inline expanded body (`DrawLayerRowBody`/`DrawSelectedMarkerInstance`, drawn once per
EXPANDED row, independent of which row is "selected"). Every file/line reference below has been
re-verified against the current tree and corrected where the refactor shifted it. The feature spec
itself (field names, wire keys, gate function shapes, per-layer snap semantics, and each control's
placement relative to its neighbors) is unchanged — only insertion points and line numbers moved.
`SelectedManualMarkerLayer`/`state.selectedLayerIndex`/`state.selectedInstanceIndex` all still
exist unchanged, confirmed against `MarkersTab_ManualLayers_UI.h`/`MarkersTab_Manual_UI.h`.

## Problem
`Params::MarkerInstanceLayer` (`src/params/MarkerInstance_PARAMS.h:23-37`) has no way to protect a
layer's markers from accidental drag/slider edits, and no way to snap a layer's markers to a grid.
The shared `DraggableList` widget (`src/ui/DraggableListWidget_UI.h:18-19,32,106-118`) already emits
a `DraggableListSignalKind::ToggleLock` signal from its per-row lock button (`[L]`/`[U]`, line 113-114)
and already reads a `DraggableListRow::bLocked` field (line 32) to choose which icon to draw — but
`MarkersTab_ManualLayers_UI.cpp`'s `DrawLayerList` (lines 60-74; its row-describe lambda, lines
64-68, is the part that matters here) never sets `row.bLocked`, and `ApplyLayerListSignal`
(lines 81-100) has no branch for `DraggableListSignalKind::ToggleLock` at all,
so every row always shows `[U]` and the button is a dead click. `MarkerInstanceLayer` has no
`bLocked` field to wire it to. Confirmed by reading both files in full: no lock enforcement exists
anywhere in the marker drag/reposition/add/remove paths.

Grid snap does not exist anywhere in the marker domain today (confirmed: no `GridSnap`/`gridSnap`
match under `src/`).

## Fix

### 1. New fields — `MarkerInstance_PARAMS.h`
Add to `MarkerInstanceLayer` (`src/params/MarkerInstance_PARAMS.h:23-37`), next to `iconScale`:
```cpp
struct MarkerInstanceLayer {
    std::string name;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float iconScale = 1.0f;
    int   layerId = -1;
    Params::SymmetrySetting symmetry;
    bool  bLocked = false;                    // NEW — STEP106 §1. Blocks drag/reposition/add/
                                               // remove for every marker on this layer.
    bool  bGridSnapEnabled = false;            // NEW — STEP106 §2. Per-layer, not global (see §2).
    float gridSnapSizeWorldUnits = 1.0f;       // NEW — STEP106 §2. World-unit cell size; only
                                               // meaningful while bGridSnapEnabled is true.
};
```
All three additive-only. No `SanGenVersion` bump (Constitution's additive-field rule — same posture
every prior `MarkerInstanceLayer` field addition used, e.g. STEP68's `symmetry`).

### 2. Wire representation — `MapExporter_Markers_IO.cpp` / `MapImporter_Markers_IO.cpp`
Wire keys, PascalCase to match this array's existing sibling keys (`"Name"`, `"Color"`,
`"IconScale"`, `"Id"`): `"Locked"`, `"GridSnapEnabled"`, `"GridSnapSizeWorldUnits"`.

`BuildMarkerGroupsJson` (`src/io/MapExporter_Markers_IO.cpp:63-78`) — add after the existing
`layerJson["RadialSymmetryRepeatCount"] = ...` line (line 74), before `markerGroups.push_back(layerJson);`:
```cpp
layerJson["Locked"] = layer.bLocked;
layerJson["GridSnapEnabled"] = layer.bGridSnapEnabled;
layerJson["GridSnapSizeWorldUnits"] = layer.gridSnapSizeWorldUnits;
```

`ReadMarkerGroupsJson` (`src/io/MapImporter_Markers_IO.cpp`, the function whose body reads
`layerJson["Name"]`/`"Color"`/`"IconScale"`/`"Id"`/`"SymmetryUseGlobal"`/`"SymmetryMask"`/
`"RadialSymmetryRepeatCount"` — lines 120-134 per the grep above) — add after the existing
`ReadJsonInteger(layerJson, "RadialSymmetryRepeatCount", layer.symmetry.radialSymmetryRepeatCount);`
line, inside the same `if (layerJson.is_object())` block:
```cpp
ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
ReadJsonBoolean(layerJson, "GridSnapEnabled", layer.bGridSnapEnabled);
ReadJsonFloat(layerJson, "GridSnapSizeWorldUnits", layer.gridSnapSizeWorldUnits);
```
Absent keys (legacy `.sanmap` files saved before this ticket) keep the struct's own defaults
(`false`/`false`/`1.0f`) — no clamp, no range validation needed (Constitution §6's
validate-then-default rule is satisfied by the field defaults themselves; a non-positive
`gridSnapSizeWorldUnits` is handled defensively at the quantization function in §5, not here).

### 3. Enforcement gate — `IsMarkerInstanceLayerLocked`, `MarkersTab_ManualLayers_UI.h`
One shared, pure, out-of-range-safe function, living beside `SelectedManualMarkerLayer`
(`src/ui/MarkersTab_ManualLayers_UI.h:57-61`), mirroring `ResolveEffectiveMarkerSymmetry`'s shape
(`MarkerDragGesture_UI.h:66-77` — read the layer array and an index, resolve safely, no state):
```cpp
// True when `layerIndex` names a layer with bLocked set. Out-of-range (Constitution §6) resolves
// to false — an invalid layerIndex must never itself become a reason to refuse an edit; that is a
// distinct failure mode (see the existing layerIndex clamp-on-import, STEP60 §4) this gate does not
// participate in.
inline bool IsMarkerInstanceLayerLocked(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                        int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return false;
    return markerLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}
```

### 4. Call sites — lock enforcement
- **Drag-begin — `BeginMarkerDragGesture`** (`src/ui/MarkerDragGesture_UI.cpp:33-72`, declared
  `MarkerDragGesture_UI.h:95-99`): after fetching `dragged` (line 42,
  `const Params::MarkerTransform& dragged = group.transforms[...]`), before any state is set, add:
  ```cpp
  if (IsMarkerInstanceLayerLocked(markerLayers, dragged.layerIndex)) return false;
  ```
  `#include "MarkersTab_ManualLayers_UI.h"` in `MarkerDragGesture_UI.h` to reach the gate (this
  header already includes `MarkersTab_Manual_UI.h`, its sibling — adding the layers-tab header
  alongside it is the same posture). Returning `false` leaves `state` at its just-reset default
  (`state = MarkerDragGestureState{};`, line 38) — identical refusal shape to the existing
  out-of-range `groupIndex`/`transformIndex` early-returns immediately below it (lines 39, 41).

- **Reposition (grouped) — `RepositionSymmetryGroupMember`** (`MarkerDragGesture_UI.cpp:74-117`,
  declared `MarkerDragGesture_UI.h:129-133`): after fetching `dragged` (line 81,
  `Params::MarkerTransform* const dragged = SelectedMarkerInstance(...)`), before the
  `symmetryGroupIdentifier == 0` branch, add:
  ```cpp
  if (IsMarkerInstanceLayerLocked(markerLayers, dragged->layerIndex)) return false;
  ```
  Same refusal shape as the existing `dragged == nullptr` early-return two lines above (line 82).

- **Reposition (ungrouped) — the Position sliders in `DrawSelectedMarkerInstance`**
  (`src/ui/MarkersTab_ManualInstance_UI.cpp:84-117` — corrected from the pre-`04750e4` line range;
  the function itself is untouched by that refactor, only its position in the file moved): no existing symmetry-resolution call to
  piggyback on here (this function calls neither `BeginMarkerDragGesture` nor
  `RepositionSymmetryGroupMember` — it is the roster-editor's direct slider path). Add an explicit
  check and wrap the three Position sliders:
  ```cpp
  const bool bLayerLocked = IsMarkerInstanceLayerLocked(markerLayers, transform.layerIndex);
  ImGui::BeginDisabled(bLayerLocked);
  bCommitted = DrawSliderScalar("Position X", transform.transform.positionX, state.positionHorizontalRange,
                                state.positionXToggle, WidgetStyle(), "%.1f").bCommitted || bCommitted;
  bCommitted = DrawSliderScalar("Position Y (Elevation)", transform.transform.positionY,
                                state.positionElevationRange, state.positionYToggle, WidgetStyle(),
                                "%.1f").bCommitted || bCommitted;
  bCommitted = DrawSliderScalar("Position Z", transform.transform.positionZ, state.positionHorizontalRange,
                                state.positionZToggle, WidgetStyle(), "%.1f").bCommitted || bCommitted;
  ImGui::EndDisabled();
  ```
  This replaces the existing three-slider block at the end of the function (lines 109-115) verbatim
  except for the added `bLayerLocked`/`BeginDisabled`/`EndDisabled` wrapping — no other line in that
  block changes. `#include "MarkersTab_ManualLayers_UI.h"` is already present in this file (line 10)
  for `ManualMarkerLayerRowLabel`, so `IsMarkerInstanceLayerLocked` is already reachable — no new
  include needed here.

- **Instance remove — `DrawMarkerInstanceListButtons`**
  (`src/ui/MarkersTab_ManualInstance_UI.cpp:36-54` — corrected from the pre-`04750e4` line range;
  this function's own body is untouched by that refactor): gate on the SELECTED instance's own
  `transform.layerIndex` (not the currently-selected marker layer — this button acts on whichever
  instance is selected in the roster, which may sit on a different layer than the Manual Marker
  Layers tab's own selection). The existing "Remove Selected" block (lines 46-52):
  ```cpp
  ImGui::SameLine();
  if (ImGui::Button("Remove Selected")
      && SelectedMarkerInstance(transforms, state.selectedInstanceIndex) != nullptr) {
      transforms.erase(transforms.begin() + state.selectedInstanceIndex);
      state.selectedInstanceIndex = ResolvedMarkerInstanceSelection(state.selectedInstanceIndex,
                                                                     static_cast<int>(transforms.size()));
      bInstancesMoved = true;
  }
  ```
  becomes (new lock check added before the button; `markerLayers` becomes a required parameter —
  see the signature change below):
  ```cpp
  const Params::MarkerTransform* const selectedForRemove =
      SelectedMarkerInstance(transforms, state.selectedInstanceIndex);
  ImGui::SameLine();
  ImGui::BeginDisabled(selectedForRemove != nullptr
                       && IsMarkerInstanceLayerLocked(markerLayers, selectedForRemove->layerIndex));
  if (ImGui::Button("Remove Selected") && selectedForRemove != nullptr) {
      transforms.erase(transforms.begin() + state.selectedInstanceIndex);
      state.selectedInstanceIndex = ResolvedMarkerInstanceSelection(state.selectedInstanceIndex,
                                                                     static_cast<int>(transforms.size()));
      bInstancesMoved = true;
  }
  ImGui::EndDisabled();
  ```
  Note `SelectedMarkerInstance` takes a non-const `std::vector<Params::MarkerTransform>&` today
  (`MarkersTab_Manual_UI.h:84-88`) and returns a non-const pointer — assigning its result to a
  `const Params::MarkerTransform* const` here is a legal qualification-add, no signature change to
  that helper needed.

- **Instance add — `DrawMarkerInstanceListButtons`, same function**: the target layer is whichever
  layer is currently SELECTED in the Manual Marker Layers list
  (`ManualMarkerLayersState::selectedLayerIndex`, `MarkersTab_ManualLayers_UI.h:52`) — **not**
  hardcoded to layer 0. Today `DrawMarkerInstanceListButtons` has no access to that state (it lives
  in a sibling struct, `MarkersTabState::manualLayers` vs. `MarkersTabState::manual`,
  `src/ui/MarkersTab_UI.h:94,99`) and the freshly-created `MarkerTransform` relies on its
  default-constructed `layerIndex = 0` (`MarkerInstance_PARAMS.h:43`). Fix both gaps together —
  thread `markerLayers` and the selected marker-layer index down the existing call chain:

  **Signature changes required** (every one of these functions already threads `markerLayers`
  through from `DrawMarkersTab` except `DrawMarkerInstanceListButtons`, which currently takes none):
  - `DrawMarkerInstanceListButtons(std::vector<Params::MarkerTransform>& transforms,
    ManualMarkersState& state)` (`MarkersTab_ManualInstance_UI.cpp:36` — corrected from the
    pre-`04750e4` line number; signature itself unchanged by that refactor) becomes
    `DrawMarkerInstanceListButtons(std::vector<Params::MarkerTransform>& transforms,
    ManualMarkersState& state, const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    int selectedMarkerLayerIndex)`. Body's "Add Instance" block (lines 38-44) becomes:
    ```cpp
    ImGui::BeginDisabled(IsMarkerInstanceLayerLocked(markerLayers, selectedMarkerLayerIndex));
    if (ImGui::Button("Add Instance")) {
        Params::MarkerTransform transform;
        transform.name = NextMarkerInstanceName(static_cast<int>(transforms.size()));
        transform.layerIndex = (selectedMarkerLayerIndex >= 0
                                && selectedMarkerLayerIndex < static_cast<int>(markerLayers.size()))
                               ? selectedMarkerLayerIndex : 0;
        transforms.push_back(transform);
        state.selectedInstanceIndex = static_cast<int>(transforms.size()) - 1;
        bInstancesMoved = true;
    }
    ImGui::EndDisabled();
    ```
    (the ternary's out-of-range fallback to `0` mirrors `MarkerTransform::layerIndex`'s own default —
    an unselected/stale Manual Marker Layers pick must never write an invalid `layerIndex`, same
    Constitution §6 posture as the existing Layer-picker combo's mirror-and-gate pattern,
    `DrawMarkerInstanceLayerPicker`, `MarkersTab_ManualInstance_UI.cpp:76-92`).
  - Its one call site, `DrawMarkerInstanceSection` (`MarkersTab_ManualInstance_UI.cpp:146-161` —
    corrected: this function moved down the file when STEP110's row-based `DrawMarkerInstanceList`
    was added above it, but its own signature and body are otherwise untouched):
    add the same two parameters to `DrawMarkerInstanceSection`'s own signature (declared
    `MarkersTab_Manual_UI.h:149-152` — unchanged, defined `MarkersTab_ManualInstance_UI.cpp:146-148`) —
    `void DrawMarkerInstanceSection(Params::MarkerInstanceGroup& group,
    const std::vector<Params::Army>& armies,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkersState& state,
    int selectedMarkerLayerIndex)` — and pass `markerLayers, selectedMarkerLayerIndex` through to
    the `DrawMarkerInstanceListButtons(group.transforms, state, markerLayers,
    selectedMarkerLayerIndex)` call (line 155). `markerLayers` is already a parameter of this
    function (passed at line 152 into `DrawMarkerInstanceList`, which threads it down to
    `DrawSelectedMarkerInstance` inside its own row-body lambda, lines 137-138 — STEP110's row-based
    indirection; `DrawMarkerInstanceSection` no longer calls `DrawSelectedMarkerInstance` directly)
    — only `selectedMarkerLayerIndex` is new.
  - `DrawMarkerInstanceSection`'s one call site, `MarkersTab_Manual_UI.cpp:117`
    (`DrawMarkerInstanceSection(*group, armies, markerLayers, state);`) inside `DrawManualMarkers`
    (`MarkersTab_Manual_UI.cpp:105-120` — corrected line range, function body unchanged, declared
    `MarkersTab_Manual_UI.h:158-161`): `DrawManualMarkers`
    itself gains the same new `int selectedMarkerLayerIndex` parameter, passed through to line 117's
    call: `DrawMarkerInstanceSection(*group, armies, markerLayers, state, selectedMarkerLayerIndex);`.
  - `DrawManualMarkers`'s one call site, `MarkersTab_UI.cpp:54`
    (`DrawManualMarkers(recipe.markers, recipe.armies, recipe.markerLayers, state.manual);`) becomes
    `DrawManualMarkers(recipe.markers, recipe.armies, recipe.markerLayers, state.manual,
    state.manualLayers.selectedLayerIndex);` — `state.manualLayers` is already populated earlier the
    same frame (`MarkersTab_UI.cpp:49`'s `DrawManualMarkerLayers` call, which runs BEFORE this one
    per the existing STEP81 ordering comment at that line — so this frame's own layer selection,
    including any change made this frame, is what the Add-Instance gate/target sees, no one-frame lag).

### 5. Enforcement gate wiring — `DrawLayerList`/`ApplyLayerListSignal`
**Corrected for `04750e4`.** Both functions' SIGNATURES changed under STEP110's row-based refactor
(the row-describe lambda no longer takes a bare `selectedLayerIndex` int — `DrawLayerList` now reads
`state.selectedLayerIndex` directly off a `ManualMarkerLayersState&` it takes wholesale — and it
gained a `bool& bAnyNameCommitted` out-param feeding the Name-field uniqueness repair). Neither of
those STEP110 changes is this ticket's concern; the fix below only adds to what is already there.

`MarkersTab_ManualLayers_UI.cpp`'s `DrawLayerList` (lines 60-74) and `ApplyLayerListSignal`
(lines 81-100) already receive `ToggleLock` from the shared `DraggableList` widget but do nothing
with it (dead signal — confirmed, Problem section). Wire it:

`DrawLayerList` — set `row.bLocked` from the real field, inside the existing row-describe lambda
(current lines 64-68), so the widget's `[L]`/`[U]` icon reflects actual state. One new line, nothing
else in the function changes:
```cpp
DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label   = ManualMarkerLayerRowLabel(markerLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = markerLayers[static_cast<std::size_t>(rowIndex)].bLocked;   // NEW
            return row;
        },
        [&](int rowIndex) {
            if (DrawLayerRowBody(markerLayers[static_cast<std::size_t>(rowIndex)], state))
                bAnyNameCommitted = true;
        },
        state.selectedLayerIndex);
}
```

`ApplyLayerListSignal` — add a `ToggleLock` branch that flips `bLocked` on the target layer,
immediately after the existing `kind == DraggableListSignalKind::Select` branch (current lines
84-87), before the `const bool bDeleting = ...` line (current line 88):
```cpp
if (signal.kind == DraggableListSignalKind::ToggleLock) {
    if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(markerLayers.size()))
        markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked =
            !markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked;
    return false;   // cosmetic-only: no structural move, same posture as Select
}
```
Insert this branch so it returns early like `Select` does, not falling through into the
structural-signal handling below it.

**Do not touch or reuse the padlock-icon-means-"hidden" convention in
`MarkersTab_RuleLayers_UI.cpp`.** That is a separate, already-flagged pre-existing inconsistency
(a different tab's lock icon means something else entirely) — explicitly out of scope for this
ticket. `MarkerInstanceLayer::bLocked` and this ticket's enforcement gate are the only lock
semantics this ticket defines, scoped to the Manual Marker Layers block only.

### 6. Grid snap — quantization function + write-path call sites
**Per-layer, not global — a deliberate, already-confirmed product decision, stated here explicitly
so it is not "fixed" later as a bug**: each marker snaps by its OWN layer's `bGridSnapEnabled`/
`gridSnapSizeWorldUnits`, resolved via `transform.layerIndex` at write time — there is no single
global snap setting anywhere in this design. Accepted consequence: if a symmetry group's siblings
sit on different layers with different snap settings (or one snapping, one not), independent
per-layer quantization of the dragged member and each sibling can break exact mirror symmetry after
the write. This is intentional — grid snap is a per-layer authoring convenience, not a symmetry
guarantee, and the two features are allowed to interact this way. Do not add cross-layer
snap-setting reconciliation to "fix" this; it is not a defect.

New function, `src/ui/MarkersTab_ManualLayers_UI.h`, beside `IsMarkerInstanceLayerLocked` (§3):
```cpp
// The world position `(worldX, worldZ)` quantized to `layerIndex`'s own grid setting, or
// unchanged if that layer has grid snap off, is out of range (Constitution §6 — resolves to
// unchanged, the same posture as IsMarkerInstanceLayerLocked's out-of-range-safe default), or its
// own `gridSnapSizeWorldUnits` is non-positive (a non-positive cell size cannot quantize; treated
// as snap-off rather than a divide-by-zero/no-op hazard).
inline void QuantizeMarkerPositionToLayerGrid(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                              int layerIndex, float& worldX, float& worldZ) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return;
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    if (!layer.bGridSnapEnabled || layer.gridSnapSizeWorldUnits <= 0.0f) return;
    const float cellSize = layer.gridSnapSizeWorldUnits;
    worldX = std::round(worldX / cellSize) * cellSize;
    worldZ = std::round(worldZ / cellSize) * cellSize;
}
```
`#include <cmath>` in `MarkersTab_ManualLayers_UI.h` for `std::round`.

Called from BOTH write paths, each resolving snap by the transform actually being written (not a
single global setting, not the dragged member's layer for siblings — every transform snaps by its
OWN `layerIndex`'s setting, consistent with the accepted per-layer-divergence tradeoff above):

- **Drag-continue path** — `UpdateMarkerDragGesture`, `src/ui/MarkerDragGesture_Frame_UI.cpp`
  (declared `MarkerDragGesture_UI.h:108-109`; this file was not read in full for this ticket — the
  Coder must locate the exact per-transform write inside it before editing). That function writes
  the dragged member's position unconditionally each frame, and (cardinality-permitting) every
  matched sibling's position too, per its own header comment (`MarkerDragGesture_UI.h:101-109`).
  At EVERY point in that function's body where a `MarkerTransform::transform.positionX/positionZ`
  pair is assigned (the dragged member's own write, and each sibling's write inside its matching
  loop), insert a `QuantizeMarkerPositionToLayerGrid(markerLayers, <that transform's own
  layerIndex>, newPositionX, newPositionZ)` call immediately before the assignment, so the
  quantized value is what gets written — not a post-hoc correction after the unquantized value is
  already stored. `UpdateMarkerDragGesture`'s signature
  (`void UpdateMarkerDragGesture(MarkerDragGestureState& state,
  std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry, float
  newWorldX, float newWorldZ);`) has no `markerLayers` parameter today — add one:
  `void UpdateMarkerDragGesture(MarkerDragGestureState& state,
  std::vector<Params::MarkerInstanceGroup>& markers,
  const std::vector<Params::MarkerInstanceLayer>& markerLayers, const Params::Geometry& geometry,
  float newWorldX, float newWorldZ);` — update the declaration (`MarkerDragGesture_UI.h:108-109`)
  and the definition in `MarkerDragGesture_Frame_UI.cpp:11-12` (insert the new parameter between
  `markers` and `geometry`, matching `BeginMarkerDragGesture`'s existing parameter order).

  **The one production call site** — `MapCanvas::ContinueManualMarkerDrag`
  (`src/ui/MapCanvas_MarkerDrag_UI.cpp:116-123`):
  ```cpp
  void MapCanvas::ContinueManualMarkerDrag(float regionLocalX, float regionLocalY) {
      if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr || composite == nullptr) return;
      const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
      const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
          static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
      UpdateMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers, *manualMarkerDragGeometry,
                             worldPoint.worldX, worldPoint.worldZ);
  }
  ```
  becomes (mirroring the `manualMarkerDragLayers`/`kNoLayers` null-safe pattern the sibling function
  `MapCanvas::TryBeginManualMarkerDrag` already uses (`MapCanvas_MarkerDrag_UI.cpp:101-114`, the
  `BeginMarkerDragGesture` call itself at lines 108-113) for the identical purpose — same class
  member, same fallback, do not invent a different null-handling shape here):
  ```cpp
  void MapCanvas::ContinueManualMarkerDrag(float regionLocalX, float regionLocalY) {
      if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr || composite == nullptr) return;
      const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
      const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
          static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
      static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
      UpdateMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers,
                             manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                             *manualMarkerDragGeometry, worldPoint.worldX, worldPoint.worldZ);
  }
  ```
  `manualMarkerDragLayers` is the same `const std::vector<Params::MarkerInstanceLayer>*` class member
  `TryBeginManualMarkerDrag` already reads — no new member needed.

  **Test call sites** — `src/ui/MarkerDragGesture_UI_Test.cpp` calls `UpdateMarkerDragGesture(state,
  markers, geometry, ...)` at lines 63, 70, 90, 96, 101, 120, 152, 181, 185, 213 (ten call sites,
  confirmed by grep). Every one must gain a `markerLayers` argument (an empty
  `std::vector<Params::MarkerInstanceLayer>` is correct for any existing test that predates grid
  snap and asserts no snap behavior) in the same position as the production call site above — update
  all ten, do not leave any on the old four-argument shape.

- **Roster Position-slider path** — `DrawSelectedMarkerInstance`,
  `src/ui/MarkersTab_ManualInstance_UI.cpp:84-117` (corrected from the pre-`04750e4` line range;
  the same function §4 already edits for the lock
  gate — apply both edits together, not as two separate passes). After each of the three
  `DrawSliderScalar("Position X"/"Position Y (Elevation)"/"Position Z", ...)` calls reports
  `bCommitted` (a slider's `WidgetChange::bCommitted`, i.e. the drag/edit just finished — snapping
  mid-drag every frame would fight the user's cursor), quantize X/Z together (Y/elevation is never
  snapped — grid snap is a horizontal-plane concept here, consistent with `positionHorizontalRange`
  vs. `positionElevationRange` already being separate ranges in this same file):
  ```cpp
  const WidgetChange positionXChange = DrawSliderScalar("Position X", transform.transform.positionX,
      state.positionHorizontalRange, state.positionXToggle, WidgetStyle(), "%.1f");
  const WidgetChange positionYChange = DrawSliderScalar("Position Y (Elevation)", transform.transform.positionY,
      state.positionElevationRange, state.positionYToggle, WidgetStyle(), "%.1f");
  const WidgetChange positionZChange = DrawSliderScalar("Position Z", transform.transform.positionZ,
      state.positionHorizontalRange, state.positionZToggle, WidgetStyle(), "%.1f");
  if (positionXChange.bCommitted || positionZChange.bCommitted)
      QuantizeMarkerPositionToLayerGrid(markerLayers, transform.layerIndex,
                                        transform.transform.positionX, transform.transform.positionZ);
  bCommitted = positionXChange.bCommitted || positionYChange.bCommitted || positionZChange.bCommitted || bCommitted;
  ```
  This replaces the three-slider block already being edited in §4 for `BeginDisabled`/`EndDisabled` —
  apply both changes to the same block as one combined edit (lock-disable wraps the whole block;
  quantize-on-commit happens inside it, after all three sliders have drawn and reported their own
  `bCommitted`, so a mid-drag frame is never snapped, only the frame the user releases).
  Confirm `WidgetChange`'s exact member name is `bCommitted` against `SliderScalar_UI.h` before
  writing this — every other call site in this file already reads `.bCommitted` off a
  `DrawSliderScalar`/`DrawSliderScalar`-family return, so this is consistent, not a new assumption.

### 7. UI placement — "Snap to Grid" / "Grid Size" controls, `DrawLayerRowBody`
**Corrected for `04750e4`.** The pre-restructure `DrawSelectedLayer` (a bottom-of-tab panel drawn
once for whatever `state.selectedLayerIndex` pointed at) no longer exists. STEP110's row-based
refactor replaced it with `DrawLayerRowBody` (`src/ui/MarkersTab_ManualLayers_UI.cpp:36-52`), drawn
once per row, inline, whenever THAT row's own `DraggableList` `CollapsingHeader` is expanded — never
gated on `state.selectedLayerIndex`. Its parameter is a direct reference,
`Params::MarkerInstanceLayer& layer` (not the pointer `DrawSelectedLayer` used), so every field
access below is `layer.`, not `layer->`, matching this function's own existing `layer.name`/
`layer.color`/`layer.iconScale` accesses. The insertion point relative to its neighbors is
unchanged from the original spec: `DrawLayerRowBody` draws Name, Color, Icon Scale, then (current
lines 46-50) the "Layer Symmetry" section. Insert two flat controls AFTER the Icon Scale slider
(current lines 44-45, `DrawSliderScalar("Icon Scale", ...)`), BEFORE the
`DrawSectionBegin("Layer Symmetry", ...)` call (current line 46):
```cpp
const bool bSnapCommitted = DrawCheckbox("Snap to Grid", layer.bGridSnapEnabled).bCommitted;
ImGui::BeginDisabled(!layer.bGridSnapEnabled);
const bool bSnapSizeCommitted = DrawSliderScalar("Grid Size", layer.gridSnapSizeWorldUnits,
    state.gridSnapSizeRange, state.selectedLayerGridSnapToggle, WidgetStyle(), "%.2f").bCommitted;
ImGui::EndDisabled();
```
Fold `bSnapCommitted`/`bSnapSizeCommitted` into the function's existing `bNameCommitted` return
composition (current line 51's `return bNameCommitted;` becomes
`return bNameCommitted || bSnapCommitted || bSnapSizeCommitted;`). This return value already feeds
`bAnyNameCommitted` in `DrawLayerList`'s row-body lambda (§5 above, current lines 69-72), which in
turn drives `DrawManualMarkerLayers`'s `MakeNamesUnique` repair call — STEP110 already built this
fold-up path for the Name field; the two new controls ride it for free, no new plumbing needed.

`Checkbox_UI.h` is now ALREADY included by `MarkersTab_ManualLayers_UI.cpp` (current line 6) — it
was added since this ticket was first drafted (`DrawLayerSettings`'s "Use Group Color" checkbox,
current lines 20-27, already uses it). No new include needed for `DrawCheckbox` here. Confirm
`DrawCheckbox`'s exact signature and return-type member name against `Checkbox_UI.h` before writing
this call — every other Checkbox call site in the codebase (e.g. `MarkersTab_Manual_UI.cpp:24`,
`DrawCheckbox("Resource", group.bResource).bCommitted`) uses the same `.bCommitted` shape, so this
is consistent, not a new assumption.

New state fields — `ManualMarkerLayersState`, `src/ui/MarkersTab_ManualLayers_UI.h:34-53`, next to
the existing `iconScaleRange`/`selectedLayerIconScaleToggle` pair (matching that exact pattern —
one shared `ScalarSliderRange` for the block, one `RealtimeToggle` for the selected row's own
control):
```cpp
ScalarSliderRange  gridSnapSizeRange{ 0.1f, 100.0f, 0.0f };   // Constitution §8 — a setting, not a
                                                               // literal at the DrawSliderScalar call
RealtimeToggle     selectedLayerGridSnapToggle;
```
Add these next to `iconScaleRange` (line 37) and `selectedLayerIconScaleToggle` (line 49)
respectively, keeping each new field beside its same-shape sibling.

## Out of scope
- **Any global (non-per-layer) snap setting.** Explicitly not built — see §6's accepted-consequence
  note. Do not add one to "resolve" the symmetry-divergence tradeoff.
- **`MarkersTab_RuleLayers_UI.cpp`'s padlock-icon-means-"hidden" convention.** Pre-existing,
  already-flagged inconsistency in a different tab; untouched by this ticket.
- **Y/elevation grid snap.** Only X/Z (the horizontal plane) snap; `positionY` is never quantized.
- **Snap-on-import/export normalization.** A `.sanmap` loaded with `bGridSnapEnabled = true` but
  positions that are not multiples of `gridSnapSizeWorldUnits` (e.g. hand-edited JSON, or authored
  before snap was turned on) is not silently re-snapped on load — snap only applies going forward,
  on the next interactive write. No import-time pass added.
- **Canvas-level visual grid overlay** (drawing gridlines on the map preview). This ticket is the
  data model + enforcement + the two flat authoring controls only; no rendering/overlay consumer.
- **STEP107** — a separate ticket also editing `DrawLayerRowBody` (STEP110's row-based replacement
  for the pre-restructure `DrawSelectedLayer`). Must not be implemented concurrently; land this
  ticket first.
- **Locking/grid-snapping Props or Decals layers.** `PropInstanceLayer`/`DecalInstanceLayer` are
  untouched — this ticket is markers-only, matching the domain split every prior marker-layer ticket
  (STEP60, STEP68, STEP81) already uses.
- **Any change to `layerId`, `symmetry`, or the existing Symmetry section's controls.** Untouched
  except for the two new controls' insertion point relative to them.

## Files touched
- `src/params/MarkerInstance_PARAMS.h` — `bLocked`, `bGridSnapEnabled`, `gridSnapSizeWorldUnits` on
  `MarkerInstanceLayer`
- `src/io/MapExporter_Markers_IO.cpp` — `BuildMarkerGroupsJson` writes `"Locked"`,
  `"GridSnapEnabled"`, `"GridSnapSizeWorldUnits"`
- `src/io/MapImporter_Markers_IO.cpp` — `ReadMarkerGroupsJson` reads the same three keys
- `src/ui/MarkersTab_ManualLayers_UI.h` — new `IsMarkerInstanceLayerLocked`,
  `QuantizeMarkerPositionToLayerGrid`; `ManualMarkerLayersState` gains `gridSnapSizeRange`,
  `selectedLayerGridSnapToggle`; `#include <cmath>`
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `DrawLayerList` sets `row.bLocked`; `ApplyLayerListSignal`
  handles `ToggleLock`; `DrawLayerRowBody` (STEP110's row-based replacement for the pre-restructure
  `DrawSelectedLayer`) gains the Snap to Grid / Grid Size controls. `Checkbox_UI.h` is already
  included — no include change needed here.
- `src/ui/MarkerDragGesture_UI.h` — `BeginMarkerDragGesture`/`RepositionSymmetryGroupMember` gain no
  new parameters (both already take `markerLayers`); `UpdateMarkerDragGesture`'s declaration gains a
  `markerLayers` parameter; `#include "MarkersTab_ManualLayers_UI.h"`
- `src/ui/MarkerDragGesture_UI.cpp` — `BeginMarkerDragGesture` lock check;
  `RepositionSymmetryGroupMember` lock check
- `src/ui/MarkerDragGesture_Frame_UI.cpp` — `UpdateMarkerDragGesture` gains `markerLayers` parameter
  and quantizes every per-frame position write (dragged member + each matched sibling)
- `src/ui/MapCanvas_MarkerDrag_UI.cpp` — `ContinueManualMarkerDrag` passes `manualMarkerDragLayers`
  (with the existing `kNoLayers` null-safe fallback) to `UpdateMarkerDragGesture`
- `src/ui/MarkerDragGesture_UI_Test.cpp` — all ten existing `UpdateMarkerDragGesture` call sites
  (lines 63, 70, 90, 96, 101, 120, 152, 181, 185, 213) updated to pass a `markerLayers` argument
- `src/ui/MarkersTab_ManualInstance_UI.cpp` — `DrawMarkerInstanceListButtons` gains
  `markerLayers`/`selectedMarkerLayerIndex` parameters, gates Add/Remove on lock, Add uses the
  selected marker layer instead of the default `layerIndex = 0`; `DrawSelectedMarkerInstance` gates
  the three Position sliders on lock and quantizes X/Z on commit; `DrawMarkerInstanceSection` gains
  `selectedMarkerLayerIndex` parameter
- `src/ui/MarkersTab_Manual_UI.h` — `DrawMarkerInstanceSection`/`DrawManualMarkers` declarations gain
  `selectedMarkerLayerIndex` parameter
- `src/ui/MarkersTab_Manual_UI.cpp` — `DrawManualMarkers` gains and threads
  `selectedMarkerLayerIndex`; its call to `DrawMarkerInstanceSection` passes it through
- `src/ui/MarkersTab_UI.cpp` — `DrawManualMarkers` call passes
  `state.manualLayers.selectedLayerIndex`

## Verify
Acceptance bar: both fields exist, round-trip through export/import (including legacy files with
neither key present), the lock gate refuses every enumerated call site and the icon reflects real
state, grid snap quantizes on commit only (never mid-drag) using each transform's own layer's
setting, with new/updated unit tests. No canvas/rendering acceptance bar — no overlay consumer
exists to test against.

- **New unit test — `MarkerGroups` round-trip**, extend the existing live-document fixture in
  `src/io/MapImporter_IO_Test.cpp` (`FillFixtureMarkersAndChains`/`CheckMarkersAndChains`, the same
  fixture STEP60's own test coverage extended): give the fixture's `MarkerInstanceLayer` non-default
  `bLocked = true`, `bGridSnapEnabled = true`, `gridSnapSizeWorldUnits = 4.0f`; assert all three
  survive `BuildSanmapJsonText`/`ParseSanmapJsonText`.
- **New unit test — legacy default**: hand-construct a `MarkerGroups` JSON array entry with none of
  `"Locked"`/`"GridSnapEnabled"`/`"GridSnapSizeWorldUnits"` present, call `ReadMarkerGroupsJson`
  directly, assert `bLocked == false`, `bGridSnapEnabled == false`, `gridSnapSizeWorldUnits == 1.0f`
  (struct defaults, untouched).
- **New unit test — `IsMarkerInstanceLayerLocked`**: empty vector and any index returns `false`; a
  vector of two layers with `[1].bLocked = true` returns `false` for index 0, `true` for index 1;
  an out-of-range index (negative and `>= size()`) against a non-empty vector returns `false`.
- **New unit test — `QuantizeMarkerPositionToLayerGrid`**: a layer with `bGridSnapEnabled = false`
  leaves `(worldX, worldZ)` unchanged regardless of value; a layer with `bGridSnapEnabled = true`,
  `gridSnapSizeWorldUnits = 4.0f` snaps `(6.1, -3.9)` to `(8.0, -4.0)` (round-to-nearest-cell,
  confirm the exact expected values against `std::round`'s ties-away-from-zero behavior at a tie
  case like `2.0` against a `4.0` cell); a layer with `gridSnapSizeWorldUnits <= 0.0f` (e.g. `0.0f`
  or `-1.0f`) and `bGridSnapEnabled = true` leaves the position unchanged (defensive no-op, not a
  divide-by-zero); an out-of-range `layerIndex` leaves the position unchanged.
- **New unit test — lock refuses `BeginMarkerDragGesture`**, `src/ui/MarkerDragGesture_UI_Test.cpp`
  (this file already exercises `BeginMarkerDragGesture` at lines 59, 87, 117, 149, 178, 209, each
  passing `{}`/`noLayers` for the `markerLayers` parameter today — add a new check function
  alongside the existing ones, called from the file's `main`/runner the same way
  `RunRepositionSymmetryGroupMemberChecks` is at line 260): build a one-entry `markerLayers` vector
  with `bLocked = true` at index 0, a `markers` fixture whose dragged transform has `layerIndex = 0`;
  call `BeginMarkerDragGesture` with a valid `groupIndex`/`transformIndex` resolving to that
  transform; assert it returns `false` and `state.bActive == false` (mirrors this file's existing
  `Check(BeginMarkerDragGesture(...), ...)` assertion shape, e.g. line 59/209).
- **New unit test — lock refuses `RepositionSymmetryGroupMember`**, same file, alongside
  `RunRepositionSymmetryGroupMemberChecks` (lines 229-243, which already builds a `noLayers`/`markers`
  fixture and asserts both a `true` ordinary-case return at line 236 and a `false` refusal-case
  return at line 242): add a case with a one-entry `markerLayers` vector with `bLocked = true`
  matching the moved transform's `layerIndex`; assert `RepositionSymmetryGroupMember` returns `false`
  and the transform's position is unchanged from its pre-call value.
- **New unit test — Add Instance uses selected marker layer**: call
  `DrawMarkerInstanceListButtons`'s underlying logic (or a refactored pure helper if the Coder judges
  the imgui-coupled function itself untestable headlessly — check whether this file already has a
  headless test seam before assuming one must be added) with a non-zero
  `selectedMarkerLayerIndex` against a `markerLayers` vector large enough to contain it; assert the
  newly-pushed `MarkerTransform::layerIndex` equals that index, not `0`. If no headless seam exists
  for this specific function today, this assertion may instead target the smaller, definitely-pure
  pieces this ticket adds (`IsMarkerInstanceLayerLocked`, `QuantizeMarkerPositionToLayerGrid`) and
  defer the full button-click path to manual verification — do not invent a new imgui test harness
  for this ticket alone.
- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `MapExporter_IO_Test`, `MapImporter_IO_Test` (all non-lock/non-snap fixtures/checks
  byte-identical), and any existing `MarkerDragGesture`/`MarkersTab` UI-logic test binary — the
  `UpdateMarkerDragGesture` signature change means every existing call site and every existing test
  calling it directly must be updated to pass `markerLayers`, not left broken.
