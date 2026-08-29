# STEP211 — Map areas fold into the composite: `PreviewLayerKind::MapAreas`, the `AreaColorTable_UI.h` extraction, and the two-recomposite drag-suppression rule

**Layer:** UI. **Domain:** `PreviewComposite_*_UI.*` (the composited field-layer stack), the Areas tab's
color side table (relocated), `MapCanvas`'s Area gesture (STEP210's own dispatch/draw files, amended).
**Executor:** SanGen Coder. Authored by the SanGen UI Expert, per `ARCH_14_17_MapAreaFieldLayer.md`
(§14.17, ratified, human-approved) and `ARCH_21_08_AreaCanvasGesture.md`'s 2026-08-29 amendment (§21.8).
Every file this ticket cites was read directly against the live tree while drafting it (not from
memory of any prior summary, and not taken on `sangen_arch_pack/INDEX.md`'s own paraphrase where a
direct read contradicted it — see Interpretation Call 2 below).

**Sequencing law (binding on this ticket and on whoever schedules around it):**
- This ticket, **STEP211**, must land **before** `work_orders/STEP212_AreaPerAreaLockAndCursorFix_UI.md`.
  STEP212 was drafted against the pre-this-ticket tree and will be **re-authored from scratch**
  afterward against whatever tree this ticket actually produces — nothing here tries to preserve
  compatibility with STEP212's already-written diffs.
- This ticket is **independent of** `work_orders/STEP213_GenericToggleButtonAndAutoLevel_UI.md`
  (zero file overlap) — no ordering constraint either way.
- **Internally, within this ticket: Piece C must not land before Piece B.** Quoting the ARCH ruling's
  own closing line verbatim: *"(C) must not land before (B): without the composite fill, suppressing
  the dragged area's fill would make it vanish mid-drag."* Piece A has no ordering dependency on B or
  C and may land first, alone, or combined with either.

## Summary
Three independently-shippable pieces, exactly as `ARCH_14_17_MapAreaFieldLayer.md`'s own closing
paragraph names them, landed here together in one ticket, in order:

- **Piece A** — extracts `AreaColorEntry`/`ResolveAreaColor` out of `AreasTab_List_UI.h` into a new
  minimal `src/ui/AreaColorTable_UI.h`, and moves ownership of the color table from
  `AreasTabState::areaColors` to `PreviewCompositeSettings::areaColors`. Pure refactor: the resolved
  default color is byte-identical to today's (white, 0.35 alpha) — Piece B is where it becomes green.
- **Piece B** — the real composited field layer: `PreviewLayerKind::MapAreas`, the 32-byte
  `PreviewMapAreaRectangle` record in cell space, `BuildMapAreaConfigurations()`, both shader twins,
  the `LayerSourceField` explicit null case, forward-iteration last-match-wins overlap, the
  PlayableArea-is-always-Green-and-non-editable rule, the panel-catalogue wiring that makes the
  Areas `[O]` toggle drive a real layer for the first time, and the mechanical downstream fallout of
  widening `PreviewComposite`'s own constructor (every direct call site in `src/ui/*_Test.cpp`).
- **Piece C** — the transient `mapAreaSuppressedIndex` drag-suppression slot, the
  `SetAreaCompositeRefreshCallback`/fifth-`SetManualAreaDragSource`-parameter plumbing, and
  `DrawAreaOverlayPass`'s amended contract (fill only the suppressed area; border only when the layer
  is enabled AND the area is suppressed AND it is selected) — exactly two recomposites per
  drag/resize/move gesture, never one per frame.

No lock concept of any kind is added anywhere in this ticket (that is STEP212's job, against the tree
this ticket produces). `Params::MapArea` and the `.sanmap` schema are completely untouched — every
change here is presentation state, the same category `PreviewCompositeSettings` already occupies.

## Required reading
1. `ARCH_14_17_MapAreaFieldLayer.md` — this ticket's entire binding law for Pieces A/B/C; every one
   of its 14 numbered items is cited by number below.
2. `ARCH_21_08_AreaCanvasGesture.md`, especially its "AMENDED 2026-08-29" closing section — the
   binding law for Piece C's draw-pass contract specifically.
3. `sangen_arch_pack/specs/PREVIEW_COMPOSITING_SPEC.md`'s "Map areas are a field layer, not an
   overlay domain" section (implementation-facing summary; SSBO-5/SSBO-6 historical correction).
4. `sangen_arch_pack/INDEX.md`'s §14.17 entry — cross-reference only; **its "six rows... drop Areas
   from its list" phrasing for `Application_Visibility_UI.h` is corrected by direct read in this
   ticket — see Interpretation Call 2. Do not act on that phrasing literally.**

---

# Piece A — the `AreaColorTable_UI.h` extraction and the `areaColors` ownership move

## A.1 New file: `src/ui/AreaColorTable_UI.h`

```cpp
// AreaColorTable_UI.h — the UI-only per-area presentation color, and nothing else. Layer: UI.
// Extracted out of AreasTab_List_UI.h (ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9) so
// PreviewComposite_Settings_UI.h — the color table's new single owner
// (PreviewCompositeSettings::areaColors) — never has to depend on a TAB header
// (AreasTab_List_UI.h pulls ColorSwatch_UI.h, RtToggleWidget_UI.h, UniqueNameList_UI.h and
// MapArea_PARAMS.h, none of which the composite settings header needs). Depends on nothing but
// <string>/<vector>.
//
// COLOR HAS NO `_PARAMS` HOME (STEP21 ruling #4, restated by §14.17 item 13) — it is presentation
// state, kept by NAME (not vector position — position drifts under a Reorder for no reason color
// needs to care about, Constitution §6), and this is now its one home: the Areas tab, MapCanvas's
// own drag gesture, and the composite's field-layer flattening all read/write this SAME table.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

enum : int { kAreaColorChannelCount = 4 };

// The default a freshly-resolved area color takes. Piece A of ARCH §14.17's own ticket keeps this
// byte-identical to what `AreasTab_List_UI.h`'s own `AreaColorEntry` used to default to (white,
// 0.35 alpha) — a pure refactor, no visual change. §14.17 item 10 changes this to green in Piece B.
inline constexpr float kDefaultAreaColor[kAreaColorChannelCount] = { 1.0f, 1.0f, 1.0f, 0.35f };

// A UI-only color, keyed by area NAME (STEP21 ruling #4).
struct AreaColorEntry {
    std::string name;
    float color[kAreaColorChannelCount] = { kDefaultAreaColor[0], kDefaultAreaColor[1],
                                            kDefaultAreaColor[2], kDefaultAreaColor[3] };
};

// Finds the color entry for `areaName`, or appends a fresh default-colored one on first touch — the
// same linear-scan idiom `NameIsTakenBefore` already uses. Returns the channel array directly so a
// caller can hand it straight to `DrawColorSwatch` or flatten it into a composite record.
inline float* ResolveAreaColor(std::vector<AreaColorEntry>& areaColors, const std::string& areaName) {
    for (AreaColorEntry& entry : areaColors)
        if (entry.name == areaName) return entry.color;
    AreaColorEntry entry;
    entry.name = areaName;
    areaColors.push_back(entry);
    return areaColors.back().color;
}

} // namespace Ui
} // namespace SanmapGen
```

## A.2 Modified: `src/ui/AreasTab_List_UI.h`

Remove the `AreaColorEntry` struct and `ResolveAreaColor` (moved to A.1); include the new header and
assert the two channel-count constants can never drift, the same "name the count and assert against
it" idiom `kPreviewBlendModeCount` already establishes.

**Full new file contents:**
```cpp
// AreasTab_List_UI.h — the pure lifecycle rules for the list of map areas. Layer: UI.
// Accuracy class: Visual-Exact (real `Params::MapArea` content). TAB_REBUILD_PLAN "§ Areas";
// tab-rebuild WO C4; retyped onto the real `Params::MapArea` by STEP21
// (`ENTITY_AUTHORING_PARAMS_SPEC.md`).
//
// Split out of AreasTab_UI.h so the tab header stays small (ARCH §1.5) and so the three rules that
// actually have teeth — the engine-required PlayableArea, the unique-name repair the export
// depends on, and "Set to Map Size" — are PURE and assertable with no imgui frame, window or GL
// context (WidgetHelpers_UI.h "THE SPLIT").
//
// v1 ran the unique-name repair as a loop tacked onto the end of the tab draw
// (gui/tabs/Tab_Areas.cpp), so it only ever ran while the tab was open. Here it is a function the
// tab calls and a test can call too.
//
// ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9 — `AreaColorEntry`/`ResolveAreaColor` moved OUT of
// this file into the new minimal `AreaColorTable_UI.h`; this file includes it and re-exports both
// names, so every existing call site (`AreasTab_UI.cpp`, `MapCanvas_AreaDraw_UI.cpp`,
// `AreasTab_UI_Test.cpp`) keeps compiling unchanged against `AreasTab_List_UI.h`. The color table's
// single OWNER is now `PreviewCompositeSettings::areaColors` (see that header) — not
// `AreasTabState`, which no longer carries a color field of its own.
#pragma once
#include <string>
#include <vector>
#include "AreaColorTable_UI.h"
#include "ColorSwatch_UI.h"
#include "RtToggleWidget_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MapArea_PARAMS.h"

namespace SanmapGen {
namespace Ui {

static_assert(kAreaColorChannelCount == kColorSwatchChannelCount,
             "AreaColorEntry's channel count must match the swatch widget's own, or DrawColorSwatch "
             "would read/write past the array ResolveAreaColor hands it.");

// The one area the engine requires. v1 keyed "cannot be removed" off the NAME, and so does v2:
// the name is what the exported map file carries, so the name is the identity.
inline constexpr const char* kPlayableAreaName = "PlayableArea";

inline bool IsPlayableArea(const Params::MapArea& area) { return area.name == kPlayableAreaName; }
inline bool IsAreaRemovable(const Params::MapArea& area) { return !IsPlayableArea(area); }

// The label a row shows — never empty (Constitution §6).
inline const char* AreaRowLabel(const Params::MapArea& area) {
    return area.name.empty() ? "Area" : area.name.c_str();
}

// A map side that can be drawn on: a recipe carrying a nonsense size is repaired, never obeyed.
inline int ResolvedAreaMapSize(int mapSize) { return mapSize > 1 ? mapSize : 1; }

// "Set to Map Size": the whole map, origin at the corner. Reports whether the rectangle moved, so
// a button press that changes nothing costs no recomposite.
inline bool SetAreaToMapSize(Params::MapArea& area, int mapSize) {
    const float extent = static_cast<float>(ResolvedAreaMapSize(mapSize));
    const bool bMoved = area.originX != 0.0f || area.originZ != 0.0f
                     || area.width != extent || area.length != extent;
    area.originX = 0.0f;
    area.originZ = 0.0f;
    area.width   = extent;
    area.length  = extent;
    return bMoved;
}

// The name v1's Add New Area button coined, kept so an imported v1 project reads the same. Thin
// domain wrapper over the shared cross-entity template (UniqueNameList_UI.h, STEP20 ARCH ruling).
inline std::string NextAreaName(int areaCount) { return NextUniqueLabel("NewArea", areaCount); }

// The engine-required area is present or it is created, at the FRONT and sized to the map. Reports
// whether the list moved.
inline bool EnsurePlayableArea(std::vector<Params::MapArea>& areas, int mapSize) {
    for (const Params::MapArea& area : areas)
        if (IsPlayableArea(area)) return false;
    Params::MapArea playableArea;
    playableArea.name = kPlayableAreaName;
    SetAreaToMapSize(playableArea, mapSize);
    areas.insert(areas.begin(), playableArea);
    return true;
}

} // namespace Ui
} // namespace SanmapGen
```

## A.3 Modified: `src/ui/PreviewComposite_Settings_UI.h`

Add the include and the new owning field. Insert the include alphabetically among the existing one:

```cpp
#pragma once
#include <vector>
#include "AreaColorTable_UI.h"
#include "../params/GradientRamp_PARAMS.h"
```

Add the new member at the end of `PreviewCompositeSettings`, right after `worldUnitsPerCell`:

