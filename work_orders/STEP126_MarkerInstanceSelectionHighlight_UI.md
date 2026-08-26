# STEP126 — Manual Marker Instance Selection + Highlight (ARCH §19.16/§19.18/§19.19/§19.20, Ticket C)

**Layer:** UI. **Domain:** the per-Layer manual-instance list (`MarkersTab_ManualLayerRowBody_UI.cpp`),
a new top-level `MarkersTabState::selectedManualInstanceIdentifier`, the `NextMarkerInstanceIdentifier`
minting helper and its "Add Instance" call site, `MapCanvas`'s selection-highlight source/computation,
and `DrawManualMarkerRoster`'s tint-priority rewrite (`MapCanvas_MarkerDrag_UI.*`).
**Sequence:** depends on STEP124 (PARAMS/IO, Ticket A) — confirmed landed against live code this
session: `Params::MarkerTransform::instanceIdentifier` (`MarkerInstance_PARAMS.h:77`) and the four
`GlobalMarkerSettings::selectColor*[4]` fields + `ResolveMarkerGroupSelectTintColor`
(`GlobalMarkerSettings_PARAMS.h:28-31,69-76`) are both live. Independent of Ticket B (the Type-section
tab restructure, not yet drafted) — this ticket touches none of Ticket B's files. This is the ticket
`work_orders/STEP124_MarkerTypeSectionsParamsIO_PARAMS.md` explicitly deferred
`NextMarkerInstanceIdentifier` to (its own "Scope correction" section).

Ratifies `work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s "Item 4 / Open Q2, Q3,
Q7, Q4, Q5" section per `ARCH_19_16_InstanceIdentifier.md` (minting), `ARCH_19_17_SelectColorFields.md`
(already landed, STEP124), `ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md` (tint priority),
`ARCH_19_19_StaticHighlightComputationAndWiring.md` (sibling-orbit computation + `MapCanvas` wiring),
and `ARCH_19_20_ManualOnlySelectionScope.md` (manual-only, formal law). **Open Q7's own instance-list-
per-Layer UI shape has no dedicated `ARCH_19_2x` ruling** — it is absent from the design doc's own
"ARCH module-boundary rulings needed" list (items 1-14; none of them name Q7's UI shape) — so this
ticket follows the design doc's own text for that one piece directly, flagged here rather than
silently treated as ARCH-ratified.

## Problem

Three gaps block a working selection/highlight feature, each confirmed by direct read this session
against the CURRENT (post-STEP124) code, not assumed from the design doc's own text:

1. **No minting helper, and the "Add Instance" call site never stamps `instanceIdentifier` at all.**
   `MarkersTab_ManualInstance_UI.cpp:45-54`, `DrawMarkerInstanceListButtons`'s "Add Instance" button
   constructs a fresh `Params::MarkerTransform` and sets `.name` via `NextMarkerInstanceName`, but
   never touches `.instanceIdentifier` — it is left at its struct default, `-1`, for every
   freshly-authored marker. Confirmed: `DrawMarkerInstanceListButtons` receives only
   `std::vector<Params::MarkerTransform>& transforms` (the SELECTED group's own roster), not the full
   `recipe.markers` `NextMarkerInstanceIdentifier` needs for its cross-group max-scan — the call chain
   `DrawManualMarkers` (has `markers`) → `DrawMarkerInstanceSection` (has only `*group`) →
   `DrawMarkerInstanceListButtons` does not thread the full vector down. A real, load-bearing plumbing
   gap, not a one-line fix.
2. **No per-Layer instance-list UI, no selection state.** `DrawLayerRowBody`
   (`MarkersTab_ManualLayerRowBody_UI.cpp:23-50`) draws name/tint/icon-scale/grid-snap/symmetry only —
   nothing lists the transforms tagged to that layer, and `MarkersTabState`
   (`MarkersTab_UI.h:52-109`) carries no selection field of the shape Open Q7 needs (a value visible
   regardless of which Layer row happens to be expanded).
3. **No highlight computation, no `MapCanvas` wiring, no tint-priority "selected" branch.**
   `DrawManualMarkerRoster`'s tint branch chain (`MapCanvas_MarkerDrag_UI.cpp:119-127`) is confirmed
   still exactly refused-red → Spawn-army-color → layer/type-color, three branches, no "selected"
   branch. `MapCanvas_UI.h` has no `SetManualMarkerSelectionSource`/`manualMarkerSelectedInstanceIdentifier`
   pair (confirmed by full read — only `SetManualMarkerDragSource` and its bundle of pointers exist,
   `MapCanvas_UI.h:100-112,185-192`).

**File-size ceiling — every file this ticket's own edits land in was re-measured fresh this session
(`wc -l`, not trusted from any prior ticket's text), and the picture is worse than "near-ceiling":
five of the files this ticket must edit are ALREADY over ARCH §1.5's 150-line hard ceiling, pre-existing
and unremediated, before this ticket adds a single line:**

| File | Current lines | Hard ceiling | Status |
|---|---|---|---|
| `MapCanvas_MarkerDrag_UI.cpp` | 207 | 150 | already over, unremediated |
| `MapCanvas_UI.h` | 196 | 150 | already over, unremediated |
| `MarkersTab_ManualInstance_UI.cpp` | 216 | 150 | already over, unremediated |
| `MarkersTab_Manual_UI.h` | 184 | 150 | already over, unremediated |
| `MarkersTab_ManualLayers_UI.h` | 165 | 150 | already over, unremediated |
| `MarkersTab_Bundles_UI.cpp` | 138 | 150 | under, but little headroom |
| `MarkersTab_ManualLayerRowBody_UI.cpp` | 81 | 150 (100 soft) | under, real headroom |
| `MarkersTab_ManualLayers_UI.cpp` | 129 | 150 | under, real headroom |

The design doc's own text (`DESIGN_...R1.md`'s delivery-split section) called
`MapCanvas_MarkerDrag_UI.cpp` "already flagged as near-ceiling in a prior ticket" — direct measurement
this session shows it is not near the ceiling, it is 57 lines PAST it. See "File-size ceiling" below for
the disposition of each row (required split, flagged-but-not-required, or out-of-scope-follow-up).

## Fix

### 1. Minting helper — new `src/ui/MarkerInstanceId_UI.h`

Per ARCH §19.16, same shape as `NextMarkerLayerId` (`MarkerLayerId_UI.h`), one tier down (two-level
walk: group, then transform). **Placement reasoning, since the task explicitly asks to confirm the
right file by reading both existing precedents first:** `NextMarkerLayerBundleId` lives INLINE in its
own consuming header, `MarkersTab_Bundles_UI.h` (120 lines — real headroom under the ceiling when
STEP120 placed it there). `NextMarkerLayerId` lives in its OWN dedicated file, `MarkerLayerId_UI.h`,
because at STEP49-era no host header existed yet. For `NextMarkerInstanceIdentifier`, the naturally
analogous host — `MarkersTab_Manual_UI.h`, right beside the existing `NextMarkerInstanceName`
(`MarkersTab_Manual_UI.h:126`) — is confirmed THIS SESSION to already be 184 lines, over the hard
ceiling, unremediated, predating this ticket. Landing a new inline function there (the
`NextMarkerLayerBundleId` precedent) would be a further silent ratchet on an already-broken ceiling.
**Ruled: follow `NextMarkerLayerId`'s dedicated-file precedent instead** — the precedent that applies
is conditioned on host headroom, not a blanket "small helpers go inline" rule, and
`MarkersTab_Manual_UI.h` has none.

```cpp
// MarkerInstanceId_UI.h — mirrors MarkerLayerId_UI.h's own single-purpose-file precedent one tier
// down (ARCH §19.16). MarkersTab_Manual_UI.h — the natural host, alongside NextMarkerInstanceName —
// is already 184 lines, over ARCH §1.5's 150-line hard ceiling, unremediated, predating this ticket;
// landing this helper there inline (NextMarkerLayerBundleId's own precedent) would be a further
// silent ratchet. NextMarkerLayerId's dedicated-file precedent is the one that actually applies here:
// that precedent is conditioned on the host having headroom (MarkersTab_Bundles_UI.h had 120 lines
// when STEP120 placed NextMarkerLayerBundleId there inline); MarkersTab_Manual_UI.h does not.
#pragma once
#include <algorithm>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// ARCH §19.16 — scans max(instanceIdentifier) + 1 across EVERY group's transforms (roster-wide, not
// per-group) — same shape as NextMarkerLayerId, one tier down (two-level walk: group, then transform).
inline int NextMarkerInstanceIdentifier(const std::vector<Params::MarkerInstanceGroup>& markers) {
    int maximumId = -1;
    for (const Params::MarkerInstanceGroup& group : markers)
        for (const Params::MarkerTransform& transform : group.transforms)
            maximumId = std::max(maximumId, transform.instanceIdentifier);
    return maximumId + 1;
}

} // namespace Ui
} // namespace SanmapGen
```

### 2. Wire minting into "Add Instance" — thread `markers` down the call chain

`DrawMarkerInstanceListButtons` only receives the SELECTED group's own `transforms`; the full
`recipe.markers` must be threaded down from `DrawManualMarkers` (which already has it) through
`DrawMarkerInstanceSection`.

**`MarkersTab_Manual_UI.h`** — `DrawMarkerInstanceSection` declaration (lines 166-170), new first
parameter after `group`:
```cpp
void DrawMarkerInstanceSection(Params::MarkerInstanceGroup& group,
                               const std::vector<Params::MarkerInstanceGroup>& markers,   // NEW — STEP126, for NextMarkerInstanceIdentifier's global-uniqueness scan
                               const std::vector<Params::Army>& armies,
                               const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                               ManualMarkersState& state, int selectedMarkerLayerIndex,
                               const IconAtlasManifest* iconManifest);
