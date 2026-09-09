# STEP256 — Reachable "FIX SYM" header button for STEP107's orphaned per-layer symmetry-fix command

**Layer:** UI. **Domain:** `src/ui/MarkersTab_ManualLayerRowBody_UI.h/.cpp` (new header-cluster
control), `src/ui/MarkersTab_ManualLayers_UI.h/.cpp` (flat-list cluster wiring),
`src/ui/MarkersTab_Bundles_UI.h`/`MarkersTab_BundleHeaderExtras_UI.cpp`/`MarkersTab_Bundles_UI.cpp`
(Bundle-tree leaf wiring), `src/ui/MarkersTab_Globals_UI.h/.cpp` (relocated tolerance/overwrite/result
controls), `src/ui/MarkersTab_UI.cpp` (one call-site update). **Deleted:**
`src/ui/MarkerLayerSymmetrySection_UI.h/.cpp`. **Executor:** SanGen Coder. Pure UI-wiring fix — no
algorithm/PARAMS change. Do not touch `Ui::FixMarkerLayerSymmetry`
(`MarkerSymmetryFixCommand_UI.h/.cpp`), `Pipeline::FindMarkerSymmetryMatches`
(`MarkerSymmetryDetection_PIPELINE.h/.cpp`), or either's own test binary
(`MarkerSymmetryFixCommand_UI_Test.cpp`, `MarkerSymmetryDetection_PIPELINE_Test.cpp`) — all four are
correct and already ratified/tested (STEP107).

## The bug

STEP107 built a complete, tested "Fix Symmetry" per-layer backfill command
(`Ui::FixMarkerLayerSymmetry`) and its own UI trigger (`DrawFixSymmetryCommand`/
`DrawLayerSymmetrySection`, `src/ui/MarkerLayerSymmetrySection_UI.h/.cpp`) — but the trigger is dead
code. Grep-confirmed: `DrawLayerSymmetrySection` has exactly one declaration + one definition and
**zero callers** anywhere in the live UI tree.

The actual live per-row body, `DrawLayerRowBody` (`src/ui/MarkersTab_ManualLayerRowBody_UI.cpp:89-154`),
receives `markerLayers`/`geometry`/`globalSymmetryMask`/`globalRadialRepeatCount`/
`markerSymmetryFixSettings` and immediately discards every one of them via `(void)` casts (lines
99-100) — its own comment (101-108) documents that STEP142, per direct human instruction at the time,
deliberately stripped the "Layer Symmetry" section (tolerance slider, overwrite checkbox, Fix Symmetry
button, result text) out of the per-row body and moved Icon Size/Grid Snap into the always-visible
header cluster instead, but nothing ever re-added a Fix Symmetry trigger anywhere reachable. `[SYM]`
today (`DrawMarkerLayerSymmetryToggleHeaderControl`, `MarkersTab_ManualLayerRowBody_UI.cpp:226-237`) is
only an enable/disable toggle for `MarkerInstanceLayer::bSymmetryEnabled` — it never runs detection or
writes `symmetryGroupIdentifier`. Net effect: a marker imported from a `.sanmap` or hand-placed one at
a time has **no way, anywhere in the current UI, to backfill its symmetry group** — the feature this
codebase spent a whole ticket building is completely unreachable.

## Ruling: a "FIX SYM" header button, immediately right of `[SYM]`

Per direct instruction this session: add a **"FIX SYM"** `SmallButton`, drawn in the row header's
always-visible button cluster (not gated on row-expand state), immediately to the RIGHT of the
existing `[SYM]` toggle, at every call site `[SYM]` already draws (the flat/ungrouped Manual layer
list AND the Bundle tree's Manual leaf). Clicking it runs `Ui::FixMarkerLayerSymmetry` for **that row's
own layer only** — the same per-layer scope STEP107 §1 already established (not per-selection; the
human confirmed per-layer, reachable from the header, is correct).

### Design decision 1 — where does it fit in the header's fixed width budget

New constant in `src/ui/MarkersTab_ManualLayerRowBody_UI.h`, added immediately after the existing
`kMarkerLayerSymmetryButtonWidthPixels` (`:47`), following this file's own "eyeballed against a live
frame" convention for every other button-width constant here (SYM/COL = 34px for 3-char labels, GRID =
40px for 4 chars — "FIX SYM" is 7 chars, so 58px is the linear-extrapolation starting estimate; the
coder must verify/adjust against a live frame the same way every other constant in this file was
derived, not treat 58.0f as exact):