```cpp
    // Heightfield cell -> game units (X/Z), the same quantity Placement emitted its instance
    // positions with (`Params::Geometry::worldUnitsPerCell` — map geometry, M5-0a). PIPELINE
    // sets this mirror and Placement's reader from that one recipe value (M4-5).
    float worldUnitsPerCell = 1.0f;

    // ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9 — the single owner of the per-area presentation
    // color, moved here from `AreasTabState::areaColors` (removed, not duplicated): the Areas tab,
    // MapCanvas's own drag gesture and the composite's own field-layer flattening all need the SAME
    // mutable table, and this is the category `gradientRamps`/`clearColor` already occupy —
    // presentation state that never serializes into `mapGeneratorData`.
    std::vector<AreaColorEntry> areaColors;
};
```

## A.4 Modified: `src/ui/AreasTab_UI.h`

Remove `AreasTabState::areaColors`; `DrawAreasTab` gains a `std::vector<AreaColorEntry>&` parameter
(the narrowest type the function actually needs — `ResolveAreaColor` and the rename-retargeting loop
both already operate on exactly this type; a wider `PreviewCompositeSettings&` was considered and
rejected as more coupling than the tab needs, see Interpretation Call 6).

**Diff 1** — the top-of-file comment's claim about where color lives is stale after the move; correct
it in place (lines 6-10 today):
```cpp
// AreasTab_UI.h — the areas tab: the named rectangles a map carries beside its terrain (the
// engine-required PlayableArea plus whatever regions a designer adds). Layer: UI.
// Accuracy class: Visual/Exact. TAB_REBUILD_PLAN "§ Areas"; tab-rebuild WO C4; retyped onto the
// real `Params::MapArea` by STEP21 (`ENTITY_AUTHORING_PARAMS_SPEC.md`).
//
// The stack is a DraggableList — an ORDERED set of tens of rows where every row is a drop target,
// which is exactly what that widget exists for — and every scalar is a shared SliderScalar
// carrying its own RealtimeToggle. The color is the picker-only ColorSwatch with its alpha BAR
// enabled: the areas tab is the one caller ColorSwatchOptions::bAlphaBarShown was added for.
// The pure list rules live in AreasTab_List_UI.h. The UI-only per-area color side table lives in
// `PreviewCompositeSettings::areaColors` (`AreaColorTable_UI.h` for the type itself,
// ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9) — this tab reaches it through a `DrawAreasTab`
// parameter, not a field of its own state.
```

**Diff 2** — `AreasTabState` loses `areaColors` (currently line 34):
```cpp
struct AreasTabState {
    SectionState       globalSection;
    SectionState       areaSection;
    ColorSwatchOptions colorOptions = ColorSwatchOptions();
    int  selectedAreaIndex = -1;
    bool bAreasLocked      = true;    // v1 parity, including v1's default: while set, the map
                                      // canvas may not drag or resize an area (WO E reads it)
```
(the `std::vector<AreaColorEntry> areaColors;` line is deleted, nothing else in the struct changes).

**Diff 3** — `DrawAreasTab`'s declaration (currently lines 88-89) gains the new parameter:
```cpp
// `recipe.areas` is edited directly (STEP21) — the tab reads `geometry.mapSize` to size its
// sliders and the Set to Map Size button, and writes back through `recipe.areas`. `areaColors` is
// `PreviewCompositeSettings::areaColors` (ARCH §14.17 item 9) — the tab's own call site passes
// `composite.Settings().areaColors`, never a copy of its own.
void DrawAreasTab(Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver, std::vector<AreaColorEntry>& areaColors);
```

## A.5 Modified: `src/ui/AreasTab_UI.cpp`

`DrawAreaSettings`, `DrawAreaList` and `DrawAreasTab` rebind to the injected reference instead of
`state.areaColors`. (Piece B adds one more edit inside `DrawAreaSettings` — see B.10.)

```cpp
bool DrawAreaSettings(Params::MapArea& area, AreasTabState& state, int mapSize,
                      std::vector<AreaColorEntry>& areaColors) {
    const ScalarSliderRange originRange = AreaOriginSliderRange(mapSize);
    const ScalarSliderRange extentRange = AreaExtentSliderRange(mapSize);
    bool bCommitted = false;
    if (IsPlayableArea(area)) {
        ImGui::TextDisabled("PlayableArea is required by the engine: it cannot be renamed or removed.");
    } else {
        const std::string nameBeforeEdit = area.name;
        TextInputRules nameRules;
        nameRules.maximumLength = 48;
        nameRules.bAllowEmpty   = false;
        nameRules.fallbackText  = "Area";
        bCommitted = DrawTextInput("Name", area.name, nameRules).bCommitted;
        if (bCommitted && area.name != nameBeforeEdit) {
            for (AreaColorEntry& entry : areaColors)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
        }
    }
    bCommitted = DrawSliderScalar("X Position", area.originX, originRange, state.originXToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Z Position", area.originZ, originRange, state.originZToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Width", area.width, extentRange, state.widthToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Length", area.length, extentRange, state.lengthToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    float* const color = ResolveAreaColor(areaColors, area.name);
    bCommitted = DrawColorSwatch("Color", color, state.colorOptions,
                                 state.colorToggle).bCommitted || bCommitted;
    if (ImGui::Button("Set to Map Size")) bCommitted = SetAreaToMapSize(area, mapSize) || bCommitted;
    return bCommitted;
}

DraggableListSignal DrawAreaList(std::vector<Params::MapArea>& areas, AreasTabState& state,
                                 int mapSize, std::vector<AreaColorEntry>& areaColors, bool& bAreasMoved) {
    return DraggableList<Params::MapArea>::Render(
        "areas", areas,
        [&](int rowIndex) {
            const Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label   = AreaRowLabel(area);
            row.bLocked = !IsAreaRemovable(area);      // the PlayableArea's protection, shown
            return row;
        },
        [&](int rowIndex) {
            Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            bAreasMoved = DrawAreaSettings(area, state, mapSize, areaColors) || bAreasMoved;
        },
        state.selectedAreaIndex);
}
```

`DrawAreasTab` (currently lines 129-145):
```cpp
void DrawAreasTab(Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver, std::vector<AreaColorEntry>& areaColors) {
    ImGui::PushID("areasTab");
    const int mapSize = recipe.geometry.mapSize;
    bool bAreasMoved = EnsurePlayableArea(recipe.areas, mapSize);
    bAreasMoved = DrawAreasGlobals(recipe.areas, state) || bAreasMoved;
    if (DrawSectionBegin("Area Stack", state.areaSection)) {
        const DraggableListSignal signal = DrawAreaList(recipe.areas, state, mapSize, areaColors, bAreasMoved);
        if (signal.bHasSignal()) bAreasMoved = ApplyAreaListSignal(recipe.areas, state, signal) || bAreasMoved;
        DrawSectionEnd();
    }
    // The export keys areas by NAME, so the duplicate repair runs on the frames a name settled —
    // not every frame, which would rename a row mid-typing.
    if (bAreasMoved) MakeNamesUnique(recipe.areas);
    NotifyPlacementChange(bAreasMoved, previewDriver);
    ImGui::PopID();
}
```
(`ApplyAreaListSignal` and `DrawAreasGlobals` are untouched — full functions omitted here since
nothing about them changes.)

## A.6 Modified: `src/ui/Application_PanelEnvironment_UI.cpp`

The one call site (currently line 57), per ARCH §14.17 item 9's own threading instruction:
```cpp
        case ApplicationPanel::Areas:
            DrawAreasTab(recipe, tabState.areas, &previewDriver, composite.Settings().areaColors);
            break;
```

## A.7 Modified: `src/ui/Application_UI.cpp`

The `SetManualAreaDragSource` call (currently lines 143-147) retargets its `areaColors` argument to
the relocated storage (its `mapAreaSuppressedIndex` fifth argument is added in Piece C, C.6, not here):
```cpp
    // ARCH §21.8 / §14.17 item 9 — the Area canvas gesture's drag source: `recipe.areas` and
    // `composite.Settings().areaColors` are the SAME storage the Areas tab itself edits — one
    // source of truth, never a second copy. `areaColors` moved off `tabState.areas.areaColors`
    // (removed) onto the composite's own settings (§14.17 item 9).
    canvas.SetManualAreaDragSource(&recipe.areas, &composite.Settings().areaColors,
                                   &tabState.areas.bAreasLocked, &tabState.areas.selectedAreaIndex);
```

## A.8 Modified: `src/ui/MapCanvas_ManualDragSources_UI.h`

Retarget the `#include` per the §21.8 amendment's own explicit instruction (clause 4): "this file's
`#include "AreasTab_List_UI.h" // AreaColorEntry` retargets to the new header." Current include
block (lines 9-16):
```cpp
#include <vector>
#include "AreaColorTable_UI.h"         // AreaColorEntry — ARCH §14.17 item 9's retarget
#include "AreaDragGesture_UI.h"
#include "InstanceDragGesture_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MapArea_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
```
(`ManualAreaDragSources_UI`'s own body is unchanged in Piece A — Piece C, C.5, adds the
`mapAreaSuppressedIndex` field.)

---

# Piece B — the field layer itself

## B.1 Modified: `src/ui/AreaColorTable_UI.h`

The default color changes from white to green, per §14.17 item 10 (`"PlayableArea"`'s own
force-green rule is a separate, additional edit — B.10 below):
```cpp
// The default a freshly-resolved area color takes: Green, blend Overlay (ARCH §14.17 item 10) — RGB
// changed from Piece A's white-parity placeholder; the pre-existing 0.35 fill alpha is kept verbatim
// (v1 parity, `AreasTab_List_UI.h`'s own prior default).
inline constexpr float kDefaultAreaColor[kAreaColorChannelCount] = { 0.0f, 1.0f, 0.0f, 0.35f };
```

## B.2 Modified: `src/ui/AreasTab_UI_Test.cpp`

`RunAreaColorResolutionChecks` asserted the OLD (white) default; update it to the new (green) one
(currently lines 123-129):
```cpp
void RunAreaColorResolutionChecks() {
    std::vector<AreaColorEntry> areaColors;
    float* const firstResolve = ResolveAreaColor(areaColors, "Base");
    Check(areaColors.size() == 1u, "the first touch of a name appends one entry");
    Check(firstResolve[0] == 0.0f && firstResolve[1] == 1.0f && firstResolve[2] == 0.0f
          && firstResolve[3] == 0.35f,
          "a fresh entry defaults to Green/0.35 (ARCH_14_17_MapAreaFieldLayer.md §14.17 item 10)");
```
(the rest of that function — the second/third `ResolveAreaColor` checks — is unchanged; only the
literal expected values and the label text on this one `Check` line change.)

## B.3 Modified: `src/ui/AreasTab_UI.cpp`

