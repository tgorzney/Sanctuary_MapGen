# STEP254 — Split `src/ui/MarkersTab_UI.cpp` (499 lines) below the ARCH §1.5 hard ceiling

**Layer:** UI. **Domain:** `src/ui/MarkersTab_UI.cpp` and five new sibling files. **Sequence:**
depends on nothing undone. File-disjoint from STEP255 (`MapCanvas_UI.h`) — no shared files, safe to
run in parallel.

**Origin:** human-directed file-size-ceiling audit, 2026-09-08 — `MarkersTab_UI.cpp` and
`MapCanvas_UI.h` are the two current-build files furthest over `ARCH_01_05_FileSizeCeilings.md`
§1.5's **soft 100 / hard 150** line ceiling. This ticket is the remediation for the first file.
Confirmed by direct full read (not the audit's own characterization) at 499 lines today — ~3.3x the
hard ceiling.

## 0. Why a 2-file split isn't enough — read before objecting to scope

The obvious split (four "+Instance/+Group/+Layer/+Procedural Layer" button handlers into one file,
the three pending-delete appliers + the pending-create-layer-for-bundle applier into another) is
real and necessary, but leaves `MarkersTab_UI.cpp` at an estimated **~370 lines** — because the file
also carries the Type-section header's own button-cluster layout/draw code (~95 lines:
`kHeaderButtonSpacingPixels`, `SmallButtonWidth`, `TypeSectionHeaderButtonClusterWidth`,
`HeaderButtonsSectionOptions`, the `TypeSectionHeaderButtons_UI` struct,
`DrawRightAlignedTypeSectionHeaderButtons`) and `DrawBaseSectionManualInstanceList` (~67 lines) —
neither of which is business logic in the sense the two named tickets target, but both of which are
real weight that must leave the file too. Even after moving those, `DrawMarkersTab`'s own
per-Type-section loop BODY (header draw + 4 add-action calls + tree call + 4 pending-apply calls +
2 list-body calls + base-list call + notify + hide-toggle, ~90 lines) is itself too large to leave
inline — it must become its own named function, `DrawMarkerTypeSection`, in its own file, or the
line budget never closes. **This ticket is therefore a 5-new-file split, not 2** — the two named in
the audit prompt, plus three more this ticket's own arithmetic makes necessary. Every split below is
grounded in the same `ARCH_19_MarkerLayerBundle.md` §19.2 posture already ratified for this exact
class of problem (domain-touching logic gets its own named function per concern; pure mechanics —
here, "draw one Type-section's header buttons," "draw one Type-section's body," "draw the base
instance list" — get their own small file each, never lumped into one generic dispatcher).

## 1. Final shape — 1 file edited, 5 new file pairs

| File | Contents | Est. lines |
|---|---|---|
| `MarkersTab_UI.cpp` (EDIT) | includes, `ResolveAddInstanceLayerIndex`, `SelectedMarkerRule` (both UNCHANGED, declared in `MarkersTab_UI.h`, stay paired with it), `DrawMarkersTab` shrunk to Globals+Links+loop | ~90 |
| `MarkersTab_TypeSectionAddActions_UI.h`/`.cpp` (NEW) | the 4 add-button handlers + their 3 pure-mechanics helpers | ~155–165 |
| `MarkersTab_TypeSectionPendingApply_UI.h`/`.cpp` (NEW) | the 3 pending-delete appliers + the pending-create-layer-for-bundle applier | ~90–100 |
| `MarkersTab_TypeSectionHeaderButtons_UI.h`/`.cpp` (NEW) | the header's own button-cluster layout/draw | ~100 |
| `MarkersTab_BaseInstanceList_UI.h`/`.cpp` (NEW) | `DrawBaseSectionManualInstanceList` | ~75 |
| `MarkersTab_TypeSectionBody_UI.h`/`.cpp` (NEW) | `DrawMarkerTypeSection` — the per-Type-section body orchestrator | ~95–105 |

`MarkersTab_UI.h` is **UNCHANGED** — `DrawMarkersTab`'s public signature does not change, and none of
the five new files' functions are part of the tab's public API, so no external includer
(`Application_TabState_UI.h`, `Application_PanelEnvironment_UI.h`, `Application_AssetPanel_UI.cpp`,
every `MarkersTab_*_Test.cpp` that includes `MarkersTab_UI.h`) needs any change.

**If `MarkersTab_TypeSectionAddActions_UI.cpp` still lands over the 150-line hard ceiling** once the
preserved rationale comments are actually laid out (my own estimate above is close but not
guaranteed), split its 3 pure-mechanics helpers (`FindOrCreateMarkerInstanceGroupByName`,
`MapCenterWorldUnits`, `ResolveSelectedParentBundleIdentifier`) into a sixth file,
`MarkersTab_TypeSectionAddHelpers_UI.h`/`.cpp`, rather than trimming the STEP137/STEP152/STEP235
bug-report rationale comments to force a fit — re-measure, don't guess.

## 2. `MarkersTab_TypeSectionAddActions_UI.h`/`.cpp` — the 4 add-button handlers

Relocates, **verbatim including their rationale comments**, `MarkersTab_UI.cpp`'s current lines
120–137 (`FindOrCreateMarkerInstanceGroupByName`), 139–145 (`MapCenterWorldUnits`), 147–162
(`ResolveSelectedParentBundleIdentifier`) as file-local (anonymous-namespace) helpers, plus the
bodies currently inline at lines 312–339 (`bAddInstanceClicked`), 340–362 (`bAddGroupClicked`),
363–379 (`bAddManualLayerClicked`), and 387–402 (`bAddProceduralLayerClicked`, minus the
`bool bRecipeMoved = false;` line which stays in the caller) — each becomes its own named function,
**not one generic parameterized handler** (`ARCH_19_MarkerLayerBundle.md` §19.2 — domain-touching
logic gets its own per-concern function, confirmed applicable: each handler constructs a different
concrete `Params::` type and touches different fields).

```cpp
// MarkersTab_TypeSectionAddActions_UI.h
#pragma once
#include <string>
namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Ui {
struct MarkersTabState;

void ApplyAddInstanceButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                  const std::string& typeName);
void ApplyAddGroupButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                               const std::string& typeName);
void ApplyAddManualLayerButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                     const std::string& typeName);
// Returns true unconditionally when called — mirrors DrawMarkersTab's own former inline
// `bRecipeMoved = true` (a fresh Rule Layer, seeded with one default rule, is immediately
// pipeline-visible). Caller ORs this into its own accumulated bRecipeMoved.
bool ApplyAddProceduralLayerButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                         const std::string& typeName);
} // namespace Ui
} // namespace SanmapGen
```

`.cpp` includes `MarkersTab_UI.h` (full `MarkersTabState` definition — mirrors
`MarkersTab_Bundles_UI.h`'s own forward-declare-in-header/full-include-in-cpp pattern exactly),
`MarkerInstanceCreateSymmetric_UI.h`, `MarkerLayerId_UI.h`,
`MarkersTab_ManualInstanceSelection_UI.h` (`IsManualInstanceSelectionEntirelyType`,
`ReassignManualInstanceLayers`), `MarkersTab_ManualLayerHelpers_UI.h`
(`CanonicalMarkerTypeSectionName`), `../params/Geometry_PARAMS.h`, `../params/MapRecipe_PARAMS.h`.
Body content is the exact original code from the 4 line ranges above, unchanged logic, parameterized
on `recipe`/`state`/`typeName` instead of closing over the loop's locals.

## 3. `MarkersTab_TypeSectionPendingApply_UI.h`/`.cpp` — the 4 pending appliers

Relocates the current lines 415–468 (the `state.bundles.pendingDelete*`/`pendingCreateLayerFor*`
block), split into 4 **separately named** functions, confirmed NOT structurally identical (per this
ticket's own required verification): the bundle case is ID-equality-only, the manual-layer case is
positional-index-with-decrement, the procedural-layer case is positional-index-with-decrement PLUS
resets `selectedRuleIndex` and sets `bRecipeMoved`, and the create-layer case has no delete/cascade
branching at all — do not merge into one generic function.

```cpp
// MarkersTab_TypeSectionPendingApply_UI.h
#pragma once
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerRule_PARAMS.h"
namespace SanmapGen {
namespace Ui {
struct MarkerLayerBundlesState;

void ApplyPendingBundleDelete(MarkerLayerBundlesState& bundlesState,
                              std::vector<Params::MarkerLayerBundle>& bundles,
                              std::vector<Params::MarkerRuleLayer>& ruleLayers,
                              std::vector<Params::MarkerInstanceLayer>& markerLayers,
                              std::vector<Params::MarkerInstanceGroup>& markers);

void ApplyPendingManualLayerDelete(MarkerLayerBundlesState& bundlesState, int& selectedManualLayerIndex,
                                   std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers);

// Returns true only when a delete was actually pending and applied this call — caller ORs this into
// its own accumulated bRecipeMoved, mirroring the original inline block's exact behavior.
bool ApplyPendingProceduralLayerDelete(MarkerLayerBundlesState& bundlesState, int& selectedRuleLayerIndex,
                                       int& selectedRuleIndex,
                                       std::vector<Params::MarkerRuleLayer>& ruleLayers);

// Thin wrapper around the PRE-EXISTING MarkersTab_Bundles_UI.h::ApplyPendingCreateLayerForBundle
// (unchanged, NOT duplicated here) — this function only relocates the guard + call + 3-field reset
// that used to sit inline in DrawMarkersTab (lines 460–468), so it reads as the 4th named applier
// alongside the 3 above rather than a bare unnamed if-block.
void ApplyPendingCreateLayerForBundleIfRequested(MarkerLayerBundlesState& bundlesState,
                                                 std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                                 std::vector<Params::MarkerInstanceGroup>& markers);
} // namespace Ui
} // namespace SanmapGen
```

`.cpp` includes `MarkersTab_Bundles_UI.h` (`MarkerLayerBundlesState`'s full definition,
`ApplyPendingCreateLayerForBundle`) and `MarkersTab_BundleDelete_UI.h` (the 5 `Delete*` functions).
Each function's guard (`if (state.bundles.pendingDelete*Identifier >= 0) { ... }`) moves INSIDE the
function body — callers call all 4 unconditionally every frame, each no-ops when nothing is pending.

## 4. `MarkersTab_TypeSectionHeaderButtons_UI.h`/`.cpp` — the header button cluster

Relocates current lines 24–118 verbatim (constant, `SmallButtonWidth`, `TypeSectionHeaderButtonClusterWidth`,
`HeaderButtonsSectionOptions`, the `TypeSectionHeaderButtons_UI` struct, `DrawRightAlignedTypeSectionHeaderButtons`)
— pure UI layout, zero PARAMS mutation, a legitimate single concern ("this Type-section header's own
button row"). Header includes `Section_UI.h` (for `SectionOptions`, the return type) and
forward-declares `MarkersTabGlobals`, `IconAtlasManifest`, `IconAtlasPairingLookup`,
`Params::GlobalMarkerSettings`; `.cpp` includes `MarkersTab_Globals_UI.h` for the real field access.

## 5. `MarkersTab_BaseInstanceList_UI.h`/`.cpp` — the base/unassigned instance list

Relocates current lines 164–230 verbatim (`DrawBaseSectionManualInstanceList` and its doc comment) —
mirrors the sibling body-drawer pattern already established for the Rule/Manual layer lists
(`MarkersTab_RuleLayers_UI.h`/`.cpp`, `MarkersTab_ManualLayerRowBody_UI.h`/`.cpp`). `.cpp` includes
`MarkersTab_ManualInstanceSelection_UI.h` (`DrawManualLayerInstanceDropTarget`),
`MarkersTab_ManualLayerHelpers_UI.h` (`CanonicalMarkerTypeSectionName`),
`SymmetryClusterInstanceList_UI.h`, `../params/MarkerInstance_PARAMS.h`, `imgui.h`.

## 6. `MarkersTab_TypeSectionBody_UI.h`/`.cpp` — the per-Type-section body orchestrator

New function, `DrawMarkerTypeSection`, extracted from `DrawMarkersTab`'s own per-Type-section loop
body (current lines ~296–494, everything from `ImGui::PushID(typeName)` through the matching
`ImGui::PopID()`), now calling into the 4 files above plus the pre-existing
`DrawMarkerLayerBundleTree`, `DrawRuleLayerListBody`, `DrawManualMarkerLayerListBody`,
`ApplyAddLinkAction`, `NotifyPlacementChange` (all UNCHANGED call sites, just relocated):

```cpp
// MarkersTab_TypeSectionBody_UI.h
#pragma once
#include <functional>
#include <string>
#include <vector>
namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {
struct MarkersTabState;
struct IconAtlasManifest;
struct IconAtlasPairingLookup;

// One Type-section's full collapsible body — header buttons, the Group/Layer/Instance tree, the
// pending-apply mutations the tree's own header-extra "X"/drop targets recorded this frame, the
// flat Rule/Manual-layer list bodies, and the base "no Layer" instance list. Extracted out of
// DrawMarkersTab's own per-Type-section loop body (STEP254) so DrawMarkersTab itself stays a thin
// Globals+Links+loop wrapper. `rowIndex`/`typeName` are the SAME loop variables the body used to
// close over inline.
void DrawMarkerTypeSection(Params::MapRecipe& recipe, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                           const IconAtlasPairingLookup* pairingLookup,
                           const Data::PlacementInstances* placedMarkers, int rowIndex, const char* typeName,
                           const std::function<void(int clickedInstanceIdentifier,
                                                    const std::vector<int>& selectedInstanceIdentifiers)>&
                               selectManualMarkerInstanceCallback,
                           const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                               selectProceduralMarkerInstanceCallback);
} // namespace Ui
} // namespace SanmapGen
```

`.cpp` includes: `MarkersTab_UI.h`, `MarkersTab_TypeSectionAddActions_UI.h`,
`MarkersTab_TypeSectionPendingApply_UI.h`, `MarkersTab_TypeSectionHeaderButtons_UI.h`,
`MarkersTab_BaseInstanceList_UI.h`, `MarkersTab_Links_UI.h` (`ApplyAddLinkAction`),
`MarkersTab_ManualLayerRowBody_UI.h` (`DrawManualMarkerLayerListBody`), `PlacementRuleSections_UI.h`
(`NotifyPlacementChange`), `imgui.h`, `../params/MapRecipe_PARAMS.h`, `../pipeline/PreviewDriver_PIPELINE.h`.

Body, in order (byte-identical logic to today, only the call targets change for the 8 relocated
blocks): `PushID(typeName)` → `bHidden` → `DrawSectionBegin` → `DrawRightAlignedTypeSectionHeaderButtons`
→ 3 `if (buttons.bAdd*Clicked) Apply*ButtonAction(...)` calls → the unchanged `bAddLinkClicked`/
`ApplyAddLinkAction` call → `bool bRecipeMoved = false;` → `if (buttons.bAddProceduralLayerClicked)
bRecipeMoved = ApplyAddProceduralLayerButtonAction(...);` → `DrawMarkerLayerBundleTree` (unchanged) →
the 4 `ApplyPending*`/`ApplyPendingCreateLayerForBundleIfRequested` calls (ORing into `bRecipeMoved`
where applicable) → `Separator` → `DrawRuleLayerListBody` (ORing into `bRecipeMoved`, unchanged) →
`DrawManualMarkerLayerListBody` (unchanged) → `Separator` → `DrawBaseSectionManualInstanceList` →
`NotifyPlacementChange(bRecipeMoved, previewDriver)` → hide-toggle → `DrawSectionEnd()` → `PopID()`.

## 7. `MarkersTab_UI.cpp` — final shape

```cpp
void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                    const IconAtlasPairingLookup* pairingLookup,
                    const Data::PlacementInstances* placedMarkers,
                    const std::function<void(int, const std::vector<int>&)>& selectManualMarkerInstanceCallback,
                    const std::function<void(int, bool, bool)>& selectProceduralMarkerInstanceCallback) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals);
    DrawMarkerLinksSection(recipe, state.links, state.selectedManualInstanceIdentifier,
                          state.selectedManualInstanceIdentifiers,
                          state.manualInstanceSelectionAnchorIdentifier,
                          selectManualMarkerInstanceCallback, previewDriver);
    for (int rowIndex = 0; rowIndex < kMarkerGlobalScaleRowCount; ++rowIndex) {
        DrawMarkerTypeSection(recipe, state, previewDriver, iconManifest, pairingLookup, placedMarkers,
                             rowIndex, markerGlobalScaleRowLabels[rowIndex],
                             selectManualMarkerInstanceCallback, selectProceduralMarkerInstanceCallback);
    }
    ImGui::PopID();
}
```

`ResolveAddInstanceLayerIndex` and `SelectedMarkerRule` (current lines 234–270) are **UNCHANGED and
stay in this file** — they are declared in `MarkersTab_UI.h` (the same-named header, the established
pairing convention) and are directly tested by `MarkersTab_UI_Test.cpp`,
`MarkersTab_RuleLayers_UI_Test.cpp`, `ParameterTabs_DirtyTier_UI_Test.cpp`,
`ParameterTabs_Rules_UI_Test.cpp` via that header's declaration — neither is part of the business
logic this ticket targets, and relocating them buys nothing (they're already small, ~36 lines
combined) at the cost of an unusual declared-in-X/defined-in-Y split with no precedent need.

## 8. Standing recorded defect — explicitly NOT this ticket's job

`work_orders/EXECUTION_CONFLICT_MAP.md` §3 flags `MarkersTab_UI.cpp:295-296`'s hardcoded 3-entry
Type-section loop (should be the ratified dynamic `DrawMarkerTypeSection` enumeration over live
`markerTypeName` values, `ARCH_19_13`/§19.14) as a real, already-recorded, unticketed defect. **Do
not fix it here** — this ticket is a pure structural split; the loop's own iteration bound
(`kMarkerGlobalScaleRowCount`) is relocated unchanged, defect and all. Leave it exactly as
hardcoded, flagged for its own future ticket.

## 9. Out of scope

- Any behavior change of any kind — every relocated function's logic is byte-identical.
- Fixing the hardcoded 3-entry Type-section loop (§8 above).
- Touching `MarkersTab_UI.h`, `MarkersTab_Bundles_UI.h`/`.cpp`,
  `MarkersTab_BundleDelete_UI.h`/`.cpp`, `MarkersTab_Links_UI.h`, or any other sibling file's own
  declarations — every existing function this ticket calls into is reused as-is.
- Any test file edit — confirmed by direct grep that no test includes any of the relocated
  anonymous-namespace/file-local helpers by name; every test that touches this tab does so through
  the unchanged public `DrawMarkersTab`, `ResolveAddInstanceLayerIndex`, or `SelectedMarkerRule`
  entry points (`MarkersTab_UI_Test.cpp`'s `RunAddProceduralLayerHeaderButtonClickThroughChecks`
  specifically drives the "+Layer → Procedural" click all the way through the real `DrawMarkersTab`
  — this is real regression coverage for `ApplyAddProceduralLayerButtonAction`'s relocation, not a
  file this ticket edits).

## 10. Files touched

- EDIT `src/ui/MarkersTab_UI.cpp`
- NEW `src/ui/MarkersTab_TypeSectionAddActions_UI.h` / `.cpp`
- NEW `src/ui/MarkersTab_TypeSectionPendingApply_UI.h` / `.cpp`
- NEW `src/ui/MarkersTab_TypeSectionHeaderButtons_UI.h` / `.cpp`
- NEW `src/ui/MarkersTab_BaseInstanceList_UI.h` / `.cpp`
- NEW `src/ui/MarkersTab_TypeSectionBody_UI.h` / `.cpp`
- (possible 6th pair, only if needed after real measurement — see §1's note)

## 11. Verify

- Full solo rebuild, clean.
- Full `ctest` pass, 100%, with **zero test file edits** — `MarkersTab_UI_Test.cpp`,
  `MarkersTab_RuleLayers_UI_Test.cpp`, `MarkersTab_Bundles_UI_Test.cpp`,
  `MarkersTab_ManualInstanceSelection_UI_Test.cpp`, `ParameterTabs_DirtyTier_UI_Test.cpp`,
  `ParameterTabs_Rules_UI_Test.cpp`, `ApplicationShell_IconBridge_UI_Test.cpp` all pass unchanged —
  this IS the acceptance test for "zero behavior change."
- Re-measure every touched/new file against `ARCH_01_05_FileSizeCeilings.md` §1.5 after implementing;
  if `MarkersTab_TypeSectionAddActions_UI.cpp` (the one file this ticket's own estimate is least
  certain about) lands over 150, apply §1's fallback split rather than trimming rationale comments.