```cpp
inline constexpr float kMarkerLayerSymmetryButtonWidthPixels      = 34.0f;
inline constexpr float kMarkerLayerFixSymmetryButtonWidthPixels   = 58.0f;   // NEW — STEP256
inline constexpr float kMarkerLayerColorOverrideButtonWidthPixels = 34.0f;
```

Fold it into `kMarkerLayerHeaderExtraCombinedWidthPixels` (`:68-72`) — this is the ONE shared
reserved-zone width every Layer-kind header row gets (flat Manual, Bundle-tree Manual leaf, Bundle-tree
Procedural leaf all read the same constant, per `MarkersTab_Bundles_UI.h:247`'s own comment), so
growing it here correctly widens the reserved zone everywhere by construction — no other constant or
call site needs independent editing:

```cpp
inline constexpr float kMarkerLayerHeaderExtraCombinedWidthPixels =
    kMarkerLayerIconSizeControlWidthPixels + kMarkerLayerGridSizeControlWidthPixels
    + kMarkerLayerSymmetryButtonWidthPixels + kMarkerLayerFixSymmetryButtonWidthPixels   // NEW term
    + kMarkerLayerColorOverrideButtonWidthPixels + kMarkerLayerColorOverrideSwatchWidthPixels
    + kMarkerLayerVisibilityButtonWidthPixels + kMarkerLayerHeaderExtraDeleteButtonWidthPixels;
```

**Do not touch `DrawRightAlignedProceduralLayerCluster`'s own `clusterWidth` math**
(`MarkersTab_BundleHeaderExtras_UI.cpp:88-91`) — a Procedural leaf draws no FIX SYM button (it has no
`symmetryGroupIdentifier` concept), so it simply gets a slightly larger unused margin within the same
shared reserved zone, exactly the pre-existing "narrower cluster, some unused margin" relationship
STEP125 already documented (`MarkersTab_Bundles_UI.h:255-259`'s own comment) — not a regression, and
not a STEP206-class overlap bug: `DrawRightAlignedProceduralLayerCluster`'s own push is still
`reservedZoneWidthPixels - clusterWidth` where `clusterWidth` is unchanged and `reservedZoneWidthPixels`
only grew, so its right edge only moves further LEFT of the strip, never past it.

**Width-accounting rule — do not repeat the STEP206 bug.** In the flat list's
`DrawRightAlignedSymmetryColorOverrideCluster` (`MarkersTab_ManualLayers_UI.cpp:92-127`), inserting a
control between `[SYM]` and `[COL]` adds ONE new external `SameLine()` gap (SYM->FIXSYM, replacing the
old single SYM->COL gap with two: SYM->FIXSYM and FIXSYM->COL) — the live-`ItemSpacing.x`-based
`clusterWidth` STEP206 already fixed must go from `5.0f * itemSpacing` to `6.0f * itemSpacing`, plus add
`kMarkerLayerFixSymmetryButtonWidthPixels` to the width sum:

```cpp
const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
const float clusterWidth = kMarkerLayerIconSizeControlWidthPixels
                          + kMarkerLayerGridSizeControlWidthPixels
                          + kMarkerLayerSymmetryButtonWidthPixels
                          + kMarkerLayerFixSymmetryButtonWidthPixels   // NEW
                          + kMarkerLayerColorOverrideButtonWidthPixels
                          + kMarkerLayerColorOverrideSwatchWidthPixels
                          + 6.0f * itemSpacing;   // was 5.0f — one new external gap (STEP206's own rule)
```

The Bundle tree's own `DrawMarkerGroupLeafHeaderExtra` Manual-leaf branch
(`MarkersTab_BundleHeaderExtras_UI.cpp:261-309`) draws its controls with plain sequential
`ImGui::SameLine()` and no pre-computed width-budget subtraction at all (STEP206 verified this branch
is a different, self-correcting shape — its trailing "X" right-aligns off the LIVE
`GetContentRegionAvail()` via `DrawRightAlignedDeleteButton`) — inserting one more `SmallButton()` +
`SameLine()` there needs no width-math change, only the same insertion-order change as the flat list.

### Design decision 2 — tolerance slider + overwrite checkbox: relocated to the tab's Global section, not dropped

The header strip has no room for a slider + checkbox + result-text combo. **Ruling: relocate them to
`DrawMarkersTabGlobals` (`src/ui/MarkersTab_Globals_UI.h/.cpp`)**, not a popup on the button. Reasoning:
STEP107 §5 already established `markerSymmetryFixSettings.distanceTolerance` is a **recipe-level**
value, not per-layer, and STEP107 §2's own "New known accepted limitation" already documented that
`bFixSymmetryOverwrite`/`bHasFixSymmetryResult`/`lastFixSymmetryResult` are **ONE shared
`ManualMarkerLayersState` instance for the whole tab** (`MarkersTabState::manualLayers`,
`MarkersTab_UI.h:108` — confirmed a single field, not per-Type-section, not per-row). The Global section
is therefore the first place these fields draw at their own REAL scope, not a narrower one. A popup
would work but adds a second interaction pattern (click-and-hold/right-click) nothing else in this tab
uses; the Global section is zero-new-mechanism and already the tab's home for exactly this kind of
recipe-wide, non-per-layer setting.

**The header button still consults and consumes the relocated checkbox** — it is not inert. Clicking
"FIX SYM" reads `state.bFixSymmetryOverwrite` (wherever the user last set it in the Global section) to
select skip vs. overwrite mode, runs the command, and then resets `state.bFixSymmetryOverwrite = false`
— the EXACT "consumed per-use, not sticky" behavior STEP107 §2 already specified, just now driven by a
control drawn in a different place. Relocating the checkbox without still consulting it would leave the
overwrite pathway editable-but-dead, exactly the kind of orphaned-control bug this ticket exists to fix
— not acceptable.

New declaration, `src/ui/MarkersTab_Globals_UI.h` (near the existing `DrawMarkersTabGlobals`
declaration, `:187`):

```cpp
// STEP256 — the "Fix Symmetry" command's own recipe-level tolerance + overwrite-mode controls,
// relocated here now that the per-layer "FIX SYM" header button (MarkersTab_ManualLayerRowBody_UI.h)
// replaces the retired per-row "Layer Symmetry" section these used to live in (STEP107 §2/§5):
// `markerSymmetryFixSettings.distanceTolerance` is recipe-level and `manualLayersState.
// bFixSymmetryOverwrite`/`bHasFixSymmetryResult`/`lastFixSymmetryResult` were ALREADY one shared
// tab-wide instance (MarkersTabState::manualLayers) — this is the first place they draw at their own
// real scope. Declared here (not file-local) so a headless test can drive it directly.
void DrawMarkerSymmetryFixSettings(Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                   ManualMarkerLayersState& manualLayersState);
```

`MarkersTab_Globals_UI.h` needs `#include "Checkbox_UI.h"` (not currently included) and
`#include "../params/Symmetry_PARAMS.h"` (not currently included — `MarkerSymmetryFixSettings` lives
there) added; forward-declare `struct ManualMarkerLayersState;` (mirroring
`MarkersTab_Bundles_UI.h:37`'s own forward-declare of the same type) since only a reference appears in
the declaration. `DrawMarkersTabGlobals`'s own declaration (`:187`) widens:

```cpp
void DrawMarkersTabGlobals(MarkersTabGlobals& globals,
                           Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                           ManualMarkerLayersState& manualLayersState);
```

`src/ui/MarkersTab_Globals_UI.cpp` — add `#include "MarkersTab_ManualLayers_UI.h"` (the real
`ManualMarkerLayersState` definition, needed here since the .cpp actually touches its fields; verified
no circular include — `MarkersTab_ManualLayers_UI.h` does not include `MarkersTab_Globals_UI.h`).
New function body:

```cpp
void DrawMarkerSymmetryFixSettings(Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                   ManualMarkerLayersState& manualLayersState) {
    ImGui::Separator();
    ImGui::TextUnformatted("Fix Symmetry (per-layer \"FIX SYM\" header button)");
    DrawSliderScalar("Fix Symmetry Distance Tolerance", markerSymmetryFixSettings.distanceTolerance,
                     manualLayersState.fixSymmetryToleranceRange, manualLayersState.fixSymmetryToleranceToggle,
                     WidgetStyle(), "%.2f");
    DrawCheckbox("Overwrite manually-adjusted positions", manualLayersState.bFixSymmetryOverwrite);
    if (manualLayersState.bHasFixSymmetryResult) {
        ImGui::Text("Fix Symmetry: %d group(s) created, %d slot(s) unmatched",
                   manualLayersState.lastFixSymmetryResult.confirmedGroupCount,
                   manualLayersState.lastFixSymmetryResult.unmatchedSlotCount);
    }
}

void DrawMarkersTabGlobals(MarkersTabGlobals& globals, Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                           ManualMarkerLayersState& manualLayersState) {
    if (!DrawSectionBegin("Global", globals.section)) return;
    DrawGamedataSource(globals);
    DrawMarkerSymmetryFixSettings(markerSymmetryFixSettings, manualLayersState);
    DrawSectionEnd();
}
```

Call site, `src/ui/MarkersTab_UI.cpp:72` — only production call site (grep-confirmed, no test calls
`DrawMarkersTabGlobals` directly):

```cpp
DrawMarkersTabGlobals(state.globals, recipe.markerSymmetryFixSettings, state.manualLayers);
```

### Design decision 3 — `MarkerLayerSymmetrySection_UI.h/.cpp`: delete, do not repurpose

Grep-confirmed zero remaining references anywhere except the two source files themselves and one
historical design-doc mention (`work_orders/DESIGN_MarkersUICorrectionRound2_R1.md`). Its shape (a full
`DrawSectionBegin("Layer Symmetry", ...)` wrap, the axis-picker, a full slider+checkbox+button+result
combo) does not fit a compact header button — only its one-line call into `FixMarkerLayerSymmetry` is
reusable, not worth keeping a whole file alive for. **Ruling: delete both files outright.** Repurposing
them would leave a second, still-effectively-dead code shell around (the exact anti-pattern this ticket
exists to close), not a real reuse.

### Design decision 4 — result feedback: the relocated Global section's text line, not a tooltip

Decision 2 already answers this: `state.bHasFixSymmetryResult`/`lastFixSymmetryResult` keep their
EXACT STEP107 semantics (written by the command, read by a `%d group(s) created, %d slot(s) unmatched`
line) — they simply now draw in `DrawMarkerSymmetryFixSettings` (Global section) instead of the retired
per-row section. The header button itself gets ONLY a static informational tooltip (no result text — a
`SmallButton`'s hover state does not reliably persist long enough after a click to make a
result-in-tooltip worth the complexity), pointing the user at where to read the outcome and where to
edit tolerance/overwrite:

```cpp
if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Fix Symmetry (tolerance %.2f%s) - edit tolerance/overwrite mode and see the "
                      "last result in the tab's Global section",
                      markerSymmetryFixSettings.distanceTolerance,
                      state.bFixSymmetryOverwrite ? ", OVERWRITE mode armed" : "");
```

## The new control itself

New function, `src/ui/MarkersTab_ManualLayerRowBody_UI.h` (declared alongside
`DrawMarkerLayerSymmetryToggleHeaderControl`, `:131-132`, same file every other header-cluster control
lives in — `DrawManualMarkerLayerColorOverrideHeaderControl`, `DrawMarkerLayerSymmetryToggleHeaderControl`,
`DrawMarkerLayerIconSizeHeaderControl`, `DrawMarkerLayerGridSnapHeaderControl` all already live here, so
this is co-location with precedent, not a new pattern):

```cpp
// STEP256 — the row header's own "FIX SYM" command button, immediately right of [SYM]. Runs
// Ui::FixMarkerLayerSymmetry for THIS row's own layer only (STEP107 §1's per-layer scope), using the
// recipe-level `markerSymmetryFixSettings.distanceTolerance` (read-only here — edited in the tab's
// Global section, MarkersTab_Globals_UI.h) and `state.bFixSymmetryOverwrite` (read AND consumed —
// reset to false after every run, STEP107 §2's own "not sticky" rule, unchanged). `markerLayers` is
// the full vector (needed only for ResolveEffectiveMarkerSymmetry's own lookup — the button's OWN
// layer is identified purely by `layerIndex`, mirroring DrawLayerRowBody's own established shape).
void DrawMarkerLayerFixSymmetryHeaderControl(int layerIndex,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
    int globalSymmetryMask, int globalRadialRepeatCount,
    const Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state);
```

Add `#include "MarkerSymmetryFixCommand_UI.h"` to `MarkersTab_ManualLayerRowBody_UI.cpp` (for
`FixMarkerLayerSymmetry`/`MarkerSymmetryFixResult` — currently reached only transitively via
`MarkersTab_ManualLayers_UI.h`, which this file already includes; direct include matches this
codebase's own convention of including what you use). `ResolveEffectiveMarkerSymmetry` is already
reachable (`MarkersTab_ManualLayerHelpers_UI.h`, already included at `:7`). Definition, placed right
after `DrawMarkerLayerSymmetryToggleHeaderControl` (`MarkersTab_ManualLayerRowBody_UI.cpp:226-237`) —
same shape/call sequence `MarkerLayerSymmetrySection_UI.cpp`'s now-deleted `DrawFixSymmetryCommand`
already proved correct (STEP246's synthetic layer-index-only-transform + empty-Links resolution carve-out):

```cpp
void DrawMarkerLayerFixSymmetryHeaderControl(int layerIndex,
        const std::vector<Params::MarkerInstanceLayer>& markerLayers,
        std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
        int globalSymmetryMask, int globalRadialRepeatCount,
        const Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state) {
    if (ImGui::SmallButton("FIX SYM##fixSymmetry")) {
        Params::MarkerTransform layerIndexOnlyTransform;
        layerIndexOnlyTransform.layerIndex = layerIndex;
        static const std::vector<Params::MarkerLink> kNoLinks;
        int effectiveMask = 0;
        int effectiveRadialRepeatCount = 0;
        ResolveEffectiveMarkerSymmetry(markerLayers, layerIndexOnlyTransform, kNoLinks, globalSymmetryMask,
                                       globalRadialRepeatCount, effectiveMask, effectiveRadialRepeatCount);
        state.lastFixSymmetryResult = FixMarkerLayerSymmetry(markers, geometry, layerIndex, effectiveMask,
            effectiveRadialRepeatCount, markerSymmetryFixSettings.distanceTolerance, state.bFixSymmetryOverwrite);
        state.bHasFixSymmetryResult = true;
        state.bFixSymmetryOverwrite = false;   // consumed per-use — STEP107 §2's rule, unchanged
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Fix Symmetry (tolerance %.2f%s) - edit tolerance/overwrite mode and see the "
                          "last result in the tab's Global section",
                          markerSymmetryFixSettings.distanceTolerance,
                          state.bFixSymmetryOverwrite ? ", OVERWRITE mode armed" : "");
}
```

## Wiring — flat/ungrouped Manual layer list

`DrawRightAlignedSymmetryColorOverrideCluster` (declared `MarkersTab_ManualLayers_UI.h:120-122`,
defined `MarkersTab_ManualLayers_UI.cpp:92-127`) widens to accept the layer's own index plus the
command's five required inputs (inserted after `layer`, before `state`, mirroring
`DrawLayerRowBody`'s own established parameter order):

```cpp
void DrawRightAlignedSymmetryColorOverrideCluster(Params::MarkerInstanceLayer& layer, int layerIndex,
                                                  const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                                  const Params::Geometry& geometry, int globalSymmetryMask,
                                                  int globalRadialRepeatCount,
                                                  const Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                                  ManualMarkerLayersState& state, bool& bAnyCommitted,
                                                  const std::vector<Params::MarkerLink>& links = {});
```

Body — insert the new control between the existing SYM and COL calls (`:124-126`):

```cpp
DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted, links);
ImGui::SameLine();
DrawMarkerLayerFixSymmetryHeaderControl(layerIndex, markerLayers, markers, geometry, globalSymmetryMask,
                                        globalRadialRepeatCount, markerSymmetryFixSettings, state);
ImGui::SameLine();
DrawManualMarkerLayerColorOverrideHeaderControl(layer, state, bAnyCommitted, links);
```

Call site, `DrawLayerList`'s header-extra lambda (`MarkersTab_ManualLayers_UI.cpp:183-195`, specifically
line 194) — `rowIndex`/`markerLayers`/`markers`/`geometry`/`globalSymmetryMask`/`globalRadialRepeatCount`/
`markerSymmetryFixSettings` are ALL already `DrawLayerList`'s own parameters, already captured by this
`[&]` lambda — no new plumbing above this function:

```cpp
DrawRightAlignedSymmetryColorOverrideCluster(layer, rowIndex, markerLayers, markers, geometry,
                                             globalSymmetryMask, globalRadialRepeatCount,
                                             markerSymmetryFixSettings, state, bAnyNameCommitted, links);
```

Test call site, `MarkersTab_ManualLayers_UI_Test.cpp:341`
(`RunUngroupedClusterDoesNotOverlapAffordanceStripCheck`) breaks and must be updated to pass a
default-constructed `Params::Geometry`, a local `Params::MarkerSymmetryFixSettings`, `layerIndex = 0`,
and the test's own existing (empty) `markers`/single-element `layer`-backed `markerLayers` vector,
`globalSymmetryMask = 0`, `globalRadialRepeatCount = 0` — geometry/mask/count values are irrelevant to
this test's own pixel-overlap assertion (it never clicks the button), so any valid defaults are fine.

## Wiring — Bundle tree's Manual leaf

`DrawMarkerGroupLeafHeaderExtra` (declared `MarkersTab_Bundles_UI.h:215-223`, defined
`MarkersTab_BundleHeaderExtras_UI.cpp:252-309`) widens, inserting the same four command inputs after
`markers` (mirroring `DrawMarkerGroupLeafBody`'s own established parameter order,
`MarkersTab_Bundles_UI.cpp:26-35`):

```cpp
void DrawMarkerGroupLeafHeaderExtra(const MarkerGroupLeafKey_UI& leaf,
                                    std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers,
                                    const Params::Geometry& geometry, int globalSymmetryMask,
                                    int globalRadialRepeatCount,
                                    const Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                    ManualMarkerLayersState& manualLayersState,
                                    MarkerLayerBundlesState& bundlesState,
                                    const std::vector<int>& selectedManualInstanceIdentifiers,
                                    Pipeline::PreviewDriver* previewDriver, bool& bAnyCommitted,
                                    const std::vector<Params::MarkerLink>& links = {});
```

Body — insert between SYM and COL (`MarkersTab_BundleHeaderExtras_UI.cpp:276-278`), same relative order
as the flat list, `instanceLayers` doubling as `markerLayers` (this file's own established naming for
the same vector, confirmed by `DrawMarkerGroupLeafBody`'s own call):

```cpp
DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted, links);
ImGui::SameLine();
DrawMarkerLayerFixSymmetryHeaderControl(leaf.layerIndex, instanceLayers, markers, geometry,
                                        globalSymmetryMask, globalRadialRepeatCount,
                                        markerSymmetryFixSettings, manualLayersState);
ImGui::SameLine();
DrawManualMarkerLayerColorOverrideHeaderControl(layer, manualLayersState, bAnyCommitted, links);
```

Call site, `MarkersTab_Bundles_UI.cpp:168-172` (`DrawMarkerLayerBundleTree`'s own `drawLeafHeaderExtra`
lambda) — `geometry`/`globalSymmetryMask`/`globalRadialRepeatCount`/`markerSymmetryFixSettings` are
ALREADY `DrawMarkerLayerBundleTree`'s own function parameters (`:108-121`), already captured by this
`[&]` lambda — no new plumbing above this function either. (This is what
`MarkersTab_Bundles_UI.h:19`'s pre-existing `#include "MarkerSymmetryFixCommand_UI.h"` was quietly
anticipating — grep-confirmed it has zero direct uses in `MarkersTab_Bundles_UI.cpp`/`.h` today; this
ticket is the first thing that actually needs it.)

```cpp
[&](const MarkerGroupLeafKey_UI& leaf) {
    DrawMarkerGroupLeafHeaderExtra(leaf, ruleLayers, instanceLayers, markers, geometry, globalSymmetryMask,
                                   globalRadialRepeatCount, markerSymmetryFixSettings, rootState.manualLayers,
                                   state, rootState.selectedManualInstanceIdentifiers, previewDriver,
                                   bHeaderExtraCommitted, links);
},
```

Four test call sites in `MarkersTab_Bundles_UI_Test.cpp` (`:342`, `:352`, `:357`, `:416`) break and need
the same four new arguments inserted — a local default-constructed `Params::Geometry geometry;`,
`globalSymmetryMask = 0`, `globalRadialRepeatCount = 0`, and a local `Params::MarkerSymmetryFixSettings
markerSymmetryFixSettings;`, declared once per test function and passed at each of that function's own
call sites (`TestManualLeafDeleteButtonRecordsPendingIndex` at `:342`/`:352`/`:357`,
`TestProceduralLeafHeaderExtraDrawsDeleteButtonOnly` at `:416`). None of these four tests need real
geometry/mask/count/tolerance values — none of them click the new button.

## Explicit out-of-scope

- `Ui::FixMarkerLayerSymmetry`, `Pipeline::FindMarkerSymmetryMatches`, and both of their existing test
  binaries — algorithm/PARAMS-correct already, untouched.
- `Params::MarkerSymmetryFixSettings`'s own shape, its IO round-trip
  (`MapExporter_Symmetry_IO.cpp`/`MapImporter_Symmetry_IO.cpp`), and `Params::MapRecipe`'s own field —
  all already correct (STEP107 §5), untouched.
- No change to `DrawRightAlignedProceduralLayerCluster`'s own `clusterWidth` math or the ungrouped
  Procedural header (`MarkersTab_RuleLayers_UI.cpp`) — neither draws a FIX SYM button and neither
  reproduces any width-accounting defect from this addition (see Decision 1's own margin note).
- No new PARAMS/IO fields. No new `ManualMarkerLayersState` fields — every field the button/Global
  section pair needs (`fixSymmetryToleranceRange`, `fixSymmetryToleranceToggle`, `bFixSymmetryOverwrite`,
  `bHasFixSymmetryResult`, `lastFixSymmetryResult`) already exists (STEP107).
- No opportunistic file-size-ceiling split. `MarkersTab_ManualLayerRowBody_UI.cpp` is already well past
  ARCH §1.5's 150-line hard ceiling (342 lines pre-ticket) — pre-existing, not introduced here. Adding
  ~15 lines to an already-flagged file is not this ticket's remediation to invent; if the coder's own
  post-edit line count trips a formal ceiling flag, route it to the ARCH Expert as a STEP254/255-class
  follow-on, do not split files inline as a side effect of this ticket.

## Files touched

**New:** none.

**Deleted:**
- `src/ui/MarkerLayerSymmetrySection_UI.h`
- `src/ui/MarkerLayerSymmetrySection_UI.cpp`

**Modified:**
- `src/ui/MarkersTab_ManualLayerRowBody_UI.h` — new width constant + widened combined-width sum
  (`:47-72`), new `DrawMarkerLayerFixSymmetryHeaderControl` declaration (near `:131-132`).
- `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp` — `#include "MarkerSymmetryFixCommand_UI.h"`; new
  `DrawMarkerLayerFixSymmetryHeaderControl` definition (after `:237`).
- `src/ui/MarkersTab_ManualLayers_UI.h` — widened `DrawRightAlignedSymmetryColorOverrideCluster`
  declaration (`:120-122`).
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — widened `DrawRightAlignedSymmetryColorOverrideCluster`
  definition + `clusterWidth` math (`:92-127`); widened call site in `DrawLayerList` (`:194`).
- `src/ui/MarkersTab_ManualLayers_UI_Test.cpp` — updated call site (`:341`) in
  `RunUngroupedClusterDoesNotOverlapAffordanceStripCheck`; new acceptance coverage (see below).
- `src/ui/MarkersTab_Bundles_UI.h` — widened `DrawMarkerGroupLeafHeaderExtra` declaration
  (`:215-223`).
- `src/ui/MarkersTab_BundleHeaderExtras_UI.cpp` — widened `DrawMarkerGroupLeafHeaderExtra` definition +
  insertion between SYM/COL (`:252-309`).
- `src/ui/MarkersTab_Bundles_UI.cpp` — widened call site (`:168-172`).
- `src/ui/MarkersTab_Bundles_UI_Test.cpp` — updated four call sites (`:342`, `:352`, `:357`, `:416`).
- `src/ui/MarkersTab_Globals_UI.h` — `#include "Checkbox_UI.h"`, `#include "../params/Symmetry_PARAMS.h"`,
  forward-declare `ManualMarkerLayersState`, widened `DrawMarkersTabGlobals` declaration, new
  `DrawMarkerSymmetryFixSettings` declaration.
- `src/ui/MarkersTab_Globals_UI.cpp` — `#include "MarkersTab_ManualLayers_UI.h"`; new
  `DrawMarkerSymmetryFixSettings` definition; widened `DrawMarkersTabGlobals` body.
- `src/ui/MarkersTab_UI.cpp` — widened call site (`:72`).
- `src/ui/MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp` — new acceptance coverage (below); no
  CMakeLists change (already registered, `CMakeLists.txt:865-866`).
- `src/ui/MarkersTab_GlobalScaleRowLine_UI_Test.cpp` — new acceptance coverage (below); no CMakeLists
  change (already registered, `CMakeLists.txt:875-876`).

## Acceptance tests

1. **New header button, flat list** — `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`: a
   headless-frame click on `DrawMarkerLayerFixSymmetryHeaderControl`, with two markers on the target
   layer at genuinely mirrored positions (skip mode, `bOverwrite` false), asserts
   `state.bHasFixSymmetryResult == true` and `state.lastFixSymmetryResult.confirmedGroupCount == 1`
   after release, mirroring `MarkerSymmetryFixCommand_UI_Test.cpp`'s own already-proven skip-mode
   fixture (do not re-derive the fixture's own mirrored-position math — reuse or closely mirror that
   file's setup). Also asserts `state.bFixSymmetryOverwrite` is left `false` after a run started with it
   `true` (the "consumed per-use" contract).
2. **Width overlap, flat list** — extend
   `RunUngroupedClusterDoesNotOverlapAffordanceStripCheck` (`MarkersTab_ManualLayers_UI_Test.cpp`) with
   its updated widened call, both at default `ItemSpacing` and the existing exaggerated-spacing pass —
   `clusterMax.x <= stripMin.x + 0.5f` must still hold with the new button in the cluster.
3. **Bundle-tree wiring compiles and does not regress delete/rename** — the existing
   `TestManualLeafDeleteButtonRecordsPendingIndex`/`TestManualLeafHeaderExtraDrawsAndFlipsSymmetry`/
   `TestProceduralLeafHeaderExtraDrawsDeleteButtonOnly` in `MarkersTab_Bundles_UI_Test.cpp`, updated for
   the widened signature, stay green unmodified in their own assertions (proves the new button's
   insertion does not shift the delete-button's own right-aligned position or the SYM button's own
   flip behavior).
4. **Globals relocation** — `MarkersTab_GlobalScaleRowLine_UI_Test.cpp`: a headless frame calling
   `DrawMarkerSymmetryFixSettings` directly asserts the tolerance slider edits
   `markerSymmetryFixSettings.distanceTolerance`, the checkbox edits
   `manualLayersState.bFixSymmetryOverwrite`, and the result line renders (`ImGui::IsItemVisible()`-style
   presence check or item-count check) only when `bHasFixSymmetryResult` is true, mirroring this file's
   own existing headless-frame harness conventions.
5. **Deletion is clean** — full solo rebuild confirms no remaining reference to
   `MarkerLayerSymmetrySection_UI.h`/`.cpp`/`DrawLayerSymmetrySection`/`DrawFixSymmetryCommand`
   anywhere in `src/` (grep check), and `CMakeLists.txt`'s GLOB-based production source coverage needs
   no edit (no dedicated test binary existed for the deleted files).

Full solo rebuild + `ctest -C Debug`: every previously-passing test in
`MarkersTab_ManualLayers_UI_Test`, `MarkersTab_Bundles_UI_Test`,
`MarkersTab_ManualLayerColorOverrideHeader_UI_Test`, `MarkersTab_GlobalScaleRowLine_UI_Test`,
`MarkerSymmetryFixCommand_UI_Test`, and `MarkerSymmetryDetection_PIPELINE_Test` stays green.