Two edits: a new include, and the PlayableArea disabled/re-pin swatch block inside `DrawAreaSettings`
(§14.17 item 10 — the swatch is always drawn `ImGui::BeginDisabled`/`EndDisabled` for the
PlayableArea, and re-pinned to `kDefaultAreaColor` first, mirroring the row-lock precedent that
already keys "cannot be removed" off `IsPlayableArea` in this same file's `DrawAreaList`).

New include, added to the existing block:
```cpp
#include "AreaColorTable_UI.h"
#include "AreasTab_UI.h"
```

Replace the swatch two lines (from A.5's version) inside `DrawAreaSettings`:
```cpp
    // ARCH §14.17 item 10 — PlayableArea is always Green and non-editable: re-pin its color before
    // drawing (the swatch below is the only OTHER path that could ever set a PlayableArea color) and
    // disable the control so a designer cannot pick a different one.
    float* const color = ResolveAreaColor(areaColors, area.name);
    if (IsPlayableArea(area)) {
        color[0] = kDefaultAreaColor[0]; color[1] = kDefaultAreaColor[1];
        color[2] = kDefaultAreaColor[2]; color[3] = kDefaultAreaColor[3];
        ImGui::BeginDisabled();
        DrawColorSwatch("Color", color, state.colorOptions, state.colorToggle);
        ImGui::EndDisabled();
    } else {
        bCommitted = DrawColorSwatch("Color", color, state.colorOptions,
                                     state.colorToggle).bCommitted || bCommitted;
    }
```

## B.4 Modified: `src/ui/PreviewComposite_Settings_UI.h`

The enum gains `MapAreas`, appended last (its integer value is load-bearing on both the CPU switch
and the generated GLSL `#define`s, §14.17 item 3); the stale comment above it is corrected per
item 14:
```cpp
// Which per-pixel COLOR SOURCE a layer draws — a baked `Data::MapFields` field, SAMPLED and never
// re-derived (Slope is the Mask stage's own bake, colorized as-is: this is what keeps the shadow-sim
// deleted, ARCH §3.2), a PARAMS-flattened analytic source with no baked field behind it at all
// (StratumSplat's nine weight fields + tints; MapAreas' rectangles + colors,
// ARCH_14_17_MapAreaFieldLayer.md §14.17 item 1), or a combination (Water: a threshold over the
// heightfield, parameterized by `Params::Water`). What a layer may NEVER be is a re-decision of a
// PLACEMENT rule (markers/props/decals/units/reclaim) — that is the shadow-sim defect this comment
// used to describe too narrowly by omission (PREVIEW_COMPOSITING_SPEC "the shadow-sim problem").
enum class PreviewLayerKind { HeightRamp, StratumSplat, Flow, Accumulation, Water, Slope, MapAreas };
```

## B.5 Modified: `src/ui/PreviewComposite_Kernel_UI.h`

New binding, new buffer name, and the flattened record — per §14.17 item 4.

`CompositeBinding` (append after `kSlope`):
```cpp
namespace CompositeBinding {
constexpr unsigned kEntityIdentifiers      = 0;
constexpr unsigned kHeightfield            = 1;
constexpr unsigned kFlow                   = 2;
constexpr unsigned kAccumulation           = 3;
constexpr unsigned kSurfaceStratumWeights  = 4;   // the 9 baked weight fields, concatenated
constexpr unsigned kGradientLookupTables   = 5;   // every baked ramp table, concatenated
constexpr unsigned kEntityPoints           = 6;
constexpr unsigned kConfiguration          = 8;
constexpr unsigned kLayerConfigurations    = 9;
constexpr unsigned kStratumConfigurations  = 10;
constexpr unsigned kSlope                  = 11;  // the Mask stage's baked slope (M5-0c)
constexpr unsigned kMapAreaRectangles      = 12;  // ARCH §14.17 — binding 7 stays vacant (see above)
} // namespace CompositeBinding
```

`CompositeBufferName` (append after `kSlope`):
```cpp
namespace CompositeBufferName {
constexpr const char* kEntityIdentifiers     = "previewCompositeEntityIdentifiers";
constexpr const char* kHeightfield           = "previewCompositeHeightfield";
constexpr const char* kFlow                  = "previewCompositeFlow";
constexpr const char* kAccumulation          = "previewCompositeAccumulation";
constexpr const char* kSurfaceStratumWeights = "previewCompositeSurfaceStratumWeights";
constexpr const char* kGradientLookupTables  = "previewCompositeGradientLookupTables";
constexpr const char* kEntityPoints          = "previewCompositeEntityPoints";
constexpr const char* kConfiguration         = "previewCompositeConfiguration";
constexpr const char* kLayerConfigurations   = "previewCompositeLayerConfigurations";
constexpr const char* kStratumConfigurations = "previewCompositeStratumConfigurations";
constexpr const char* kSlope                 = "previewCompositeSlope";
constexpr const char* kMapAreaRectangles     = "previewCompositeMapAreaRectangles";
} // namespace CompositeBufferName
```

New record, added beside `PreviewStratumConfiguration` (verbatim from §14.17 item 4):
```cpp
// One map area, flattened to the composite's own CELL space and its presentation color.
// 8 scalars = 32 bytes. Areas are PRESENTATION geometry: no placement rule stands behind them,
// so this record re-decides nothing a PROC stage resolved (ARCH §14.17 item 1).
struct PreviewMapAreaRectangle {
    float minimumX = 0.0f;
    float minimumZ = 0.0f;
    float maximumX = 0.0f;
    float maximumZ = 0.0f;
    float colorRed = 0.0f;
    float colorGreen = 0.0f;
    float colorBlue = 0.0f;
    float colorAlpha = 0.0f;
};
```

## B.6 Modified: `src/ui/PreviewComposite_UI.h`

New include, widened constructor, new member, new private method, new data member.

Include list (insert alphabetically between `Geometry_PARAMS.h` and `Stratum_PARAMS.h`):
```cpp
#include "../params/Geometry_PARAMS.h"
#include "../params/MapArea_PARAMS.h"
#include "../params/Stratum_PARAMS.h"
#include "../params/Water_PARAMS.h"
```

Constructor declaration (currently lines 43-47):
```cpp
    PreviewComposite(const Params::Geometry& geometrySettings, const Params::Water& waterSettings,
                     const std::vector<Params::Stratum>& stratumSettings,
                     const std::vector<Params::MapArea>& mapAreaSettings,
                     const Data::MapFields& inputFields,
                     const Data::PlacementInstances& placedInstances,
                     Data::EntityIdBuffer& entityIdentifierOutput);
```

Private method declarations (currently lines 100-102) gain one line:
```cpp
    void PrepareRun();                                       // PreviewComposite_UI.cpp
    void BuildConfigurationRecord();                         // PreviewComposite_UI.cpp
    void BuildStratumConfigurations();                       // PreviewComposite_UI.cpp
    void BuildMapAreaConfigurations();                       // PreviewComposite_Prepare_UI.cpp
    void BuildLayerConfigurations();                         // PreviewComposite_Prepare_UI.cpp
    void BuildEntityPoints();                                // PreviewComposite_Prepare_UI.cpp
```

Member data (currently lines 119-133) gains `areas` beside `strata`, and `mapAreaRectangles` beside
`stratumConfigurations`:
```cpp
    const Params::Geometry&             geometry;
    const Params::Water&                water;
    const std::vector<Params::Stratum>& strata;
    const std::vector<Params::MapArea>& areas;
    const Data::MapFields&              mapFields;
    const Data::PlacementInstances&     instances;
    Data::EntityIdBuffer&               entityIdentifierBuffer;
    PreviewCompositeSettings            settings;

    PreviewCompositeConfiguration            configuration;
    std::vector<PreviewLayerConfiguration>   layerConfigurations;
    std::vector<PreviewStratumConfiguration> stratumConfigurations;
    std::vector<PreviewMapAreaRectangle>     mapAreaRectangles;
    std::vector<PreviewEntityPoint>          entityPoints;
```

## B.7 Modified: `src/ui/PreviewComposite_UI.cpp`

Constructor definition (currently lines 20-28):
```cpp
PreviewComposite::PreviewComposite(const Params::Geometry& geometrySettings,
                                   const Params::Water& waterSettings,
                                   const std::vector<Params::Stratum>& stratumSettings,
                                   const std::vector<Params::MapArea>& mapAreaSettings,
                                   const Data::MapFields& inputFields,
                                   const Data::PlacementInstances& placedInstances,
                                   Data::EntityIdBuffer& entityIdentifierOutput)
    : geometry(geometrySettings), water(waterSettings), strata(stratumSettings), areas(mapAreaSettings),
      mapFields(inputFields), instances(placedInstances),
      entityIdentifierBuffer(entityIdentifierOutput) {}
```

`PrepareRun()` (currently lines 78-91) gains one call, right after `BuildLayerConfigurations()`:
```cpp
void PreviewComposite::PrepareRun() {
    BuildConfigurationRecord();
    BuildStratumConfigurations();
    BuildLayerConfigurations();
    BuildMapAreaConfigurations();
    BuildEntityPoints();
    const std::size_t texelCount = static_cast<std::size_t>(configuration.previewResolution)
                                 * configuration.previewResolution;
    compositeTexels.assign(texelCount, 0u);
    if (entityIdentifierBuffer.Width() != configuration.previewResolution
        || entityIdentifierBuffer.Height() != configuration.previewResolution)
        entityIdentifierBuffer.Resize(configuration.previewResolution,
                                      configuration.previewResolution);
    executedPassCount = 0;
}
```

## B.8 Modified: `src/ui/PreviewComposite_Prepare_UI.cpp`

`LayerSourceField` gets the explicit `case`, never the `default:` fall-through (§14.17 item 5):
```cpp
const Data::FloatField* PreviewComposite::LayerSourceField(PreviewLayerKind kind) const {
    switch (kind) {
        case PreviewLayerKind::Flow:         return &mapFields.flow;
        case PreviewLayerKind::Accumulation: return &mapFields.accumulation;
        case PreviewLayerKind::Slope:        return &mapFields.slope;
        case PreviewLayerKind::StratumSplat: return nullptr;
        case PreviewLayerKind::MapAreas:     return nullptr;
        default:                             return &mapFields.heightfield;
    }
}
```

New function, appended at the end of the file, before the closing `namespace Ui`/`namespace
SanmapGen` braces (§14.17 items 4/6/9 — cell-space flattening via the SAME reciprocal
`WorldToPreviewPixel` already uses, forward-iteration with the suppressed-index skip that Piece C's
own `mapAreaSuppressedIndex` field makes meaningful — the field already exists as `-1` by
default even before Piece C wires anything into it, so this function is correct and complete on its
own the moment Piece B lands, per the dispatch note's "(C) is inert until (B) lands" framing):
```cpp
// One rectangle per `recipe.areas` entry (skipping the currently drag-suppressed index, ARCH §14.17
// item 11), flattened to CELL space with its resolved presentation color — the SAME reciprocal
// WorldToPreviewPixel already takes, so multiply-never-divide holds (Constitution §3) and there is
// no second copy of the world->cell arithmetic. An empty (or entirely-suppressed) result still
// pushes one degenerate sentinel rectangle (`minimumX > maximumX`), so the buffer this binds is
// never zero bytes and the shader's forward scan costs exactly one rejected iteration.
void PreviewComposite::BuildMapAreaConfigurations() {
    mapAreaRectangles.clear();
    const float cellsPerWorldUnit = ReciprocalOrZero(settings.worldUnitsPerCell);
    for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
        if (index == settings.mapAreaSuppressedIndex) continue;
        const Params::MapArea& area = areas[static_cast<std::size_t>(index)];
        PreviewMapAreaRectangle record;
        record.minimumX = area.originX * cellsPerWorldUnit;
        record.minimumZ = area.originZ * cellsPerWorldUnit;
        record.maximumX = (area.originX + area.width) * cellsPerWorldUnit;
        record.maximumZ = (area.originZ + area.length) * cellsPerWorldUnit;
        float* const color = ResolveAreaColor(settings.areaColors, area.name);
        record.colorRed   = color[0];
        record.colorGreen = color[1];
        record.colorBlue  = color[2];
        record.colorAlpha = color[3];
        mapAreaRectangles.push_back(record);
    }
    if (mapAreaRectangles.empty()) {
        PreviewMapAreaRectangle sentinel;
        sentinel.minimumX = 1.0f;
        sentinel.maximumX = -1.0f;   // minimumX > maximumX: fails every sample test unconditionally
        mapAreaRectangles.push_back(sentinel);
    }
}
```
(This function reads `settings.mapAreaSuppressedIndex`, a field Piece C, C.1, adds to
`PreviewCompositeSettings` — see the ordering note at the top of this ticket: land B before C, or
this line references a not-yet-existing field. If Piece B ships alone without Piece C, add
`int mapAreaSuppressedIndex = -1;` to `PreviewCompositeSettings` as part of Piece B instead, and
Piece C then only adds the plumbing that WRITES it — the field itself is harmless, inert
presentation state either way.)

## B.9 Modified: `src/ui/PreviewComposite_Cpu_UI.cpp`

`LayerColorAtPixel` gains the `MapAreas` branch, added in the SAME change as B.8's `LayerSourceField`
case (§14.17 item 5's own explicit requirement — the CPU twin's null deref at the generic
`sourceField->SampleBilinear` line is unreachable ONLY because this early-return exists, the exact
same invariant `StratumSplat` already relies on). Full updated function:
```cpp
PreviewColor PreviewComposite::LayerColorAtPixel(const PreviewLayerConfiguration& layerConfiguration,
                                                 float sampleX, float sampleY) const {
    const float* const lookupTable = layerConfiguration.gradientLookupOffset >= 0
                                   ? gradientLookupTables.data() + layerConfiguration.gradientLookupOffset
                                   : nullptr;
    const int lookupEntryCount = layerConfiguration.gradientLookupEntryCount;
    const PreviewLayerKind layerKind = static_cast<PreviewLayerKind>(layerConfiguration.layerKind);
    if (layerKind == PreviewLayerKind::StratumSplat) {
        float stratumWeights[Data::MapFields::stratumCount];
        for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
            stratumWeights[stratum] =
                mapFields.surfaceStratumWeights[stratum].SampleBilinear(sampleX, sampleY);
        return SplatSurfaceStrata(stratumWeights, stratumConfigurations.data(),
                                  Data::MapFields::stratumCount,
                                  configuration.bNormalizeSplatWeights,
                                  configuration.splatWeightEpsilon);
    }
    if (layerKind == PreviewLayerKind::MapAreas) {
        // ARCH §14.17 items 5/6 — forward iteration, LAST containing match wins (the same Z rule
        // §21.8's own body hit-test already implements), so click-to-select and what-you-see can
        // never disagree. The degenerate sentinel fails the first test unconditionally.
        PreviewColor result;
        for (const PreviewMapAreaRectangle& rectangle : mapAreaRectangles) {
            if (sampleX < rectangle.minimumX || sampleX > rectangle.maximumX) continue;
            if (sampleY < rectangle.minimumZ || sampleY > rectangle.maximumZ) continue;
            result.red = rectangle.colorRed; result.green = rectangle.colorGreen;
            result.blue = rectangle.colorBlue; result.alpha = rectangle.colorAlpha;
        }
        return result;
    }
    if (layerKind == PreviewLayerKind::Water) {
        if (configuration.bWaterEnabled == 0) return PreviewColor();
        const float depth = NormalizedWaterDepth(
            mapFields.heightfield.SampleBilinear(sampleX, sampleY), configuration);
        if (depth < 0.0f) return PreviewColor();          // the baked surface is above the water
        return SampleGradientLookupTable(lookupTable, lookupEntryCount, depth);
    }
    const Data::FloatField* const sourceField = LayerSourceField(layerKind);
    const float value = sourceField->SampleBilinear(sampleX, sampleY);
    return SampleGradientLookupTable(lookupTable, lookupEntryCount,
                                     NormalizeToDomain(value, layerConfiguration.domainMinimum,
                                                       layerConfiguration.domainRangeReciprocal));
}
```

## B.10 Modified: `src/ui/PreviewComposite_Sampling_UI.glsl`

New struct + buffer declaration (added after the existing `StratumConfigurations` buffer
declaration, currently line 40):
```glsl
layout(std430, binding = PREVIEW_BINDING_STRATA)  readonly buffer StratumConfigurations { StratumConfiguration stratumConfigurations[]; };

// ARCH §14.17 item 4 — one map area, cell-space bounds + resolved color. Declared in THIS unit only
// (the pass unit reaches it only through layerColorAtPixel, this file's own stated convention).
struct MapAreaRectangle {
    float minimumX;  float minimumZ;  float maximumX;  float maximumZ;
    float colorRed;  float colorGreen; float colorBlue; float colorAlpha;
};
layout(std430, binding = PREVIEW_BINDING_MAP_AREAS) readonly buffer MapAreaRectangles { MapAreaRectangle mapAreaRectangles[]; };
```

New function, added after `splatSurfaceStrata` (verbatim from §14.17 item 6):
```glsl
vec4 mapAreaColorAtCell(float sampleX, float sampleY) {
    vec4 result = vec4(0.0);
    for (int index = 0; index < mapAreaRectangles.length(); ++index) {
        MapAreaRectangle area = mapAreaRectangles[index];
        if (sampleX < area.minimumX || sampleX > area.maximumX) continue;
        if (sampleY < area.minimumZ || sampleY > area.maximumZ) continue;
        result = vec4(area.colorRed, area.colorGreen, area.colorBlue, area.colorAlpha);
    }
    return result;
}
```

`layerColorAtPixel`'s body gains the MapAreas branch, in the same relative position as the CPU
twin (B.9):
```glsl
vec4 layerColorAtPixel(int layerIndex, float sampleX, float sampleY) {
    LayerConfiguration layer = layerConfigurations[layerIndex];
    if (layer.layerKind == PREVIEW_LAYER_STRATUM_SPLAT) return splatSurfaceStrata(sampleX, sampleY);
    if (layer.layerKind == PREVIEW_LAYER_MAP_AREAS) return mapAreaColorAtCell(sampleX, sampleY);
    if (layer.layerKind == PREVIEW_LAYER_WATER) {
        if (configuration[0].bWaterEnabled == 0) return vec4(0.0);
        float depth = normalizedWaterDepth(sampleFieldBilinear(layer.layerKind, sampleX, sampleY),
                                           configuration[0].terrainMaxHeight,
                                           configuration[0].waterLevelMaximum,
                                           configuration[0].deepWaterDepthMinimum,
                                           configuration[0].deepWaterDepthRangeReciprocal);
        if (depth < 0.0) return vec4(0.0);              // the baked surface is above the water
        return sampleGradientLookupTable(layer.gradientLookupOffset, layer.gradientLookupEntryCount, depth);
    }
    return sampleGradientLookupTable(layer.gradientLookupOffset, layer.gradientLookupEntryCount,
                                     normalizeToDomain(sampleFieldBilinear(layer.layerKind, sampleX, sampleY),
                                                       layer.domainMinimum, layer.domainRangeReciprocal));
}
```

## B.11 Modified: `src/ui/PreviewComposite_GpuBuffers_UI.cpp`

`BindComposeBuffers` gains one more `EnsureAndBind` line, appended after the existing
`kStratumConfigurations` line (currently the function's last statement):
```cpp
    EnsureAndBind(manager, CompositeBufferName::kStratumConfigurations, stratumConfigurations.data(),
                  stratumConfigurations.size() * sizeof(PreviewStratumConfiguration),
                  CompositeBinding::kStratumConfigurations);
    EnsureAndBind(manager, CompositeBufferName::kMapAreaRectangles, mapAreaRectangles.data(),
                  mapAreaRectangles.size() * sizeof(PreviewMapAreaRectangle),
                  CompositeBinding::kMapAreaRectangles);
}
```

## B.12 Modified: `src/ui/PreviewComposite_GpuProgram_UI.cpp`

`BuildEnumDefinitions()` gains one line, appended right after `PREVIEW_LAYER_SLOPE`:
```cpp
std::string BuildEnumDefinitions() {
    return IntegerDefinition("PREVIEW_LAYER_HEIGHT_RAMP",   static_cast<int>(PreviewLayerKind::HeightRamp))
         + IntegerDefinition("PREVIEW_LAYER_STRATUM_SPLAT", static_cast<int>(PreviewLayerKind::StratumSplat))
         + IntegerDefinition("PREVIEW_LAYER_FLOW",          static_cast<int>(PreviewLayerKind::Flow))
         + IntegerDefinition("PREVIEW_LAYER_ACCUMULATION",  static_cast<int>(PreviewLayerKind::Accumulation))
         + IntegerDefinition("PREVIEW_LAYER_WATER",         static_cast<int>(PreviewLayerKind::Water))
         + IntegerDefinition("PREVIEW_LAYER_SLOPE",         static_cast<int>(PreviewLayerKind::Slope))
         + IntegerDefinition("PREVIEW_LAYER_MAP_AREAS",     static_cast<int>(PreviewLayerKind::MapAreas))
         + IntegerDefinition("PREVIEW_BLEND_REPLACE",       static_cast<int>(PreviewBlendMode::Replace))
         + IntegerDefinition("PREVIEW_BLEND_ALPHA",         static_cast<int>(PreviewBlendMode::AlphaBlend))
         + IntegerDefinition("PREVIEW_BLEND_ADD",           static_cast<int>(PreviewBlendMode::Add))
         + IntegerDefinition("PREVIEW_BLEND_MULTIPLY",      static_cast<int>(PreviewBlendMode::Multiply))
         + IntegerDefinition("PREVIEW_BLEND_MAXIMUM",       static_cast<int>(PreviewBlendMode::Maximum))
         + IntegerDefinition("PREVIEW_BLEND_MINIMUM",       static_cast<int>(PreviewBlendMode::Minimum))
         + IntegerDefinition("PREVIEW_BLEND_SUBTRACT",      static_cast<int>(PreviewBlendMode::Subtract))
         + IntegerDefinition("PREVIEW_BLEND_DIVIDE",        static_cast<int>(PreviewBlendMode::Divide))
         + IntegerDefinition("PREVIEW_BLEND_OVERLAY",       static_cast<int>(PreviewBlendMode::Overlay))
         + IntegerDefinition("PREVIEW_BLEND_SCREEN",        static_cast<int>(PreviewBlendMode::Screen))
         + IntegerDefinition("PREVIEW_BLEND_SOFT_LIGHT",    static_cast<int>(PreviewBlendMode::SoftLight))
         + IntegerDefinition("PREVIEW_BLEND_HARD_LIGHT",    static_cast<int>(PreviewBlendMode::HardLight));
}
```

`BuildBindingDefinitions()` gains one line, appended at the end:
```cpp
std::string BuildBindingDefinitions() {
    return IntegerDefinition("PREVIEW_BINDING_ENTITY_IDENTIFIERS", static_cast<int>(CompositeBinding::kEntityIdentifiers))
         + IntegerDefinition("PREVIEW_BINDING_HEIGHTFIELD",        static_cast<int>(CompositeBinding::kHeightfield))
         + IntegerDefinition("PREVIEW_BINDING_FLOW",               static_cast<int>(CompositeBinding::kFlow))
         + IntegerDefinition("PREVIEW_BINDING_ACCUMULATION",       static_cast<int>(CompositeBinding::kAccumulation))
         + IntegerDefinition("PREVIEW_BINDING_SLOPE",              static_cast<int>(CompositeBinding::kSlope))
         + IntegerDefinition("PREVIEW_BINDING_SURFACE_WEIGHTS",    static_cast<int>(CompositeBinding::kSurfaceStratumWeights))
         + IntegerDefinition("PREVIEW_BINDING_GRADIENT_TABLES",    static_cast<int>(CompositeBinding::kGradientLookupTables))
         + IntegerDefinition("PREVIEW_BINDING_ENTITY_POINTS",      static_cast<int>(CompositeBinding::kEntityPoints))
         + IntegerDefinition("PREVIEW_IMAGE_COMPOSITE",            static_cast<int>(CompositeImageUnit::kCompositeImage))
         + IntegerDefinition("PREVIEW_BINDING_CONFIGURATION",      static_cast<int>(CompositeBinding::kConfiguration))
         + IntegerDefinition("PREVIEW_BINDING_LAYERS",             static_cast<int>(CompositeBinding::kLayerConfigurations))
         + IntegerDefinition("PREVIEW_BINDING_STRATA",             static_cast<int>(CompositeBinding::kStratumConfigurations))
         + IntegerDefinition("PREVIEW_BINDING_MAP_AREAS",          static_cast<int>(CompositeBinding::kMapAreaRectangles));
}
```

## B.13 Modified: `src/ui/Application_Panels_UI.h`

The Areas row (currently lines 86-87) becomes a real `FieldLayer` target, per §14.17 item 10:
```cpp
    // ARCH_14_17_MapAreaFieldLayer.md §14.17 item 10 — Areas' `[O]` toggle now drives a real
    // composite layer instead of being one of Application_Visibility_UI.h's inert rows; visible by
    // default (the layer is seeded topmost and enabled, Application_PreviewSetup_UI.cpp).
    { ApplicationPanel::Areas,        "Areas",         ApplicationPanelGroup::Environment,
      true,  true,  PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::MapAreas },
```

## B.14 `src/ui/Application_Visibility_UI.h` — **NO EDIT** (see Interpretation Call 2)

`sangen_arch_pack/INDEX.md`'s own §14.17 paraphrase says to "update that note's 'six rows' count to
five and drop Areas from its list." **Direct read of the live file shows this instruction cannot be
followed literally: Areas was never one of the six names the SCOPE NOTE lists** (`Symmetry, Detail
Normal, Tint, Holes, Smoothness and Atmosphere`) — it was a silently-uncounted SEVENTH inert row
(`bHasVisibilityToggle=true, visibilityTarget=None`) the comment's own "six" claim already omitted.
This ticket's B.13 edit removes Areas from the inert population entirely, bringing the TRUE count
back down to six — exactly the six the comment already names, with zero text change required. Making
no edit here is the correct action, not an oversight; see Interpretation Call 2 for the full
reasoning trail.

## B.15 Modified: `src/ui/Application_PreviewSetup_UI.cpp`

`ConfigureDefaultPreview` pushes the new layer LAST, after Accumulation (§14.17 item 10 verbatim):
```cpp
    previewSettings.fieldLayers.push_back(
        MakeAutoDomainLayer(PreviewLayerKind::Flow, flowRampRow, 1.0f));
    previewSettings.fieldLayers.push_back(
        MakeAutoDomainLayer(PreviewLayerKind::Accumulation, accumulationRampRow, 1.0f));
    // ARCH §14.17 item 10 — topmost, no ramp (gradientRampIndex = -1, the same posture StratumSplat
    // uses: the color comes from the per-area tint, never a ramp lookup), Overlay blend, full opacity.
    previewSettings.fieldLayers.push_back(MakeFieldLayer(
        PreviewLayerKind::MapAreas, PreviewBlendMode::Overlay, -1, 0.0f, 1.0f, 1.0f));
    previewSettings.entityMarkRadiusPixels = 3.0f;
```

## B.16 Modified: `src/ui/Application_ViewLayersPopup_UI.cpp`

`previewLayerKindNames[]` gains an entry so the View popup's Terrain section shows a real label
instead of "Unknown" for the new layer (see Interpretation Call 3):
```cpp
const char* const previewLayerKindNames[] = {
    "HeightRamp", "StratumSplat", "Flow", "Accumulation", "Water", "Slope", "MapAreas"
};
```

## B.17 Modified: `src/ui/PreviewComposite_TestScene_UI.h`

`PreviewTestScene` gains an `areas` member (empty by default — the sentinel path handles that):
```cpp
struct PreviewTestScene {
    Params::Geometry             geometry;
    Params::Water                water;
    std::vector<Params::Stratum> strata;
    // ARCH §14.17 — empty by default; BuildMapAreaConfigurations pushes a degenerate sentinel
    // rectangle for an empty list, so leaving this untouched is a legal, no-op scene.
    std::vector<Params::MapArea> areas;
    Data::MapFields              fields;
    Data::PlacementInstances     instances;
    Data::EntityIdBuffer         entityIdentifiers;
};
```

## B.18 Mechanical: every direct `PreviewComposite(...)` constructor call gains `areas`/`scene.areas`

Confirmed by direct grep against the live tree: `PreviewComposite`'s constructor is called directly
(never through a factory) at exactly the following locations, every one in the identical shape
`..., scene.strata, scene.fields, ...` (or, for the one PARAMS-side caller,
`..., recipe.strata, assembler.Fields(), ...`). At every location below, insert the new argument
(`scene.areas` / `recipe.areas`) between the `strata` argument and the `fields` argument — no other
token in the call changes. (Application_UI.cpp's own call site is covered separately in B.19, since
its exact surrounding text is already quoted there.)

| File | Line(s) | Old | New |
| --- | --- | --- | --- |
| `AreaDragGesture_UI_Test.cpp` | 60 | `PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields, scene.instances,` | `PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields, scene.instances,` |
| `ManualInstanceHitTest_UI_Test.cpp` | 30 | `composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.fields,` | `composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,` |
| `MapCanvas_GestureOwnership_UI_Test.cpp` | 112 | `PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,` | `PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,` |
| `MapCanvas_ActivePanelGate_UI_Test.cpp` | 117 | (same shape) | (same substitution) |
| `MapCanvas_ActivePanelGate_UI_Test.cpp` | 189 | (same shape) | (same substitution) |
| `MapCanvas_IconLayer_TestFixture_UI.h` | 39 | `composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.fields,` | `composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,` |
| `MapCanvas_MarkerDrag_UI_Test.cpp` | 46 | (same `new PreviewComposite` shape) | (same substitution) |
| `MapCanvas_Picking_UI_Test.cpp` | 37 | `PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,` | (same substitution) |
| `MapCanvas_Picking_UI_Test.cpp` | 94 | (same shape) | (same substitution) |
| `MapCanvas_Render_UI_Test.cpp` | 107 | (same shape) | (same substitution) |
| `MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp` | 90 | (same shape) | (same substitution) |
| `MapCanvas_ScenarioEditMode_DrawMarkers_UI_Test.cpp` | 42 | (same `new PreviewComposite` shape) | (same substitution) |
| `MapCanvas_ScenarioEditMode_Interaction_UI_Test.cpp` | 28 | (same `new PreviewComposite` shape) | (same substitution) |
| `PreviewComposite_Wysiwyg_UI_Test.cpp` | 41, 60, 89, 117 | `Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,` | `Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,` |
| `PreviewComposite_VisibilityToggle_UI_Test.cpp` | 25 | (same `Ui::PreviewComposite` shape) | (same substitution) |
| `PreviewComposite_UI_Test.cpp` | 31, 53, 78, 99, 120, 145 | (same `Ui::PreviewComposite` shape) | (same substitution) |

(21 call sites across 15 files, all mechanical, all identical in shape — confirmed by
`grep -n "composite(scene\.geometry\|composite(recipe\.geometry"` against every `.cpp`/`.h` under
`src/ui/` returning exactly this set plus `PreviewComposite_UI.h`/`.cpp` themselves and the two files
covered in B.19.)

## B.19 Modified: `src/ui/Application_UI.cpp` and `src/ui/PreviewIntegration_TestScene_UI.h`

The two remaining direct-construction sites, both PARAMS-side (`recipe.strata`/`recipe.areas`)
rather than test-scene-side.

`Application_UI.cpp` (currently line 30, the `composite(...)` member-init):
```cpp
Application::Application(ApplicationSettings applicationSettings)
    : settings(std::move(applicationSettings)),
      recipe(MakeDefaultMapRecipe()),
      assembler(recipe),
      composite(recipe.geometry, recipe.water, recipe.strata, recipe.areas, assembler.Fields(),
                assembler.Placements().markers, entityIdentifiers),
      previewDriver(AssemblerWithDefaultStages(assembler)),
      threadPool(settings.workerThreadCount) {
```

`PreviewIntegration_TestScene_UI.h` (currently lines 29-30):
```cpp
    PreviewIntegrationScene()
        : recipe(AssemblerTest::MakeRecipe(4242u)),
          assembler(recipe),
          composite(recipe.geometry, recipe.water, recipe.strata, recipe.areas, assembler.Fields(),
                    assembler.Placements().markers, entityIdentifiers),
          driver(assembler) {
```

## B.20 New test file: `src/ui/PreviewComposite_MapAreas_UI_Test.cpp`

Cpu-only (no GL needed — mirrors `PreviewComposite_UI_Test.cpp`'s own posture). Also carries Piece
C's own suppressed-index acceptance (`TestSuppressedIndexOmitsRectangle`, at the bottom) since it is
the same composite-level surface and needs no GL either.

```cpp
// PreviewComposite_MapAreas_UI_Test.cpp — ARCH §14.17 acceptance: Params::MapArea rectangles
// compositing as a real PreviewFieldLayer (`PreviewLayerKind::MapAreas`) — an empty list paints
// nothing (the degenerate sentinel), a single area colors every cell it covers, overlapping areas
// resolve forward-iteration LAST-match-wins (the same Z rule §21.8's own body hit-test uses), and
// (§14.17 item 11) the drag-suppressed index is entirely absent from the composited input. Runs the
// Cpu twin only — no GL context needed (PreviewComposite_UI_Test.cpp's own established posture).
#include "PreviewComposite_TestScene_UI.h"

using namespace SanmapGen;

namespace {

using Ui::ChannelNear;
void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

// A fresh scene with entities cleared, so the ONE MapAreas layer this file adds is the only thing
// that can paint a pixel — no entity-mark noise, no default ramps/layers from ConfigurePreviewSettings.
void BuildBareMapAreasScene(Ui::PreviewTestScene& scene) {
    Ui::BuildPreviewTestScene(scene);
    scene.instances.Clear();
}

void ConfigureBareSettings(Ui::PreviewCompositeSettings& settings) {
    settings.previewResolution = 4;
    settings.bEntitiesEnabled  = false;
    settings.fieldLayers.push_back(
        Ui::MakeLayer(Ui::PreviewLayerKind::MapAreas, Ui::PreviewBlendMode::AlphaBlend, -1, 0.0f, 1.0f));
}

void TestEmptyAreaListPaintsNothing() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    composite.Compose();
    const unsigned int texel = composite.CompositeTexels()[0];
    check(ChannelNear(texel, 0, 0.0f) && ChannelNear(texel, 1, 0.0f) && ChannelNear(texel, 2, 0.0f),
          "an empty area list paints nothing — the clear color survives untouched");
}

void TestSingleAreaColorsCoveredCells() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Params::MapArea area;
    area.name = "Whole"; area.originX = 0.0f; area.originZ = 0.0f;
    area.width = 4.0f; area.length = 4.0f;   // the whole 4x4 map: world == cell space, worldUnitsPerCell 1.0
    scene.areas.push_back(area);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    Ui::AreaColorEntry color;
    color.name = "Whole"; color.color[0] = 1.0f; color.color[1] = 0.0f;
    color.color[2] = 0.0f; color.color[3] = 1.0f;
    composite.Settings().areaColors.push_back(color);
    composite.Compose();
    const unsigned int texel = composite.CompositeTexels()[0];
    check(ChannelNear(texel, 0, 1.0f) && ChannelNear(texel, 1, 0.0f) && ChannelNear(texel, 2, 0.0f),
          "a full-coverage area colors every cell with its own resolved color, full opacity");
}

void TestOverlapLastMatchWins() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Params::MapArea first;  first.name = "First";  first.width = 4.0f; first.length = 4.0f;
    Params::MapArea second; second.name = "Second"; second.width = 4.0f; second.length = 4.0f;
    scene.areas.push_back(first);
    scene.areas.push_back(second);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    Ui::AreaColorEntry firstColor;
    firstColor.name = "First"; firstColor.color[0] = 1.0f; firstColor.color[1] = 0.0f;
    firstColor.color[2] = 0.0f; firstColor.color[3] = 1.0f;
    Ui::AreaColorEntry secondColor;
    secondColor.name = "Second"; secondColor.color[0] = 0.0f; secondColor.color[1] = 1.0f;
    secondColor.color[2] = 0.0f; secondColor.color[3] = 1.0f;
    composite.Settings().areaColors.push_back(firstColor);
    composite.Settings().areaColors.push_back(secondColor);
    composite.Compose();
    const unsigned int texel = composite.CompositeTexels()[0];
    check(ChannelNear(texel, 0, 0.0f) && ChannelNear(texel, 1, 1.0f) && ChannelNear(texel, 2, 0.0f),
          "two fully-overlapping areas resolve to the LAST one in the vector, forward iteration");
}

// ARCH §14.17 item 11 (Piece C) — the drag-suppressed index is entirely absent from this frame's
// composited input, so the one area being dragged never double-paints against its own live canvas
// fill (MapCanvas_AreaDraw_UI.cpp's own suppressed-only fill).
void TestSuppressedIndexOmitsRectangle() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Params::MapArea area;
    area.name = "Whole"; area.width = 4.0f; area.length = 4.0f;
    scene.areas.push_back(area);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    Ui::AreaColorEntry color;
    color.name = "Whole"; color.color[0] = 1.0f; color.color[1] = 0.0f;
    color.color[2] = 0.0f; color.color[3] = 1.0f;
    composite.Settings().areaColors.push_back(color);

    composite.Settings().mapAreaSuppressedIndex = 0;
    composite.Compose();
    const unsigned int suppressedTexel = composite.CompositeTexels()[0];
    check(ChannelNear(suppressedTexel, 0, 0.0f) && ChannelNear(suppressedTexel, 1, 0.0f)
       && ChannelNear(suppressedTexel, 2, 0.0f),
          "the suppressed index's rectangle is entirely absent from the composited frame, matching "
          "the immediate-mode canvas pass drawing that ONE area meanwhile");

    // An out-of-range suppressed value suppresses nothing — the correct degradation §14.17 item 11
    // names for a list reorder racing a gesture.
    composite.Settings().mapAreaSuppressedIndex = 5;
    composite.Compose();
    const unsigned int unsuppressedTexel = composite.CompositeTexels()[0];
    check(ChannelNear(unsuppressedTexel, 0, 1.0f) && ChannelNear(unsuppressedTexel, 1, 0.0f)
       && ChannelNear(unsuppressedTexel, 2, 0.0f),
          "an out-of-range suppressed index suppresses nothing");
}

} // namespace

int main() {
    TestEmptyAreaListPaintsNothing();
    TestSingleAreaColorsCoveredCells();
    TestOverlapLastMatchWins();
    TestSuppressedIndexOmitsRectangle();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
```

---

# Piece C — drag-suppression and the amended draw pass

## C.1 Modified: `src/ui/PreviewComposite_Settings_UI.h`

New transient field, appended right after `areaColors` (added in A.3):
```cpp
    std::vector<AreaColorEntry> areaColors;

    // ARCH §14.17 item 11 — the ONE area currently mid-drag/resize/move on the canvas, omitted from
    // this frame's composited input so a live drag costs exactly two recomposites (begin+end), never
    // one per frame. Transient interaction state: NEVER serialized, and an out-of-range value
    // suppresses nothing (the safe degradation if a list reorder ever races a gesture).
    int mapAreaSuppressedIndex = -1;
};
```
(If Piece B already landed this field per B.8's own fallback note, this edit is a no-op — confirm
the field exists with this exact name/default/comment rather than adding a second one.)

## C.2 `src/ui/PreviewComposite_Prepare_UI.cpp` — already correct

`BuildMapAreaConfigurations()` (B.8) already reads `settings.mapAreaSuppressedIndex` and skips it —
no further edit needed here in Piece C.

## C.3 Modified: `src/ui/MapCanvas_ManualDragSources_UI.h`

`ManualAreaDragSources_UI` gains the fifth mutable pointer:
```cpp
// ARCH §21.8 — the Area canvas gesture's own injected-pointer bundle. `recipe.areas` is a flat
// vector with no group/transform/lock shape at all (§21.8 correction 1), so this does NOT mirror
// ManualPropDragSources_UI/ManualDecalDragSources_UI's own `InstanceDragGestureState` — it carries
// the standalone AreaDragGestureState instead.
struct ManualAreaDragSources_UI {
    std::vector<Params::MapArea>* areas             = nullptr;   // mutable: canvas creates/moves/resizes
    std::vector<AreaColorEntry>*  areaColors         = nullptr;   // mutable: ResolveAreaColor lazily
                                                                    // appends a default entry for a
                                                                    // freshly canvas-created area
    const bool*                   bAreasLocked       = nullptr;   // read-only: canvas never writes the lock
    int*                          selectedAreaIndex  = nullptr;   // mutable: auto-select-on-touch/deselect
    // ARCH §14.17 item 11 — mutable: the canvas sets/clears this to omit the dragged area from the
    // composite input for the duration of a gesture. Points at
    // `PreviewCompositeSettings::mapAreaSuppressedIndex` — one source of truth, never a second copy.
    int*                          mapAreaSuppressedIndex = nullptr;
    AreaDragGestureState           state;
};
```

## C.4 Modified: `src/ui/MapCanvas_UI.h`

`SetManualAreaDragSource` gains the fifth parameter; a new `SetAreaCompositeRefreshCallback` setter
is added beside it (mirroring `SetSelectionChangedCallback`'s own injection shape verbatim); a new
private helper and a new private `std::function<void()>` member are added.

Public setter (currently lines 155-163):
```cpp
    // ARCH §21.8 — mirrors SetManualPropDragSource's shape minus Geometry/globalSymmetryRecipe (Areas
    // carry no symmetry/layer/lock concept of their own, §21.8 correction 1/3). `areas`/`areaColors`/
    // `selectedAreaIndex` are the only mutable pointers; `areasLocked` is read-only — the canvas never
    // writes the tab-wide lock.
    // ARCH §14.17 item 11 — a fifth parameter, `mapAreaSuppressedIndex`, lets the canvas set/clear the
    // composite's transient drag-suppression slot WITHOUT reaching through the canvas's own `const
    // PreviewComposite* composite` (deliberately const — the canvas never composites, see below).
    void SetManualAreaDragSource(std::vector<Params::MapArea>* areas, std::vector<AreaColorEntry>* areaColors,
                                  const bool* areasLocked, int* selectedAreaIndex,
                                  int* mapAreaSuppressedIndex) {
        manualAreaDrag.areas = areas; manualAreaDrag.areaColors = areaColors;
        manualAreaDrag.bAreasLocked = areasLocked; manualAreaDrag.selectedAreaIndex = selectedAreaIndex;
        manualAreaDrag.mapAreaSuppressedIndex = mapAreaSuppressedIndex;
    }
    // ARCH §14.17 item 11 — the drag-suppression recomposite request, mirroring
    // SetSelectionChangedCallback's own injection shape verbatim (this header's established pattern).
    // Unset = no refresh, never a crash. Fired exactly twice per drag/resize/move gesture (begin +
    // end) and once per create-by-drag — never once per ContinueAreaDrag frame.
    void SetAreaCompositeRefreshCallback(std::function<void()> refreshCallback) {
        areaCompositeRefreshCallback = std::move(refreshCallback);
    }
```

Private method declarations — one new line, added directly after `DrawAreaOverlayPass`'s own
declaration (currently the last line of the private-method block, per STEP210):
```cpp
    void DrawAreaOverlayPass(float regionOriginX, float regionOriginY);        // MapCanvas_AreaDraw_UI.cpp
    // ARCH §14.17 item 11 — writes `*manualAreaDrag.mapAreaSuppressedIndex` null-safely and fires
    // `areaCompositeRefreshCallback` only when the value actually changed, so TryBeginAreaDrag/
    // EndAreaDrag never hold two copies of that "did it actually change" condition.
    void SetMapAreaSuppression(int areaIndex);                                 // MapCanvas_AreaDragDispatch_UI.cpp
```

Private data — one new field, added directly after `manualAreaDrag`/`bAreaDragActive` (currently the
last two lines of the class):
```cpp
    ManualAreaDragSources_UI manualAreaDrag;
    bool                     bAreaDragActive = false;
    // ARCH §14.17 item 11 — the recomposite-request callback (see SetAreaCompositeRefreshCallback).
    std::function<void()>    areaCompositeRefreshCallback;
};
```

## C.5 Modified: `src/ui/MapCanvas_AreaDragDispatch_UI.cpp`

`TryBeginAreaDrag`/`EndAreaDrag`/`CreateAreaFromDrag` gain the suppression/refresh side effects; a
new `SetMapAreaSuppression` definition is added. Full updated file:

```cpp
// MapCanvas_AreaDragDispatch_UI.cpp — MapCanvas::AreaGestureEligible/TryBeginAreaDrag/
// ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag (ARCH §21.8), plus SetMapAreaSuppression
// (ARCH §14.17 item 11's exactly-two-recomposites-per-gesture rule). Standalone sibling of
// MapCanvas_ManualDragDispatch_UI.cpp's 3-way Markers/Props/Decals dispatcher — Areas has no
// group/transform/lock shape to fit into that dispatcher's switch (§21.8 correction 1/5).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // NextAreaName, MakeNamesUnique (via UniqueNameList_UI.h)
#include "PreviewComposite_UI.h"
#include <algorithm>
#include <cmath>

namespace SanmapGen {
namespace Ui {

bool MapCanvas::AreaGestureEligible() const {
    return activePanelSource != nullptr && *activePanelSource == ApplicationPanel::Areas
        && manualAreaDrag.bAreasLocked != nullptr && !*manualAreaDrag.bAreasLocked;
}

// ARCH §14.17 item 11 — the ONE place the "did the suppressed index actually change" condition is
// evaluated, shared by every call site below (an index, never a `bEnabled` toggle: flipping
// `fieldLayers[i].bEnabled` for a drag's duration would clobber the user's own View-popup/left-column
// enable state, which this dedicated slot cannot collide with).
void MapCanvas::SetMapAreaSuppression(int areaIndex) {
    if (manualAreaDrag.mapAreaSuppressedIndex == nullptr) return;
    if (*manualAreaDrag.mapAreaSuppressedIndex == areaIndex) return;
    *manualAreaDrag.mapAreaSuppressedIndex = areaIndex;
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

bool MapCanvas::TryBeginAreaDrag(float regionLocalX, float regionLocalY) {
    bAreaDragActive = false;
    if (!AreaGestureEligible()) return false;
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return false;
    std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;

    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));

    // Step 1 — a selection exists: hit-test THAT one area's own 8 handles + body first.
    if (manualAreaDrag.selectedAreaIndex != nullptr) {
        const int selectedIndex = *manualAreaDrag.selectedAreaIndex;
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())) {
            const AreaHandle_UI handle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                             *composite, view, regionLocalX, regionLocalY);
            if (handle != AreaHandle_UI::None) {
                bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, selectedIndex, handle,
                                                       worldPoint.worldX, worldPoint.worldZ);
                // ARCH §14.17 item 11 — the FIRST of exactly two recomposites this gesture will cost.
                if (bAreaDragActive) SetMapAreaSuppression(selectedIndex);
                return bAreaDragActive;
            }
        }
    }

    // Step 2 — a miss on the selected area's own handles/body: body hit-test over EVERY area,
    // forward iteration, last match wins (later-in-vector is drawn topmost, Widget_AreaEditor.cpp's
    // own "reverse Z-order" comment, Widget_AreaEditor.cpp:50).
    int hitIndex = -1;
    for (int index = 0; index < static_cast<int>(areas.size()); ++index)
        if (IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ))
            hitIndex = index;
    if (hitIndex < 0) return false;   // total miss — no state recorded; release resolves click/create

    if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = hitIndex;
    bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, hitIndex, AreaHandle_UI::Center,
                                           worldPoint.worldX, worldPoint.worldZ);
    if (bAreaDragActive) SetMapAreaSuppression(hitIndex);
    return bAreaDragActive;
}