```

**`MarkersTab_Manual_UI.cpp`** — `DrawManualMarkers`'s call site (line 118):
```cpp
DrawMarkerInstanceSection(*group, markers, armies, markerLayers, state, selectedMarkerLayerIndex, iconManifest);
```

**`MarkersTab_ManualInstance_UI.cpp`** — `#include "MarkerInstanceId_UI.h"` added to the include
block (line 7 area). `DrawMarkerInstanceListButtons` (lines 40-54) gains the same new parameter and
stamps the identifier:
```cpp
bool DrawMarkerInstanceListButtons(std::vector<Params::MarkerTransform>& transforms,
                                   const std::vector<Params::MarkerInstanceGroup>& markers,   // NEW — STEP126
                                   ManualMarkersState& state,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   int selectedMarkerLayerIndex) {
    bool bInstancesMoved = false;
    ImGui::BeginDisabled(IsMarkerInstanceLayerLocked(markerLayers, selectedMarkerLayerIndex));
    if (ImGui::Button("Add Instance")) {
        Params::MarkerTransform transform;
        transform.name = NextMarkerInstanceName(static_cast<int>(transforms.size()));
        transform.instanceIdentifier = NextMarkerInstanceIdentifier(markers);   // NEW — ARCH §19.16
        transform.layerIndex = (selectedMarkerLayerIndex >= 0
                                && selectedMarkerLayerIndex < static_cast<int>(markerLayers.size()))
                               ? selectedMarkerLayerIndex : 0;
        transforms.push_back(transform);
        state.selectedInstanceIndex = static_cast<int>(transforms.size()) - 1;
        bInstancesMoved = true;
    }
    ImGui::EndDisabled();
    // ... Remove Selected block UNCHANGED ...
```
`DrawMarkerInstanceSection`'s own definition (lines 195-198) and its call to
`DrawMarkerInstanceListButtons` (lines 205-206) both gain the same `markers` pass-through:
```cpp
void DrawMarkerInstanceSection(Params::MarkerInstanceGroup& group,
                               const std::vector<Params::MarkerInstanceGroup>& markers,
                               const std::vector<Params::Army>& armies,
                               const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                               ManualMarkersState& state, int selectedMarkerLayerIndex,
                               const IconAtlasManifest* iconManifest) {
    bool bInstancesMoved = false;
    bool bAnyInstanceCommitted = false;
    const DraggableListSignal signal =
        DrawMarkerInstanceList(group.transforms, group, armies, markerLayers, state, bAnyInstanceCommitted);
    if (signal.bHasSignal())
        bInstancesMoved = ApplyMarkerInstanceListSignal(group.transforms, state, signal) || bInstancesMoved;
    bInstancesMoved = DrawMarkerInstanceListButtons(group.transforms, markers, state, markerLayers,
                                                    selectedMarkerLayerIndex) || bInstancesMoved;
    ...
```

### 3. Per-frame instance index — new `src/ui/ManualInstanceLayerIndex_UI.h`

Per Open Q7's own text: a per-frame `layerIndex -> (groupIndex, transformIndex)` index, mirroring
`BuildMarkerLayerBundleLeafIndex`'s exact shape one tier down. Built ONCE per frame by whichever
function iterates multiple `Params::MarkerInstanceLayer` rows in one frame
(`DrawManualMarkerLayers`/`DrawMarkerLayerBundleTree`), threaded down into `DrawLayerRowBody` — never
rebuilt per row.

```cpp
// ManualInstanceLayerIndex_UI.h — new, single-purpose (mirrors MarkerLayerId_UI.h's own "no host has
// room" shape): a per-frame index from layerIndex -> every (groupIndex, transformIndex) pair among
// recipe.markers whose own transform.layerIndex matches, consumed by DrawLayerRowBody's new per-Layer
// instance list (Open Q7, DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md — no dedicated
// ARCH_19_2x ruling exists for this UI shape; followed directly from the design doc's own text).
// Mirrors MarkerLayerBundleLeafIndex_UI/BuildMarkerLayerBundleLeafIndex (MarkersTab_Bundles_UI.h)
// one tier down — a (groupIndex, transformIndex) pair instead of a single leaf key.
#pragma once
#include <unordered_map>
#include <utility>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ManualInstanceLayerIndex_UI {
    // (groupIndex, transformIndex) — into recipe.markers[groupIndex].transforms[transformIndex].
    std::unordered_map<int, std::vector<std::pair<int, int>>> instancesByLayerIndex;
};

inline ManualInstanceLayerIndex_UI BuildManualInstanceLayerIndex(
        const std::vector<Params::MarkerInstanceGroup>& markers) {
    ManualInstanceLayerIndex_UI index;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()); ++groupIndex) {
        const std::vector<Params::MarkerTransform>& transforms =
            markers[static_cast<std::size_t>(groupIndex)].transforms;
        for (int transformIndex = 0; transformIndex < static_cast<int>(transforms.size()); ++transformIndex)
            index.instancesByLayerIndex[transforms[static_cast<std::size_t>(transformIndex)].layerIndex]
                .push_back({groupIndex, transformIndex});
    }
    return index;
}

} // namespace Ui
} // namespace SanmapGen
```

### 4. `MarkersTabState` gains the selection field — `MarkersTab_UI.h`

After the existing `bundles` field (line 108), before the struct's closing `};` (line 109):
```cpp
    // STEP126 — the SINGLE selection target for the per-Layer instance-list click (Open Q7) and the
    // MapCanvas static highlight (ARCH §19.19/§19.20, manual-only). Lives at the TOP level, not
    // inside ManualMarkerLayersState/ManualMarkersState, because it must stay visible regardless of
    // which Layer's own row body happens to be expanded when the click occurs — same reasoning
    // MarkerLayerBundlesState::selectedBundleIdentifier already applies one tier up its own struct.
    // -1 = no selection (Constitution §6 sentinel convention).
    int selectedManualInstanceIdentifier = -1;
```