void MapCanvas::ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld) {
    if (!bAreaDragActive || manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    // ARCH §21.8 correction 4 / §14.17 item 11 — writes recipe.areas LIVE every frame and requests
    // ZERO recomposites: the live visual is the bespoke immediate-mode pass in
    // MapCanvas_AreaDraw_UI.cpp, not a GPU recompose.
    UpdateAreaDragGesture(manualAreaDrag.state, *manualAreaDrag.areas, worldPoint.worldX, worldPoint.worldZ,
                          bShiftHeld, bCtrlHeld);
}

void MapCanvas::EndAreaDrag() {
    EndAreaDragGesture(manualAreaDrag.state);
    bAreaDragActive = false;
    // ARCH §14.17 item 11 — the SECOND of exactly two recomposites: the area rejoins the composite
    // input now that the gesture is over.
    SetMapAreaSuppression(-1);
}

void MapCanvas::CreateAreaFromDrag(float pressRegionLocalX, float pressRegionLocalY,
                                   float releaseRegionLocalX, float releaseRegionLocalY) {
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate pressPixel = view.ResolvePreviewPixel(pressRegionLocalX, pressRegionLocalY);
    const PreviewPixelCoordinate releasePixel = view.ResolvePreviewPixel(releaseRegionLocalX, releaseRegionLocalY);
    const PreviewComposite::PreviewWorldPoint pressWorld = composite->PreviewPixelToWorld(
        static_cast<float>(pressPixel.pixelX), static_cast<float>(pressPixel.pixelY));
    const PreviewComposite::PreviewWorldPoint releaseWorld = composite->PreviewPixelToWorld(
        static_cast<float>(releasePixel.pixelX), static_cast<float>(releasePixel.pixelY));

    Params::MapArea area;
    area.originX = std::min(pressWorld.worldX, releaseWorld.worldX);
    area.originZ = std::min(pressWorld.worldZ, releaseWorld.worldZ);
    area.width   = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldX - pressWorld.worldX));
    area.length  = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldZ - pressWorld.worldZ));
    area.name = NextAreaName(static_cast<int>(manualAreaDrag.areas->size()));   // AreasTab_List_UI.h:62,
                                                                                 // the SAME helper "Add New Area" uses
    manualAreaDrag.areas->push_back(area);
    MakeNamesUnique(*manualAreaDrag.areas);   // called HERE, not left for DrawAreasTab's end-of-frame
                                               // call — see ARCH §21.8's own "Create-by-drag" section
    if (manualAreaDrag.selectedAreaIndex != nullptr)
        *manualAreaDrag.selectedAreaIndex = static_cast<int>(manualAreaDrag.areas->size()) - 1;
    // ARCH §14.17 item 11 — a brand-new area must appear: one recomposite, no suppression change.
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

} // namespace Ui
} // namespace SanmapGen
```

## C.6 Modified: `src/ui/MapCanvas_AreaDraw_UI.cpp`

`DrawAreaOverlayPass`'s amended contract per §21.8's 2026-08-29 amendment / §14.17 item 12: draws
the **fill** only for the suppressed area; the **border** only when (a) the layer is enabled AND (b)
the area is suppressed AND (c) it is selected — never at all while the layer is disabled. **The 8
handle circles and the hover-only cursor-shape feedback are UNCHANGED from STEP210** — same
selected-area-only gate, same fresh re-hit-test, nothing here touches them beyond moving them below
the rewritten fill/border block.

```cpp
// MapCanvas_AreaDraw_UI.cpp — MapCanvas::DrawAreaOverlayPass. AMENDED per
// ARCH_21_08_AreaCanvasGesture.md's 2026-08-29 amendment / ARCH_14_17_MapAreaFieldLayer.md §14.17
// item 12: the FILL is now the composite's own job in the steady state (PreviewLayerKind::MapAreas)
// — this pass draws the fill ONLY for the one area currently suppressed from the composite (the one
// being dragged/resized/moved), never every area, or it would double-paint the composite's own fill
// for every non-dragged area. The BORDER draws only when the MapAreas layer is enabled AND this is
// the suppressed area AND it is selected — never at all while the layer is disabled (a disabled
// layer means "do not show me areas," and a border is showing an area). The 8 handles (selected-area
// only) and the hover-only cursor-shape feedback are UNCHANGED from STEP210 — nothing here touches
// their own rules.
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // ResolveAreaColor
#include "PreviewComposite_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// §14.17 item 12's own preferred path: read (a) through the canvas's existing `const
// PreviewComposite*` — it needs no new plumbing at all, since `Settings()` already has a const
// overload. A plain linear scan (not `PreviewFieldLayerOfKind`, which has no const overload) since
// this is the one place a const settings reference is available.
bool IsMapAreasLayerEnabled(const PreviewCompositeSettings& settings) {
    for (const PreviewFieldLayer& layer : settings.fieldLayers)
        if (layer.kind == PreviewLayerKind::MapAreas) return layer.bEnabled;
    return false;
}

} // namespace

void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int selectedIndex = manualAreaDrag.selectedAreaIndex != nullptr ? *manualAreaDrag.selectedAreaIndex : -1;
    const int suppressedIndex = manualAreaDrag.mapAreaSuppressedIndex != nullptr
                              ? *manualAreaDrag.mapAreaSuppressedIndex : -1;

    auto ToScreen = [&](float worldX, float worldZ) {
        const PreviewComposite::PreviewPixelPoint pixel = composite->WorldToPreviewPixel(worldX, worldZ);
        const RegionLocalPoint local = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
        return ImVec2(regionOriginX + local.regionLocalX, regionOriginY + local.regionLocalY);
    };

    // ARCH §14.17 item 12 — fill + (conditional) border, ONLY for the suppressed area.
    if (suppressedIndex >= 0 && suppressedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(suppressedIndex)];
        const ImVec2 nwScreen = ToScreen(area.originX, area.originZ);
        const ImVec2 seScreen = ToScreen(area.originX + area.width, area.originZ + area.length);

        const float* const color = manualAreaDrag.areaColors != nullptr
            ? ResolveAreaColor(*manualAreaDrag.areaColors, area.name) : nullptr;
        const ImU32 fillColor = color != nullptr
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
            : ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.35f));
        drawList->AddRectFilled(nwScreen, seScreen, fillColor);

        if (IsMapAreasLayerEnabled(composite->Settings()) && suppressedIndex == selectedIndex)
            drawList->AddRect(nwScreen, seScreen, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
    }

    // The 8 handles — selected-area-only, UNCHANGED from STEP210.
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(selectedIndex)];
        AreaHandleWorldPoint_UI handlePoints[8];
        ComputeAreaHandleWorldPoints(area, handlePoints);
        const ImU32 handleColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        for (const AreaHandleWorldPoint_UI& handlePoint : handlePoints)
            drawList->AddCircleFilled(ToScreen(handlePoint.worldX, handlePoint.worldZ),
                                      kAreaHandleScreenRadiusPixels, handleColor);
    }

    // Cursor-shape feedback — hover-only, re-hit-tested fresh, UNCHANGED from STEP210.
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(areas.size())) return;
    const ImGuiIO& io = ImGui::GetIO();
    const float hoverRegionLocalX = io.MousePos.x - regionOriginX;
    const float hoverRegionLocalY = io.MousePos.y - regionOriginY;
    const float regionSide = view.RegionSidePixels();
    if (hoverRegionLocalX < 0.0f || hoverRegionLocalY < 0.0f
        || hoverRegionLocalX > regionSide || hoverRegionLocalY > regionSide) return;

    const AreaHandle_UI hoveredHandle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                           *composite, view, hoverRegionLocalX, hoverRegionLocalY);
    switch (hoveredHandle) {
        case AreaHandle_UI::N: case AreaHandle_UI::S: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
        case AreaHandle_UI::E: case AreaHandle_UI::W: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
        case AreaHandle_UI::NE: case AreaHandle_UI::SW: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
        case AreaHandle_UI::NW: case AreaHandle_UI::SE: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
        case AreaHandle_UI::Center: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll); break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