### 5. `DrawLayerRowBody` gains the per-Layer instance list — `MarkersTab_ManualLayers_UI.h` / `MarkersTab_ManualLayerRowBody_UI.cpp`

**`MarkersTab_ManualLayers_UI.h`** — add `#include "ManualInstanceLayerIndex_UI.h"` to the include
block (line 22 area). `DrawLayerRowBody`'s declaration (lines 131-135) and `DrawManualMarkerLayers`'s
declaration (lines 157-162) each gain one new trailing parameter:
```cpp
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state,
                      const ManualInstanceLayerIndex_UI& instanceIndex, int& selectedManualInstanceIdentifier);
...
void DrawManualMarkerLayers(ManualMarkerLayersState& state,
                            std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers,
                            const Params::Geometry& geometry, int globalSymmetryMask,
                            int globalRadialRepeatCount,
                            Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                            int& selectedManualInstanceIdentifier);   // NEW — STEP126
```

**`MarkersTab_ManualLayerRowBody_UI.cpp`** — `#include <string>` added; `DrawLayerRowBody`'s signature
gains the same two trailing parameters, and its body gains the instance-list block after the existing
`DrawLayerSymmetrySection` call, before `return`:
```cpp
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state,
                      const ManualInstanceLayerIndex_UI& instanceIndex, int& selectedManualInstanceIdentifier) {
    ... existing body through the DrawLayerSymmetrySection call, UNCHANGED ...
    DrawLayerSymmetrySection(layer, layerIndex, markerLayers, markers, geometry, globalSymmetryMask,
                             globalRadialRepeatCount, markerSymmetryFixSettings, state);

    // STEP126, Open Q7 — the per-Layer instance list. Plain ImGui::Selectable rows, NOT a DraggableList
    // instantiation (an instance's own home group can differ from this Layer, so there is no single
    // homogeneous backing vector for a reorder/delete signal to apply against — see the design doc's
    // own reasoning for rejecting DraggableList<Params::MarkerTransform> here). No delete/reorder
    // affordance: deletion/repositioning stays owned by the roster editor (MarkersTab_Manual_UI.h).
    ImGui::Separator();
    ImGui::TextUnformatted("Instances");
    const auto instanceIt = instanceIndex.instancesByLayerIndex.find(layerIndex);
    if (instanceIt == instanceIndex.instancesByLayerIndex.end() || instanceIt->second.empty()) {
        ImGui::TextDisabled("(none)");
    } else {
        for (const std::pair<int, int>& groupTransformIndex : instanceIt->second) {
            const Params::MarkerInstanceGroup& instanceGroup =
                markers[static_cast<std::size_t>(groupTransformIndex.first)];
            const Params::MarkerTransform& instanceTransform =
                instanceGroup.transforms[static_cast<std::size_t>(groupTransformIndex.second)];
            const std::string rowLabel = instanceGroup.name + " - " + (!instanceTransform.name.empty()
                ? instanceTransform.name : std::to_string(groupTransformIndex.second));
            const bool bRowSelected = selectedManualInstanceIdentifier == instanceTransform.instanceIdentifier;
            if (ImGui::Selectable(rowLabel.c_str(), bRowSelected))
                selectedManualInstanceIdentifier = instanceTransform.instanceIdentifier;
        }
    }
    return bNameCommitted || bColorOverrideCommitted || bSnapCommitted || bSnapSizeCommitted;
}
```
A click here does NOT set the function's own `bXxxCommitted` return — it never moves `markerLayers`,
so no `MakeNamesUnique` repair is needed (matches Open Q7's own text). `layer.bLocked` is left
un-consulted by this block deliberately: the instance list is read/select-only (never edits a
transform), so the lock's existing "blocks drag/reposition/add/remove" contract has nothing to gate
here.

### 6. Threading the index + selection through both `DrawLayerRowBody` call sites

**`MarkersTab_ManualLayers_UI.cpp`** — `DrawLayerList` (lines 33-58) gains the same two trailing
parameters, threaded into its `DrawLayerRowBody` call:
```cpp
DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted,
                                  const ManualInstanceLayerIndex_UI& instanceIndex,
                                  int& selectedManualInstanceIdentifier) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) { ... UNCHANGED ... },
        [&](int rowIndex) {
            if (DrawLayerRowBody(markerLayers[static_cast<std::size_t>(rowIndex)], rowIndex, markerLayers, markers,
                                 geometry, globalSymmetryMask, globalRadialRepeatCount, markerSymmetryFixSettings,
                                 state, instanceIndex, selectedManualInstanceIdentifier))
                bAnyNameCommitted = true;
        },
        [&](int rowIndex) { ... header-extra callback UNCHANGED ... },
        kMarkerLayerColorOverrideHeaderWidthPixels,
        state.selectedLayerIndex);
}
```
`DrawManualMarkerLayers` (lines 110-126) builds the index once and threads the new parameter:
```cpp
void DrawManualMarkerLayers(ManualMarkerLayersState& state, std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                            int globalSymmetryMask, int globalRadialRepeatCount,
                            Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                            int& selectedManualInstanceIdentifier) {
    if (!DrawSectionBegin("Ungrouped Manual Marker Layers", state.section)) return;
    DrawLayerSettings(state);
    bool bLayersMoved = DrawLayerListButtons(markerLayers, state, -1);
    bool bAnyNameCommitted = false;
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);   // NEW
    const DraggableListSignal signal = DrawLayerList(markerLayers, markers, geometry, globalSymmetryMask,
        globalRadialRepeatCount, markerSymmetryFixSettings, state, bAnyNameCommitted,
        instanceIndex, selectedManualInstanceIdentifier);
    ... rest UNCHANGED ...
```

**`MarkersTab_Bundles_UI.cpp`** — `DrawMarkerGroupLeafBody` (lines 17-32, anonymous-namespace-local)
gains the same two parameters, threaded into its Manual-branch `DrawLayerRowBody` call only (the
Procedural branch/`DrawRuleLayerSettings` is untouched):
```cpp
void DrawMarkerGroupLeafBody(const MarkerGroupLeafKey_UI& leaf, std::vector<Params::MarkerRuleLayer>& ruleLayers,
                             std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                             std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                             int globalSymmetryMask, int globalRadialRepeatCount,
                             Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                             MarkersTabState& rootState, Pipeline::PreviewDriver* previewDriver,
                             const ManualInstanceLayerIndex_UI& instanceIndex) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural) {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(ruleLayers.size())) return;
        DrawRuleLayerSettings(ruleLayers[static_cast<std::size_t>(leaf.layerIndex)], previewDriver);
    } else {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        DrawLayerRowBody(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)], leaf.layerIndex,
                         instanceLayers, markers, geometry, globalSymmetryMask, globalRadialRepeatCount,
                         markerSymmetryFixSettings, rootState.manualLayers,
                         instanceIndex, rootState.selectedManualInstanceIdentifier);
    }
}
```
`DrawMarkerLayerBundleTree` (lines 62-135) builds the index once (mirroring `leafIndex`'s own
already-established once-per-call posture, line 81) and passes it into the `drawExpandedLeafBody`
lambda's `DrawMarkerGroupLeafBody` call:
```cpp
    const MarkerLayerBundleLeafIndex_UI leafIndex = BuildMarkerLayerBundleLeafIndex(ruleLayers, instanceLayers);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);   // NEW

    const TreeListSignal<MarkerGroupLeafKey_UI> signal = ...
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafBody(leaf, ruleLayers, instanceLayers, markers, geometry, globalSymmetryMask,
                                        globalRadialRepeatCount, markerSymmetryFixSettings, rootState, previewDriver,
                                        instanceIndex);
            },
            ...
```
No new parameter needed on `DrawMarkerLayerBundleTree` itself — `rootState.selectedManualInstanceIdentifier`
is already reachable through the existing `MarkersTabState& rootState` parameter.

### 7. `DrawMarkersTab` call-site update — `MarkersTab_UI.cpp`

Line 56-58, `DrawManualMarkerLayers`'s call gains the new trailing argument:
```cpp
DrawManualMarkerLayers(state.manualLayers, recipe.markerLayers, recipe.markers, recipe.geometry,
                      recipe.globalSymmetryMask, recipe.radialSymmetryRepeatCount,
                      recipe.markerSymmetryFixSettings, state.selectedManualInstanceIdentifier);
```
`DrawMarkerLayerBundleTree`'s own call (line 48-51) is UNCHANGED — it already threads `state` as
`rootState`.

### 8. Selection-highlight computation — new `src/ui/MarkerSelectionHighlight_UI.h` / `.cpp`

Per ARCH §19.19: fresh, one-shot, discarded every frame; UI-resident pure logic calling
`Pipeline::BuildWorldSymmetryOrbit`; explicitly NOT `MarkerOrbitCorrespondence_UI.h` (that matcher
solves a cross-frame drift problem this static feature doesn't have). Own file, mirroring
`MarkerDragGesture_UI.h`'s own "pure, imgui-free, testable with no window" split from the drawing half
— both a correct module-boundary choice per ARCH §19.19's own text AND the piece that keeps
`MapCanvas_MarkerDrag_UI.cpp`'s own required split (§10 below) from needing to carry this logic too.

```cpp
// MarkerSelectionHighlight_UI.h — the static selection-highlight computation (ARCH §19.19). Pure,
// imgui-free, testable with no window — same posture as MarkerDragGesture_UI.h/
// MarkerOrbitCorrespondence_UI.h. Deliberately NOT MarkerOrbitCorrespondence_UI.h's cross-frame
// matcher: this is a fresh, one-shot, discard-every-frame query, with none of that matcher's
// orbit-grows/shrinks-across-frames drift problem to solve. Calls the existing PIPELINE query
// Pipeline::BuildWorldSymmetryOrbit — UI -> PIPELINE, the already-legal query passthrough (ARCH
// §16.3) — no new PIPELINE surface.
#pragma once
#include <vector>
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Locates `selectedInstanceIdentifier` (ARCH §19.16) by linear scan across `markers`, resolves its
// effective symmetry (ResolveEffectiveMarkerSymmetry), queries Pipeline::BuildWorldSymmetryOrbit, and
// nearest-matches (within `distanceTolerance`) every orbit point beyond slot 0 against sibling
// transforms in the SAME MarkerInstanceGroup. Returns every matched instanceIdentifier, always
// including the selected instance itself as the first element. Returns empty for
// selectedInstanceIdentifier == -1 (no selection) or a stale identifier (deleted since selection —
// Constitution §6, never a crash). orbitCount <= 1 (no siblings, including the common "never
// dragged, symmetryGroupIdentifier == 0" case) returns just the selected instance, by construction —
// no special-casing needed (ARCH §19.19's own point: position-driven orbit matching subsumes it).
std::vector<int> ComputeManualMarkerSelectionHighlight(
    const std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
    float distanceTolerance, int selectedInstanceIdentifier);

} // namespace Ui
} // namespace SanmapGen
```

```cpp
// MarkerSelectionHighlight_UI.cpp
#include "MarkerSelectionHighlight_UI.h"
#include "MarkerDragGesture_UI.h"                    // ResolveEffectiveMarkerSymmetry
#include "../params/Symmetry_PARAMS.h"                // symmetryOrbitMaximum
#include "../pipeline/SymmetryOrbitQuery_PIPELINE.h"   // BuildWorldSymmetryOrbit

namespace SanmapGen {
namespace Ui {

std::vector<int> ComputeManualMarkerSelectionHighlight(
        const std::vector<Params::MarkerInstanceGroup>& markers,
        const std::vector<Params::MarkerInstanceLayer>& markerLayers,
        const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
        float distanceTolerance, int selectedInstanceIdentifier) {
    std::vector<int> result;
    if (selectedInstanceIdentifier < 0) return result;

    int selectedGroupIndex = -1, selectedTransformIndex = -1;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()) && selectedGroupIndex < 0; ++groupIndex) {
        const std::vector<Params::MarkerTransform>& transforms =
            markers[static_cast<std::size_t>(groupIndex)].transforms;
        for (int transformIndex = 0; transformIndex < static_cast<int>(transforms.size()); ++transformIndex)
            if (transforms[static_cast<std::size_t>(transformIndex)].instanceIdentifier == selectedInstanceIdentifier) {
                selectedGroupIndex = groupIndex; selectedTransformIndex = transformIndex; break;
            }
    }
    if (selectedGroupIndex < 0) return result;   // stale identifier — Constitution §6, never a crash

    const Params::MarkerInstanceGroup& group = markers[static_cast<std::size_t>(selectedGroupIndex)];
    const Params::MarkerTransform& selected = group.transforms[static_cast<std::size_t>(selectedTransformIndex)];
    result.push_back(selected.instanceIdentifier);

    int effectiveMask = 0, effectiveRepeatCount = 0;
    ResolveEffectiveMarkerSymmetry(markerLayers, selected.layerIndex, globalSymmetryMask,
                                   globalRadialRepeatCount, effectiveMask, effectiveRepeatCount);

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(geometry, effectiveMask, effectiveRepeatCount,
        selected.transform.positionX, selected.transform.positionZ, orbitPoints, Params::symmetryOrbitMaximum);
    if (orbitCount <= 1) return result;   // no siblings — highlight only the selected instance

    const float toleranceSquared = distanceTolerance * distanceTolerance;
    for (int orbitIndex = 1; orbitIndex < orbitCount; ++orbitIndex) {
        const Pipeline::WorldSymmetryOrbitPoint& orbitPoint = orbitPoints[orbitIndex];
        for (int transformIndex = 0; transformIndex < static_cast<int>(group.transforms.size()); ++transformIndex) {
            if (transformIndex == selectedTransformIndex) continue;
            const Params::MarkerTransform& candidate = group.transforms[static_cast<std::size_t>(transformIndex)];
            const float deltaX = candidate.transform.positionX - orbitPoint.worldPositionX;
            const float deltaZ = candidate.transform.positionZ - orbitPoint.worldPositionZ;
            if (deltaX * deltaX + deltaZ * deltaZ <= toleranceSquared) {
                result.push_back(candidate.instanceIdentifier);
                break;   // first match wins — same tie posture as HitTestManualMarkers
            }
        }
    }
    return result;
}

} // namespace Ui
} // namespace SanmapGen
```

### 9. `MapCanvas` wiring — `MapCanvas_UI.h` / `Application_UI.cpp`

Per ARCH §19.19, same null-safe-injection shape as `SetManualMarkerDragSource`, closer to
`SetActivePanelSource`'s single-pointer form. `MapCanvas_UI.h`, after the existing
`SetManualMarkerDragSource` method (line 112), before `Draw`:
```cpp
    // STEP126 — the static selection-highlight source: `selectedInstanceIdentifier` is the SAME
    // address as MarkersTabState::selectedManualInstanceIdentifier (Application_UI.cpp) — one source
    // of truth, never a second copy. A single scalar pointer, the simplest form of this file's own
    // established null-safe-injection shape (ARCH §19.19 — closer to SetActivePanelSource's
    // one-pointer form than SetManualMarkerDragSource's bundle). Null (no shell has wired a selection
    // source) refuses — the highlight computation treats null identically to "-1: nothing selected,"
    // never defaulting to "everything selected."
    void SetManualMarkerSelectionSource(const int* selectedInstanceIdentifier) {
        manualMarkerSelectedInstanceIdentifier = selectedInstanceIdentifier;
    }
```
Private member, beside `manualMarkerDragRecipe` (line 190):
```cpp
    const int*                                      manualMarkerSelectedInstanceIdentifier = nullptr;
```
**`Application_UI.cpp`**, `WireCallbacks` (right after the existing `SetManualMarkerDragSource` call,
line 101):
```cpp
    // STEP126 — the static selection-highlight source; see MapCanvas_UI.h's
    // SetManualMarkerSelectionSource. Points at the SAME MarkersTabState field the Markers tab's own
    // instance-list rows write (tabState.markers.selectedManualInstanceIdentifier) — one source of
    // truth, never a second copy.
    canvas.SetManualMarkerSelectionSource(&tabState.markers.selectedManualInstanceIdentifier);
```

### 10. Tint-priority rewrite + `MapCanvas_MarkerDrag_UI.cpp`'s required split

Per ARCH §19.18, insert the new "selected" branch at priority #2 (refused-drag still wins; selected
now wins over army/layer color). This lands inside `DrawManualMarkerRoster`, which — combined with the
new `selectedHighlightInstanceIdentifiers` parameter and its membership check — is real growth on top
of a file (`MapCanvas_MarkerDrag_UI.cpp`, 207 lines today) already 57 lines past the hard ceiling
BEFORE this ticket. **Required split** (Constitution §7 — a further silent ratchet is not acceptable),
along the file's own genuinely separable responsibilities — confirmed by direct read, these three
groups share no state and are already loosely coupled: (a) hit-testing, (b) roster drawing/tinting,
(c) `MapCanvas`'s own gesture-lifecycle method definitions. Three-way split, all still declared by the
UNCHANGED `MapCanvas_MarkerDrag_UI.h` (only `DrawManualMarkerRoster`'s own declared signature changes,
gaining the new parameter):

**New file — `src/ui/MapCanvas_MarkerHitTest_UI.cpp`.** Moves `HitTestManualMarkers` out verbatim,
unchanged (this ticket does not touch its logic — pure ceiling remediation, same "move, don't rewrite"
posture STEP124 used for `ReadMarkerGroupsJson`):
```cpp
// MapCanvas_MarkerHitTest_UI.cpp — HitTestManualMarkers, split out of MapCanvas_MarkerDrag_UI.cpp
// (STEP126) once that file's own line count — already 207, over ARCH §1.5's 150-line hard ceiling
// before this ticket — would have crossed further with this ticket's own tint-priority/highlight
// additions. Moved verbatim; no logic change. Declared in MapCanvas_MarkerDrag_UI.h, unchanged.
#include "MapCanvas_MarkerDrag_UI.h"
#include "PreviewComposite_UI.h"

namespace SanmapGen {
namespace Ui {

bool HitTestManualMarkers(const std::vector<Params::MarkerInstanceGroup>& markers,
                          const PreviewComposite& composite, const MapCanvasView& view,
                          float regionLocalX, float regionLocalY, float pickRadiusScreenPixels,
                          int& outGroupIndex, int& outTransformIndex) {
    // ... body identical to the current MapCanvas_MarkerDrag_UI.cpp:64-96, unchanged ...
}

} // namespace Ui
} // namespace SanmapGen
```

**New file — `src/ui/MapCanvas_MarkerRosterDraw_UI.cpp`.** Moves the anonymous-namespace draw helpers
(`kManualMarkerBaseDotRadiusScreenPixels`, `ManualMarkerDotRadius`, `ProjectWorldToScreen`,
`ManualMarkerTint`, `ManualSpawnArmyTint`) and `DrawManualMarkerRoster` out verbatim, THEN applies this
ticket's own tint-priority rewrite:
```cpp
// MapCanvas_MarkerRosterDraw_UI.cpp — DrawManualMarkerRoster and its own draw-time helpers, split out
// of MapCanvas_MarkerDrag_UI.cpp (STEP126) for the same ceiling reason as MapCanvas_MarkerHitTest_UI.cpp.
#include "MapCanvas_MarkerDrag_UI.h"
#include "MapCanvas_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/GlobalMarkerSettings_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

constexpr float kManualMarkerBaseDotRadiusScreenPixels = 6.0f;

float ManualMarkerDotRadius(...) { /* unchanged, moved verbatim */ }
ImVec2 ProjectWorldToScreen(...) { /* unchanged, moved verbatim */ }
ImU32 ManualMarkerTint(...) { /* unchanged, moved verbatim */ }
ImU32 ManualSpawnArmyTint(...) { /* unchanged, moved verbatim */ }

// STEP126 — true when `instanceIdentifier` is in this frame's computed highlight set
// (ComputeManualMarkerSelectionHighlight). Linear scan — the design doc's own "small per-frame
// vector" posture, authoring scale.
bool IsInstanceHighlighted(const std::vector<int>& selectedHighlightInstanceIdentifiers, int instanceIdentifier) {
    if (instanceIdentifier < 0) return false;
    for (int highlighted : selectedHighlightInstanceIdentifiers)
        if (highlighted == instanceIdentifier) return true;
    return false;
}

} // namespace

void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const Params::GlobalMarkerSettings& globalMarkerSettings,
                            const MarkerDragGestureState& dragState, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            const std::vector<int>& selectedHighlightInstanceIdentifiers,   // NEW — STEP126
                            ImDrawList& drawList) {
    if (composite.PixelsPerPreviewCell() <= 0.0f) return;
    const ImU32 refusedTint = IM_COL32(220, 60, 40, 255);
    const ImU32 ghostTint   = IM_COL32(200, 200, 200, 130);

    for (std::size_t groupIndex = 0; groupIndex < markers.size(); ++groupIndex) {
        const Params::MarkerInstanceGroup& group = markers[groupIndex];
        const bool bThisGroupDragging = dragState.bActive && dragState.groupIndex == static_cast<int>(groupIndex);
        for (std::size_t transformIndex = 0; transformIndex < group.transforms.size(); ++transformIndex) {
            if (bThisGroupDragging
                && IsMarkerSoftHiddenThisFrame(dragState, static_cast<int>(groupIndex), static_cast<int>(transformIndex)))
                continue;
            const Params::MarkerTransform& transform = group.transforms[transformIndex];
            const ImVec2 screenCenter = ProjectWorldToScreen(composite, view, transform.transform.positionX,
                                                             transform.transform.positionZ, regionOriginX, regionOriginY);
            ImU32 tint;
            // ARCH §19.18 — canonical priority, highest to lowest:
            if (bThisGroupDragging && dragState.bSpawnCardinalityRefused) {
                tint = refusedTint;
            } else if (IsInstanceHighlighted(selectedHighlightInstanceIdentifiers, transform.instanceIdentifier)) {
                // NEW — full fill replacement, opaque. ResolveMarkerGroupSelectTintColor's own
                // ratified signature returns RGB only (mirroring ResolveMarkerGroupTypeTintColor's 3-
                // out-param shape); this ticket's own call: alpha = 1.0f (fully opaque), the strongest,
                // most unambiguous "selected" signal — not layer.color[3] (that alpha belongs to the
                // UNSELECTED type/layer-color path) and not a selectColor*[3] alpha component
                // (the resolver never exposes it). Flagged as this ticket's own judgment call, not an
                // ARCH-specified value.
                float selectRed, selectGreen, selectBlue;
                Params::ResolveMarkerGroupSelectTintColor(group.name, globalMarkerSettings, selectRed, selectGreen, selectBlue);
                tint = ImGui::ColorConvertFloat4ToU32(ImVec4(selectRed, selectGreen, selectBlue, 1.0f));
            } else if (IsSpawnMarkerGroup(group)) {
                tint = ManualSpawnArmyTint(armies, transform.name,
                                           ManualMarkerTint(markerLayers, transform.layerIndex, group.name, globalMarkerSettings));
            } else {
                tint = ManualMarkerTint(markerLayers, transform.layerIndex, group.name, globalMarkerSettings);
            }
            drawList.AddCircleFilled(screenCenter,
                                     ManualMarkerDotRadius(markerLayers, transform.layerIndex, group.name, globalMarkerSettings),
                                     tint);
        }
        if (bThisGroupDragging) {
            // ... ghost-point block UNCHANGED, verbatim ...
        }
    }
    if (dragState.bActive && dragState.bSpawnCardinalityRefused)
        ImGui::SetTooltip("Spawn count is fixed - drag limited.");
}

} // namespace Ui
} // namespace SanmapGen
```
A marker being dragged that is ALSO the currently-selected one is handled by this same priority chain
with no special-casing: refused-red still wins if the drag is Spawn-cardinality-refused; otherwise the
select tint applies exactly as it would at rest — confirmed consistent by construction, not a gap.

**Kept file — `src/ui/MapCanvas_MarkerDrag_UI.cpp`.** Keeps ONLY the four `MapCanvas::` gesture-
lifecycle method definitions. `DrawManualMarkerDragPass` gains the new highlight computation call and
threads its result into `DrawManualMarkerRoster`:
```cpp
// MapCanvas_MarkerDrag_UI.cpp — MapCanvas's own gesture-lifecycle method definitions (declared in
// MapCanvas_UI.h). Split from MapCanvas_MarkerHitTest_UI.cpp/MapCanvas_MarkerRosterDraw_UI.cpp
// (STEP126, ceiling remediation) — this file now owns lifecycle only, those own hit-test/draw.
#include "MapCanvas_MarkerDrag_UI.h"
#include "MapCanvas_UI.h"
#include "MarkerSelectionHighlight_UI.h"   // NEW — STEP126
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

bool MapCanvas::TryBeginManualMarkerDrag(float regionLocalX, float regionLocalY) { /* unchanged */ }
void MapCanvas::ContinueManualMarkerDrag(float regionLocalX, float regionLocalY) { /* unchanged */ }
void MapCanvas::EndManualMarkerDrag() { /* unchanged */ }

void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    static const std::vector<Params::Army> kNoArmies;
    static const Params::GlobalMarkerSettings kDefaultGlobalMarkerSettings;
    static const Params::MarkerSymmetryFixSettings kDefaultMarkerSymmetryFixSettings;
    // STEP126 — recomputed fresh every frame (ARCH §19.19), discarded after this draw call. Null-safe:
    // no selection source wired -> -1 -> ComputeManualMarkerSelectionHighlight returns empty.
    const std::vector<int> selectedHighlight = (manualMarkerDragGeometry != nullptr)
        ? ComputeManualMarkerSelectionHighlight(*manualMarkerDragMarkers,
              manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers, *manualMarkerDragGeometry,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalSymmetryMask : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->radialSymmetryRepeatCount : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerSymmetryFixSettings.distanceTolerance
                                                 : kDefaultMarkerSymmetryFixSettings.distanceTolerance,
              manualMarkerSelectedInstanceIdentifier != nullptr ? *manualMarkerSelectedInstanceIdentifier : -1)
        : std::vector<int>{};
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalMarkerSettings : kDefaultGlobalMarkerSettings,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          selectedHighlight, *ImGui::GetWindowDrawList());
}

} // namespace Ui
} // namespace SanmapGen
```

**`MapCanvas_MarkerDrag_UI.h`** — `DrawManualMarkerRoster`'s declaration (lines 44-50) gains the same
new parameter, in the same position, with a doc-comment addition:
```cpp
// ... existing doc comment, plus:
// STEP126: `selectedHighlightInstanceIdentifiers` is this frame's ComputeManualMarkerSelectionHighlight
// result — every instanceIdentifier that should draw with the select tint (ARCH §19.18), highest
// priority after refused-drag-red. Empty = nothing selected, no highlight branch taken.
void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const Params::GlobalMarkerSettings& globalMarkerSettings,
                            const MarkerDragGestureState& dragState, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            const std::vector<int>& selectedHighlightInstanceIdentifiers,
                            ImDrawList& drawList);
```

No `CMakeLists.txt` edit needed for either new `.cpp` — auto-discovered by `SANGEN_V2_SOURCES`'s
`GLOB_RECURSE CONFIGURE_DEPENDS` (`CMakeLists.txt:174`, same as every prior ticket's new file).

### 11. Manual-only scope / no camera pan — confirmed, not re-derived

Per ARCH §19.20: nothing in this ticket references `Data::PlacementInstances` or
`OverlayInstanceKey_UI` — confirmed by construction, every new/touched function in this ticket takes
only `Params::MarkerInstanceGroup`/`MarkerTransform`/`MarkerInstanceLayer` types. Per Open Q4: no
`MapCanvasView`/camera call appears anywhere in the click handler (§5 above) — the Selectable click
writes only `selectedManualInstanceIdentifier`, nothing else.

## File-size ceiling — full disposition

Every row re-measured this session (`wc -l`), not trusted from any prior ticket's text, per the task's
own instruction.

| File | Before | This ticket's own delta | Disposition |
|---|---|---|---|
| `MapCanvas_MarkerDrag_UI.cpp` | 207 (already over) | split into 3 files | **Required split** (§10) — post-split, kept file ≈ 60-70 lines; new `MapCanvas_MarkerHitTest_UI.cpp` ≈ 35 lines; new `MapCanvas_MarkerRosterDraw_UI.cpp` ≈ 115-135 lines (Coder: confirm actual counts once formatted; if the roster-draw file still exceeds the hard ceiling, that is this ticket's own second exception to flag explicitly, Constitution §7, not to silently absorb — there is no further natural fault line inside `DrawManualMarkerRoster` itself, so a genuine overrun there would mean re-examining whether the tint-priority helper belongs in yet another file) |
| `MapCanvas_UI.h` | 196 (already over, unremediated) | +~8 lines (one method, one member) | Flagged, not required to remediate: a small addition to a file whose overrun predates this ticket by a wide margin and is unrelated to this ticket's own content (draw/pan/zoom/pick, not markers). Full remediation is a comparably-sized undertaking of its own — out of this ticket's scope, same carve-out STEP123 used for `TreeListWidget_UI`'s missing header-extra slot. Flagged for a follow-up ticket. |
| `MarkersTab_ManualInstance_UI.cpp` | 216 (already over, unremediated) | +~5 lines (one parameter threaded through 2 functions, one mint call, one include) | Flagged, not required: same reasoning — a ~5-line addition to a 216-line pre-existing overrun; full remediation out of scope, follow-up. |
| `MarkersTab_Manual_UI.h` | 184 (already over, unremediated) | +1 line (one new parameter on one declaration) | Flagged, not required: `NextMarkerInstanceIdentifier` was deliberately placed in its OWN new file (§1) specifically to avoid growing this file further; the one-line signature change is unavoidable (the real call site). Follow-up for full remediation. |
| `MarkersTab_ManualLayers_UI.h` | 165 (already over, unremediated) | +~5 lines (one include, two signature changes) | Flagged, not required: same reasoning. Follow-up. |
| `MarkersTab_Bundles_UI.cpp` | 138 (under, little headroom) | +~10 lines (index build + 2 threaded params on 2 functions) | Projected ≈ 146-148 lines — inside the hard ceiling but very close. **Coder: confirm the actual formatted count; if it crosses 150, this file has no further natural fault line left (it was already once split into `MarkersTab_BundleNodeBody_UI.cpp` by STEP120) — flag as this ticket's own second exception rather than force an awkward split.** |
| `MarkersTab_ManualLayerRowBody_UI.cpp` | 81 (under, real headroom) | +~18-20 lines (instance-list block + 2 signature params) | Projected ≈ 99-101 lines — crosses the 100-line SOFT ceiling, stays well under the 150 hard ceiling. Not required to split (ARCH §1.5 — only the hard ceiling forces a split); flagged for awareness only. |
| `MarkersTab_ManualLayers_UI.cpp` | 129 (under, real headroom) | +~10 lines | Projected ≈ 139. Comfortably under the hard ceiling; no flag needed. |
| `MarkerInstanceId_UI.h` | new | ~20 lines | Fine. |
| `ManualInstanceLayerIndex_UI.h` | new | ~28 lines | Fine. |
| `MarkerSelectionHighlight_UI.h` | new | ~20 lines | Fine. |
| `MarkerSelectionHighlight_UI.cpp` | new | ~45 lines | Fine. |
| `MapCanvas_MarkerHitTest_UI.cpp` | new | ~35 lines | Fine. |
| `MapCanvas_MarkerRosterDraw_UI.cpp` | new | see split row above | See above. |

**The three files flagged "not required" above (`MapCanvas_UI.h`, `MarkersTab_ManualInstance_UI.cpp`,
`MarkersTab_Manual_UI.h`, `MarkersTab_ManualLayers_UI.h`) are a real, pre-existing debt this ticket
does not fully discharge** — each is over the hard ceiling before this ticket touches it, for reasons
unrelated to this ticket's own scope. Fully remediating all four is a comparably large undertaking of
its own (each would need its own aspect-split analysis, its own new sibling file, its own header
updates) and is explicitly OUT OF SCOPE here — flagged per Constitution §7 rather than silently
absorbed, exactly as STEP123 flagged `TreeListWidget_UI`'s missing header-extra slot as a genuine,
sized follow-up rather than attempting it inline.

## Out of scope

- **Ticket B** (the Type-section tab restructure) — not drafted, not depended on; this ticket touches
  none of its files.
- **`ManualMarkersState::selectedGroupIndex`/`selectedInstanceIndex` coupling to the new instance-list
  click** — explicitly ruled out of scope by the design doc's own Open Q7 text ("cheap follow-up if
  wanted," not this round).
- **Delete/reorder from the new instance-list rows** — read/select-only; the roster editor
  (`MarkersTab_Manual_UI.h`) stays the sole owner of Add/Remove/reorder.
- **Procedural-instance selection of any kind** — ARCH §19.20, formally manual-only; no
  `Data::PlacementInstances`/`OverlayInstanceKey_UI` involvement anywhere in this ticket.
- **Camera pan/zoom on select** — ARCH-adjacent Open Q4, ruled no; not implemented.
- **Full remediation of the four already-over-ceiling files this ticket only lightly touches** (see
  File-size ceiling section) — flagged as follow-up work, not attempted here.
- **Any change to `Params::MarkerRule`/`Params::MarkerLayerBundle`/Bundle tree Move-Rotate/Type-section
  logic** — untouched; this ticket's only Bundle-file edit is threading the new index/selection
  parameters through `DrawMarkerGroupLeafBody`/`DrawMarkerLayerBundleTree`.
- **`selectColor*[3]`'s own alpha channel reaching the select-tint draw call** — this ticket's own call
  is opaque (alpha = 1.0f) since `ResolveMarkerGroupSelectTintColor`'s ratified signature returns RGB
  only (see §10's comment); flagged as a judgment call, not re-litigated here.

## Files touched

- `src/ui/MarkerInstanceId_UI.h` — **new**: `NextMarkerInstanceIdentifier`
- `src/ui/ManualInstanceLayerIndex_UI.h` — **new**: `ManualInstanceLayerIndex_UI`, `BuildManualInstanceLayerIndex`
- `src/ui/MarkerSelectionHighlight_UI.h` / `.cpp` — **new**: `ComputeManualMarkerSelectionHighlight`
- `src/ui/MapCanvas_MarkerHitTest_UI.cpp` — **new**: `HitTestManualMarkers`, moved verbatim (ceiling remediation)
- `src/ui/MapCanvas_MarkerRosterDraw_UI.cpp` — **new**: draw helpers + `DrawManualMarkerRoster`, moved + tint-priority rewrite
- `src/ui/MarkersTab_Manual_UI.h` — `DrawMarkerInstanceSection` gains a `markers` parameter
- `src/ui/MarkersTab_Manual_UI.cpp` — `DrawManualMarkers`'s call site threads `markers` through
- `src/ui/MarkersTab_ManualInstance_UI.cpp` — `DrawMarkerInstanceListButtons`/`DrawMarkerInstanceSection`
  gain the `markers` parameter; "Add Instance" mints `instanceIdentifier`
- `src/ui/MarkersTab_UI.h` — `MarkersTabState` gains `selectedManualInstanceIdentifier`
- `src/ui/MarkersTab_ManualLayers_UI.h` — new include; `DrawLayerRowBody`/`DrawManualMarkerLayers`
  signatures gain the index/selection parameters
- `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp` — `DrawLayerRowBody` gains the instance-list block
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `DrawLayerList`/`DrawManualMarkerLayers` build + thread the index/selection
- `src/ui/MarkersTab_Bundles_UI.cpp` — `DrawMarkerGroupLeafBody`/`DrawMarkerLayerBundleTree` build + thread the index/selection
- `src/ui/MarkersTab_UI.cpp` — `DrawMarkersTab`'s `DrawManualMarkerLayers` call site gains the new argument
- `src/ui/MapCanvas_UI.h` — new `SetManualMarkerSelectionSource` + `manualMarkerSelectedInstanceIdentifier` member
- `src/ui/MapCanvas_MarkerDrag_UI.h` — `DrawManualMarkerRoster` declaration gains the highlight-set parameter
- `src/ui/MapCanvas_MarkerDrag_UI.cpp` — kept file, now lifecycle-methods-only; `DrawManualMarkerDragPass`
  computes and threads the highlight set
- `src/ui/Application_UI.cpp` — `WireCallbacks` wires `SetManualMarkerSelectionSource`
- `src/ui/MarkerInstanceId_UI_Test.cpp` — **new**: mirrors `MarkerLayerId_UI_Test.cpp`
- `src/ui/MarkerSelectionHighlight_UI_Test.cpp` — **new**: pure-logic orbit-matching tests
- `src/ui/MarkersTab_ManualInstanceListRows_UI_Test.cpp` — **new**: headless-imgui click test
- `src/ui/MapCanvas_MarkerDrag_UI_Test.cpp` — extended: every existing `DrawManualMarkerRoster` call
  site gains the new trailing argument; new tint-priority-ordering tests
- `src/ui/MarkersTab_ManualLayers_UI_Test.cpp` — extended: `BuildManualInstanceLayerIndex` checks

## Verify

Acceptance bar: minting is globally unique and monotonic; an instance-list row click sets
`selectedManualInstanceIdentifier` and nothing else; the tint-priority chain resolves correctly in all
four orderings including refused-drag still beating selected; the sibling-orbit match produces the
correct sibling set, including the never-dragged/`symmetryGroupIdentifier == 0` case; procedural
markers are provably untouched; every split/moved file compiles and every existing suite this ticket
does not itself change stays green.

- **New — `MarkerInstanceId_UI_Test.cpp`** (mirrors `MarkerLayerId_UI_Test.cpp`'s exact shape):
  - Empty `markers` -> `NextMarkerInstanceIdentifier` returns 0.
  - A single group with transforms carrying `instanceIdentifier` 0, 3, 7 (unsorted) -> returns 8
    (max + 1), proving the scan is not order-dependent.
  - TWO groups, each with its own transforms, max values interleaved across groups (e.g. group A has
    2 and 9, group B has 5) -> returns 10 — proves the scan is GLOBAL across groups, not per-group
    (the exact property ARCH §19.16 requires and STEP124's own scope-correction flagged as this
    ticket's job).
  - Calling it twice in a row without applying the first result anywhere does NOT change its answer
    (pure function, no hidden state) — proves it must be called again, not cached, after each
    `push_back` (monotonic only because the CALLER re-scans, not because the function remembers).

- **New — `MarkerSelectionHighlight_UI_Test.cpp`** (pure logic, no imgui, mirrors
  `MarkerDragGesture_UI_Test.cpp`'s own fixture style — a small `Params::Geometry`, hand-built
  `MarkerInstanceGroup`/`MarkerTransform` fixtures):
  - `selectedInstanceIdentifier == -1` returns an empty vector.
  - A stale identifier (not present in `markers`) returns an empty vector — proves Constitution §6
    safety, never a crash/UB.
  - A selected transform under `SymmetryAxis::None` (mask 0) returns exactly one element: the selected
    instance's own identifier.
  - **The freshly-authored, never-dragged case** (ARCH §19.19's own explicit callout): a transform with
    `symmetryGroupIdentifier == 0` (the "Add Marker" default — never populated by anything but
    drag-materialize/Fix-Symmetry) placed under a layer with, e.g., `SymmetryAxis::MirrorAcrossX`, with
    a SECOND transform in the SAME group already sitting at the exact mirrored position (also
    `symmetryGroupIdentifier == 0`): asserts the highlight set contains BOTH instanceIdentifiers —
    proving position-driven orbit matching finds a geometric sibling an equality-on-
    `symmetryGroupIdentifier` approach would have missed entirely.
  - A sibling transform in a DIFFERENT `MarkerInstanceGroup` sitting at the exact mirrored position is
    NOT included — proves the same-group scoping rule.
  - A sibling within tolerance (`distance < distanceTolerance`) IS matched; a sibling just outside
    tolerance (`distance > distanceTolerance`) is NOT matched; a sibling exactly at the boundary
    (`distance == distanceTolerance`) IS matched (`<=`, mirroring `HitTestManualMarkers`' own boundary
    convention).
  - `orbitCount <= 1` (e.g. `SymmetryAxis::None`) returns only the selected instance, with zero calls
    that would matter if `BuildWorldSymmetryOrbit` were miswired — verified by checking the returned
    vector's size is exactly 1, not by mocking the PIPELINE call (this codebase has no mocking
    convention; a real `Params::Geometry` fixture is used, same as `MarkerDragGesture_UI_Test.cpp`).

- **New — `MarkersTab_ManualInstanceListRows_UI_Test.cpp`** (headless-imgui frame, mirrors
  `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`'s `HeadlessImguiSession`/`RunHeadlessFrame`
  harness from `ListWidget_TestFrame_UI.h`):
  - Build a `markerLayers` vector with one layer (`layerIndex 0`), a `markers` vector with one group
    holding two transforms both tagged `layerIndex = 0` (distinct `instanceIdentifier`s) and one
    transform tagged `layerIndex = 1` (a different layer). Build `BuildManualInstanceLayerIndex(markers)`.
    Call `DrawLayerRowBody` for layer 0 directly (no `DraggableList::Render` wrapper needed — a leaf
    imgui function, same posture STEP123's header-control test used). Assert exactly 2 Selectable rows
    render for layer 0 (the layer-1 transform is excluded) — proves the per-Layer filter.
  - Synthesize a click on the first row's known screen position; assert
    `selectedManualInstanceIdentifier` becomes that transform's own `instanceIdentifier`, and the
    function's own return value is `false` (no name/commit signal — a selection click never triggers
    `MakeNamesUnique`).
  - Click the SECOND row; assert `selectedManualInstanceIdentifier` updates to the second transform's
    identifier (not additive/toggled — a plain single-selection model).
  - A layer with zero matching instances in the index renders `"(none)"` and zero `Selectable` rows —
    checked via `ImGui::IsItemHovered`/item-count sweep, same technique
    `DraggableListWidget_UI_Test.cpp` already uses for its own row-sweep assertions.

- **Extend — `BuildManualInstanceLayerIndex` checks in `MarkersTab_ManualLayers_UI_Test.cpp`** (pure
  logic, no imgui, this file's own existing posture): three groups, transforms spread across
  `layerIndex` 0/1/2 including one group with TWO transforms on the same `layerIndex`; assert the
  built index's `instancesByLayerIndex[0]` contains exactly the expected `(groupIndex, transformIndex)`
  pairs, in encounter order; assert a `layerIndex` with zero transforms is simply absent from the map
  (not present with an empty vector) — the `find() == end()` case `DrawLayerRowBody`'s own "(none)"
  branch depends on.

- **Extend — `MapCanvas_MarkerDrag_UI_Test.cpp`:**
  - Every existing `DrawManualMarkerRoster(...)` call site (10 occurrences: `RunDrawAtRestAndSoftHideChecks`
    x2, `RunDrawRefusedTintChecks` x2, `RunSpawnArmyTintChecks` x4, `RunTypeDefaultColorChecks` x4,
    `RunManualMarkerDotRadiusScaleChecks` x3) gains a trailing empty `std::vector<int>{}` argument —
    every existing assertion must still pass byte-for-byte (proves the additive parameter is a true
    no-op when nothing is selected).
  - **New — `RunSelectedTintChecks`:** a single transform, `instanceIdentifier = 42`, non-Spawn group;
    call `DrawManualMarkerRoster` with `{42}` as the highlight set and a non-default
    `globalMarkerSettings.selectColorAlloy`/`selectColorDefault`; assert `LastVertexColor` matches the
    resolved select color at full opacity (255 alpha) — proves the branch fires and resolves through
    `ResolveMarkerGroupSelectTintColor` correctly for both a recognized group name and an unrecognized
    one (`selectColorDefault`).
  - **New — `RunTintPriorityOrderingChecks`,` the task's own explicit ask, all four orderings in one
    test:**
    1. Refused-drag + selected (same instance, `{identifier}` highlight set, `bSpawnCardinalityRefused
       = true`) -> asserts the color is the refused red, NOT the select color — refused wins.
    2. Selected + Spawn-army-color (a Spawn transform, name matching an army with a distinct real
       color, ALSO in the highlight set) -> asserts the color is the select color, NOT the army color
       — selected wins over army.
    3. Selected + layer/type-color (a non-Spawn transform with a non-default layer color override,
       ALSO in the highlight set) -> asserts the color is the select color, NOT the layer override —
       selected wins over layer/type.
    4. Not selected (empty highlight set), Spawn-army-color vs. layer/type-color -> reuses the EXISTING
       `RunSpawnArmyTintChecks`/`RunTypeDefaultColorChecks` results unchanged (already covers this
       ordering; no new assertion needed beyond confirming they still pass post-edit).
  - **New — a `bLocked`+selected check:** `layer.bLocked = true`, transform in the highlight set ->
    asserts the select tint still applies normally (ARCH §19.18's own explicit "no conflict, no
    ruling needed" — proves it by direct assertion, not left implicit).

- **Compile-standalone check for the split files**: `MapCanvas_MarkerHitTest_UI.cpp` and
  `MapCanvas_MarkerRosterDraw_UI.cpp` must each build as their own translation unit — exercised by a
  normal full build (auto-discovered, no separate test binary needed, same posture STEP124 used for
  its own new IO file).

- **Procedural-untouched cross-check (structural, not a runtime test)**: grep every new/touched file in
  this ticket for `PlacementInstances`/`OverlayInstanceKey_UI`/`Data::SpatialGrid` — zero matches
  expected in every one of them (this ticket's own files never include `PlacementInstances_DATA.h` or
  reference `Data::PlacementInstances` at all, confirmed by construction from the signatures above, not
  merely asserted).

- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `MarkerDragGesture_UI_Test.cpp`, `MarkerOrbitCorrespondence_UI_Test.cpp` (if it exists — not
  touched by this ticket), `MarkersTab_Bundles_UI_Test.cpp`, `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`,
  `MarkerLayerId_UI_Test.cpp`, `MarkersTab_Manual_UI_Test.cpp`, `MarkersTab_UI_Test.cpp`, and every
  IO/PARAMS suite from STEP124 (untouched by this ticket, PARAMS/IO layer not edited here).