```

## C.7 Modified: `src/ui/Application_UI.cpp`

`SetManualAreaDragSource`'s call site (already retargeted for `areaColors` in A.7) gains the fifth
argument:
```cpp
    // ARCH §21.8 / §14.17 item 9/11 — the Area canvas gesture's drag source: `recipe.areas`,
    // `composite.Settings().areaColors` and `composite.Settings().mapAreaSuppressedIndex` are the
    // SAME storage the tab/composite already own — one source of truth, never a second copy.
    canvas.SetManualAreaDragSource(&recipe.areas, &composite.Settings().areaColors,
                                   &tabState.areas.bAreasLocked, &tabState.areas.selectedAreaIndex,
                                   &composite.Settings().mapAreaSuppressedIndex);
```

`WireCallbacks()` gains one new binding, added beside the other `canvas.Set*Callback` calls (after
`selectProceduralMarkerInstanceCallback`'s assignment, at the end of the function):
```cpp
    selectProceduralMarkerInstanceCallback = [this](int arrayPosition, bool bCtrlHeld, bool bShiftHeld) {
        canvas.SelectProceduralMarkerInstanceByArrayPosition(arrayPosition, bCtrlHeld, bShiftHeld);
    };
    // ARCH §14.17 item 11 — the drag-suppression recomposite request: the canvas asks, PIPELINE
    // decides the tier. Mirrors the left column's own "mutate PreviewCompositeSettings then
    // NotifyParametersChanged()" precedent (Application_LeftColumn_UI.cpp) for a presentation-only
    // edit — exactly the derive-the-tier call a presentation-only mutation already makes elsewhere.
    canvas.SetAreaCompositeRefreshCallback([this] { previewDriver.NotifyParametersChanged(); });
}
```

## C.8 New test file: `src/ui/MapCanvas_AreaDragSuppression_UI_Test.cpp`

`TryBeginAreaDrag`/`ContinueAreaDrag`/`EndAreaDrag`/`CreateAreaFromDrag` are `MapCanvas`-private —
the only way to exercise them from a test is through a real `MapCanvas::Draw()` press/drag/release
sequence, so this mirrors `MapCanvas_ActivePanelGate_UI_Test.cpp`'s own GL-backed headless-imgui-frame
technique exactly, joining the SAME `MapCanvas_UI_Test` binary (not a new one).

```cpp
// MapCanvas_AreaDragSuppression_UI_Test.cpp — ARCH §14.17 item 11 acceptance: the Area canvas
// gesture's drag-performance rule — exactly two recomposite requests per drag/resize/move gesture
// (begin + end), never one per ContinueAreaDrag frame, driven through SetAreaCompositeRefreshCallback
// and the transient mapAreaSuppressedIndex slot SetManualAreaDragSource's fifth parameter injects.
// GL-backed (mirrors MapCanvas_ActivePanelGate_UI_Test.cpp's own technique exactly) because
// TryBeginAreaDrag/ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag are MapCanvas-private — the only
// way to exercise them from a test is through a real MapCanvas::Draw() press/drag/release sequence.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution = 64;
constexpr float kRegionSidePixels  = 256.0f;
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000005ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFontAtlasIdentifier));
    ImGui::NewFrame();
}

ImVec2 DrawOneFrame(MapCanvas& canvas) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(600.0f, 600.0f));
    ImGui::Begin("AreaDragSuppressionTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

ImVec2 ScreenPositionForWorld(MapCanvas& canvas, const PreviewComposite& composite,
                              float worldX, float worldZ) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

} // namespace

void RunMapCanvasAreaDragSuppressionChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    std::vector<Params::MapArea> areas;
    Params::MapArea existingArea;
    existingArea.name = "Existing";
    existingArea.originX = 1.0f; existingArea.originZ = 1.0f;
    existingArea.width = 1.0f;   existingArea.length = 1.0f;
    areas.push_back(existingArea);
    std::vector<AreaColorEntry> areaColors;
    bool bAreasLocked = false;
    int  selectedAreaIndex = -1;
    int  mapAreaSuppressedIndex = -1;
    int  refreshCount = 0;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    ApplicationPanel activePanel = ApplicationPanel::Areas;
    canvas.SetActivePanelSource(&activePanel);
    canvas.SetManualAreaDragSource(&areas, &areaColors, &bAreasLocked, &selectedAreaIndex,
                                   &mapAreaSuppressedIndex);
    canvas.SetAreaCompositeRefreshCallback([&] { ++refreshCount; });

    // --- Case 1: create-by-drag on empty canvas space fires exactly ONE refresh, no suppression ---
    const ImVec2 emptyPressPosition = ScreenPositionForWorld(canvas, composite, 3.2f, 3.2f);
    io.AddMousePosEvent(emptyPressPosition.x, emptyPressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(emptyPressPosition.x + 60.0f, emptyPressPosition.y + 60.0f);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);

    check(areas.size() == 2u, "a press-drag-release on empty canvas space creates a new area");
    check(refreshCount == 1, "create-by-drag requests exactly one recomposite");
    check(mapAreaSuppressedIndex == -1, "create-by-drag never touches the suppression slot");

    // --- Case 2: a body-move on the pre-existing area fires exactly TWO refreshes (begin + end),
    // suppressing that area's index for the WHOLE gesture, with ZERO extra refreshes while held ---
    selectedAreaIndex = 0;   // the pre-existing "Existing" area, index 0
    const int refreshCountBeforeMove = refreshCount;
    // Dead center of the 1x1 world rect — ~32 screen px from every 8px handle circle at this zoom,
    // so step 1's handle hit-test correctly misses and step 2's body/AABB test correctly hits.
    const ImVec2 bodyPressPosition = ScreenPositionForWorld(canvas, composite, 1.5f, 1.5f);
    io.AddMousePosEvent(bodyPressPosition.x, bodyPressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(mapAreaSuppressedIndex == 0, "TryBeginAreaDrag suppresses the dragged area immediately");
    check(refreshCount == refreshCountBeforeMove + 1, "the FIRST of exactly two recomposites fires at press");

    io.AddMousePosEvent(bodyPressPosition.x + 20.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(bodyPressPosition.x + 30.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(mapAreaSuppressedIndex == 0, "the suppression stays set for the whole held drag");
    check(refreshCount == refreshCountBeforeMove + 1,
          "ContinueAreaDrag requests zero recomposites, no matter how many held frames run");

    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(mapAreaSuppressedIndex == -1, "EndAreaDrag clears the suppression slot");
    check(refreshCount == refreshCountBeforeMove + 2,
          "the SECOND of exactly two recomposites fires at release — net two per gesture, not one per frame");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
```

Wire it into the shared `MapCanvas_UI_Test.cpp` (forward declaration + `main()` call, mirroring
every other Run*Checks function already there):
```cpp
// STEP113 — MapCanvas_ActivePanelGate_UI_Test.cpp.
void RunMapCanvasActivePanelGateChecks(Sys::GpuResourceManager& manager);
// Human's own bug report — MapCanvas_ActivePanelGate_UI_Test.cpp.
void RunMapCanvasClickSelectsManualMarkerChecks(Sys::GpuResourceManager& manager);
// ARCH §21.2/§21.5 — MapCanvas_GestureOwnership_UI_Test.cpp.
void RunMapCanvasGestureOwnershipChecks(Sys::GpuResourceManager& manager);
// ARCH §14.17 item 11 — MapCanvas_AreaDragSuppression_UI_Test.cpp.
void RunMapCanvasAreaDragSuppressionChecks(Sys::GpuResourceManager& manager);
} // namespace Ui
} // namespace SanmapGen

using namespace SanmapGen;

int main(int argumentCount, char** argumentValues) {
    const std::string shaderDirectory = argumentCount > 1 ? argumentValues[1] : ".";
    Ui::RunMapCanvasViewChecks();
    Ui::RunMapCanvasPickingChecks();
    Ui::RunManualMarkerSelectionChecks();
    Ui::RunProceduralMarkerListSelectionChecks();

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!GpuResourceTest::CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("GPU SKIPPED (no GL context)\n");
        return Ui::previewTestFailureCount == 0 ? 2 : 1;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    Ui::CheckPreviewExpectation(manager.Initialize(), "the Gpu resource manager initializes");
    Ui::RunMapCanvasRenderChecks(manager);
    Ui::RunMapCanvasScenarioEditModeOwnershipChecks(manager);
    Ui::RunMapCanvasActivePanelGateChecks(manager);
    Ui::RunMapCanvasClickSelectsManualMarkerChecks(manager);
    Ui::RunMapCanvasGestureOwnershipChecks(manager);
    Ui::RunMapCanvasAreaDragSuppressionChecks(manager);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
```

---

# CMakeLists.txt

New test file registration (currently line 572 area — `PreviewComposite_MapAreas_UI_Test`, Piece B),
and a new source in the existing `MapCanvas_UI_Test` target list (Piece C):
```cmake
add_sangen_test(PreviewComposite_VisibilityToggle_UI_Test src/ui/PreviewComposite_VisibilityToggle_UI_Test.cpp)
# ARCH_14_17_MapAreaFieldLayer.md §14.17 — the MapAreas field layer's own colorization/overlap/
# suppression acceptance, Cpu-only (no GL needed).
add_sangen_test(PreviewComposite_MapAreas_UI_Test src/ui/PreviewComposite_MapAreas_UI_Test.cpp)
add_sangen_test(MapCanvas_UI_Test
    src/ui/MapCanvas_UI_Test.cpp
    src/ui/MapCanvas_Render_UI_Test.cpp
    src/ui/MapCanvas_View_UI_Test.cpp
    src/ui/MapCanvas_Picking_UI_Test.cpp
    src/ui/MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp
    src/ui/MapCanvas_ActivePanelGate_UI_Test.cpp
    # ARCH §21.2/§21.5 — end-to-end pointer-state-machine coverage: right-button pans, left-button
    # never pans (drag-a-manual-instance or marquee-select instead), a marquee's lock-gate exclusion,
    # and a live Ctrl-click toggle.
    src/ui/MapCanvas_GestureOwnership_UI_Test.cpp
    # ARCH §14.17 item 11 — the Area drag-suppression / exactly-two-recomposites acceptance.
    src/ui/MapCanvas_AreaDragSuppression_UI_Test.cpp)
```

---

## ARCH rules invoked
- `ARCH_14_17_MapAreaFieldLayer.md` §14.17 items 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 — every
  algorithmic decision in Pieces A/B/C traces to one of these specific rulings.
- `ARCH_21_08_AreaCanvasGesture.md`'s 2026-08-29 amendment — the binding law for `DrawAreaOverlayPass`'s
  Piece C contract; its own unamended body (gesture/dispatch/create-by-drag/scope-gate) is confirmed
  unchanged, per the amendment's own opening line.
- Constitution §1 — UI sets PARAMS/presentation state, never simulates; every write here lands in a
  plain field on `PreviewCompositeSettings`/`recipe.areas`, never a re-decision of a placement rule.
- Constitution §3 — `BuildMapAreaConfigurations`'s cell-space flattening reuses `ReciprocalOrZero`,
  the SAME reciprocal `WorldToPreviewPixel` already computes — multiply, never divide, no second copy.
- Constitution §6 — every index (`mapAreaSuppressedIndex`, `selectedAreaIndex`) is range-checked
  before use; an out-of-range suppressed index is defined to suppress nothing, never to crash or to
  silently suppress the wrong entry.
- Constitution §8 — `kAreaColorChannelCount`/`kDefaultAreaColor` are named constants; no new hidden
  literal is introduced anywhere in this ticket.

## Explicit out-of-scope
- **No lock concept of any kind.** `AreasTabState::bAreasLocked` stays the ONLY lock this ticket
  touches (read-only, unchanged); a follow-up ticket (the re-authored STEP212) will add per-area lock
  on top of whatever tree this ticket produces. This ticket does not pre-guess that shape.
- **No edit to `AreaDragGesture_UI.h`/`.cpp`'s pure resize/move/hit-test math** (STEP210's own
  algorithm) — completely untouched.
- **No edit to `MapCanvas_ManualDragDispatch_UI.cpp`'s 3-way Markers/Props/Decals switch** — Areas
  stays its own independent sibling dispatcher, per §21.8 correction 5, unaffected by this ticket.
- **No `Params::MapArea` field, no `.sanmap` schema key, no `SanGenVersion` bump** — §14.17 item 13.
  Everything this ticket adds is presentation state in the same category `PreviewCompositeSettings`
  and the pre-existing `AreaColorEntry` table already occupy.
- **No new count field on `PreviewCompositeConfiguration`** — §14.17 item 4's own explicit ruling;
  the shader reads `.length()`, the CPU twin reads `.size()`.
- **Binding 7 stays vacant** — `kMapAreaRectangles` is binding 12, the next free index after
  `kSlope = 11`, per that header's own documented hole.
- **No rotation, no per-area icon/label editing** — not ratified anywhere in this ticket's law.
- **No UI-Optimization-Expert-owned throughput work** — the loops this ticket adds (forward scan over
  `mapAreaRectangles`, `IsMapAreasLayerEnabled`'s linear scan over `fieldLayers`) are both bounded by
  a designer-authored area count (tens, not tens of thousands) and a fixed handful of field layers;
  no batching/vertex-budget/atlas concern applies here the way it does to the §14.9 overlay passes.

## Acceptance test (end-to-end, in addition to the new unit test binaries)
1. Piece A alone: building and running `AreasTab_UI_Test` still passes `ALL PASS` (the relocated
   `AreaColorEntry`/`ResolveAreaColor` behave byte-identically); no visual change in a running shell.
2. Piece B: on a fresh `Application` construction, the Areas panel's `[O]` toggle in the left column
   starts ticked, and `PreviewFieldLayerOfKind(composite.Settings(), PreviewLayerKind::MapAreas)`
   resolves non-null and `bEnabled == true` (confirmed automatically by the existing
   `ApplicationShell_Visibility_UI_Test.cpp`'s `CheckEveryToggledFieldHasALayer`, which needs no edit
   of its own — it already iterates the whole catalogue generically).
3. A designer-drawn area (any name but `"PlayableArea"`) composites with its own resolved color,
   full-coverage, confirmed by `PreviewComposite_MapAreas_UI_Test`'s
   `TestSingleAreaColorsCoveredCells`; two overlapping areas resolve to the LAST one in the vector
   (`TestOverlapLastMatchWins`) — the same rule the canvas's own click-to-select already uses.
4. `"PlayableArea"`'s color swatch is disabled and always shows Green in the Areas tab, regardless of
   what a prior session's color table held for that name.
5. Unticking the Areas row in the left column (or the View popup's Terrain section) makes every area
   disappear from the composited image on the next frame, and its immediate-mode border never draws
   even while an area is selected and being dragged (item 12's disabled-layer clause).
6. A press-drag-release resize/move on a selected area costs exactly TWO
   `PreviewDriver::NotifyParametersChanged()` calls total (confirmed by
   `MapCanvas_AreaDragSuppression_UI_Test`), not one per held frame; a create-by-drag costs exactly
   one; `recipe.areas` itself is still written live every `ContinueAreaDrag` frame (§21.8 correction
   4, unamended).
7. During a drag, the dragged area's fill is drawn ONLY by the immediate-mode canvas pass (never
   double-painted by the composite, since it is omitted from that frame's `mapAreaRectangles`); on
   release, the composite's next recompose paints it back in at its final rectangle/color.
8. Full `SanGenV2` build stays clean; every existing test continues to pass; every new test binary
   (`PreviewComposite_MapAreas_UI_Test`, and `MapCanvas_UI_Test`'s widened suite) passes `ALL PASS`.

---

## Interpretation calls made beyond the ratified text

1. **Piece A's `kDefaultAreaColor` stays white (byte-identical to today), and Piece B is where it
   becomes green.** §14.17 item 10 rules the green default, but doesn't split it across pieces
   itself. Since the closing dispatch note frames Piece A as "pure refactor, no visual change," this
   ticket keeps that promise literally — the color VALUE changes only in Piece B, alongside the rest
   of the "Defaults" ruling it belongs to.
2. **`Application_Visibility_UI.h` gets NO edit, contrary to `sangen_arch_pack/INDEX.md`'s own
   paraphrase** ("update that note's 'six rows' count to five and drop Areas from its list").
   Direct read of the live file (not taken on the INDEX's word) shows its SCOPE NOTE already names
   exactly six panels — `Symmetry, Detail Normal, Tint, Holes, Smoothness, Atmosphere` — and Areas is
   **not** among them, even though Areas' own catalogue row today ALSO satisfies the same predicate
   (`bHasVisibilityToggle=true, visibilityTarget=None`), making the true pre-ticket count of inert
   rows **seven**, not six — Areas was a silently-uncounted seventh. This ticket's B.13 edit removes
   Areas from that population entirely, restoring the TRUE count to six — exactly the six the comment
   already names — so the comment requires zero edit, not a "six to five" edit. Flagged here rather
   than silently deviated from the routing instruction, per this codebase's own "verify, don't relay"
   discipline (mirrors `ARCH_16_08_SpawnArmyShrink.md`'s own precedent of correcting a routing note
   after a direct re-read).
3. **`previewLayerKindNames[]` in `Application_ViewLayersPopup_UI.cpp` gains a `"MapAreas"` entry** —
   not named as a required edit anywhere in the ratified text, but mechanically necessary: without it,
   the View popup's Terrain section would show "Unknown" for the new layer's row label. A one-line
   array append with no behavioral risk (no test asserts its exact size).
4. **The Piece C suppression test is GL-backed, joining `MapCanvas_UI_Test` rather than standing
   alone** — because `TryBeginAreaDrag`/`ContinueAreaDrag`/`EndAreaDrag`/`CreateAreaFromDrag` are
   `MapCanvas`-private, the only way to exercise them at all is a real `MapCanvas::Draw()` gesture
   loop, mirroring `MapCanvas_ActivePanelGate_UI_Test.cpp`'s own established technique verbatim rather
   than inventing a second one.
5. **The move-gesture test's press point (dead center of a 1×1 world rect)** is chosen to be ~32
   screen pixels from every 8px handle circle at the test's own zoom/resolution, so step 1's
   handle-hit-test reliably misses and step 2's body/AABB test reliably hits — an implementation
   detail of the test fixture, not an architectural decision.
6. **`DrawAreasTab` gains a narrow `std::vector<AreaColorEntry>&` parameter, not a wider
   `PreviewCompositeSettings&`.** `ResolveAreaColor` and the rename-retargeting loop both already
   operate on exactly this type; handing the tab the whole settings struct would be more coupling
   than the function needs, contrary to the "smallest reusable, hyper-specific unit" discipline this
   codebase already applies everywhere else (e.g. `PreviewRampOfFieldLayer` takes a `PreviewFieldLayer&`,
   not the whole settings object, where a narrower type suffices).
7. **`IsMapAreasLayerEnabled`'s plain linear scan, not a new const overload of
   `PreviewFieldLayerOfKind`.** §14.17 item 12 explicitly names the const `Settings()` path as
   "preferred, since it needs no new plumbing at all" — adding a const overload to a shared
   `TerrainOverlayTab_UI.h` helper used by several other tabs would be a wider, unrequested API change
   for a need this one file can meet with three lines of its own.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_14_17_MapAreaFieldLayer.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_21_08_AreaCanvasGesture.md`,
`D:\Projects\Sanctuary\Map Generator\sangen_arch_pack\specs\PREVIEW_COMPOSITING_SPEC.md`,
`D:\Projects\Sanctuary\Map Generator\sangen_arch_pack\INDEX.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_List_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ManualDragSources_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDragDispatch_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDraw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ActivePanelGate_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvasView_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_Panels_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_Visibility_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_PreviewSetup_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_ViewLayersPopup_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_PanelEnvironment_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\TerrainOverlayTab_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Settings_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Kernel_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Prepare_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Cpu_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Color_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_GpuBuffers_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_GpuProgram_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Sampling_UI.glsl`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.glsl`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_TestScene_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewIntegration_TestScene_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\ApplicationShell_Visibility_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\ColorSwatch_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\MapArea_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt`,
and the every `PreviewComposite(...)` direct-construction call site enumerated in B.18 (confirmed by
grep against the live tree, not assumed from any prior summary).
