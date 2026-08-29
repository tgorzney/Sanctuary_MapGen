# STEP217 — The 16-entry distinct-color palette for new map areas (`ResolveAreaColor`'s lazy-append, `kAreaPaletteStride`, `kPlayableAreaName` relocation)

**Layer:** UI. **Domain:** `AreaColorTable_UI.h` (the one funnel every area's presentation color resolves through), `AreasTab_List_UI.h`'s re-export shape, `AreasTab_UI.cpp`'s PlayableArea re-pin. **Executor:** SanGen Coder. Authored by the SanGen UI Expert, per `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` (§14.18 Part 2, items 11-16). Every file cited here was read directly against the live tree while drafting — no forward-looking/not-yet-landed prerequisites. This is ARCH §14.18's dispatchable **Piece B** — pure presentation, zero dependency on STEP216 (Piece A) or the not-yet-authored Piece C.

## Summary
A freshly-resolved, non-`"PlayableArea"` area currently defaults to a flat, hand-picked Green (`kDefaultAreaColor`). §14.18 items 11-16 rule a 16-entry hand-authored palette instead — every entry has one channel at exactly `0.00` and one at exactly `1.00` (a mid-gray channel is the `Overlay` blend's identity and would be invisible), and the green ±30° neighborhood is excluded and reserved for `PlayableArea`. Assignment is `kAreaPaletteColors[(ordinal * 7) & 15]`, where `ordinal` is the count of already-resolved non-PlayableArea entries — a bitmask, never a division (`gcd(7,16)==1`, so all 16 entries are visited before any repeat, and consecutively-created areas land at least 112.5° apart). The critical correction the ruling itself makes: **this assignment lives inside `ResolveAreaColor`'s lazy append — the ONE funnel every area reaches (tab, canvas, and imported/migrated `.sanmap` areas alike)** — not at either of the two creation call sites, because an imported area never passes through either one and would stay flat green under a creation-site rule. Neither creation call site (`AreasTab_UI.cpp`'s "Add New Area", `MapCanvas_AreaDragDispatch_UI.cpp`'s `CreateAreaFromDrag`) changes at all.

## Required reading
`ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` Part 2 (items 11-16 — read in full; item 13's worked ordinal sequence and item 16's `kDefaultAreaColor` split are both load-bearing on this ticket's exact code).

---

## 1. Modified: `src/ui/AreaColorTable_UI.h` (full file)

```cpp
// AreaColorTable_UI.h — the UI-only per-area presentation color, the 16-entry distinct-color
// palette new areas draw from, and the one pinned reserved name (`kPlayableAreaName`). Layer: UI.
// Extracted out of AreasTab_List_UI.h (ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9) so
// PreviewComposite_Settings_UI.h — the color table's new single owner
// (PreviewCompositeSettings::areaColors) — never has to depend on a TAB header
// (AreasTab_List_UI.h pulls ColorSwatch_UI.h, RtToggleWidget_UI.h, UniqueNameList_UI.h and
// MapArea_PARAMS.h, none of which the composite settings header needs). Depends on nothing but
// <string>/<vector>. ARCH_14_18_AreaLiveBlendFidelityAndPalette.md item 12 moves `kPlayableAreaName`
// down into this SAME file, for the identical reason (`ResolveAreaColor` needs to pin it without
// reaching up into a tab header), and preserves the no-new-#include rule exactly.
//
// COLOR HAS NO `_PARAMS` HOME (STEP21 ruling #4, restated by §14.17 item 13) — it is presentation
// state, kept by NAME (not vector position — position drifts under a Reorder for no reason color
// needs to care about, Constitution §6), and this is now its one home: the Areas tab, MapCanvas's
// own drag gesture, and the composite's own field-layer flattening all read/write this SAME table.
//
// ARCH §14.18 items 11-16 — new areas no longer default to a flat Green. `ResolveAreaColor`'s lazy
// append now assigns from the 16-entry `kAreaPaletteColors` below, by a fixed-stride cycle keyed on
// ordinal (the count of already-resolved non-PlayableArea entries). This is the ONE funnel every
// area reaches — the tab's swatch, the canvas, and `PreviewComposite::BuildMapAreaConfigurations`
// all call it — so created, imported, and migrated areas are all covered by this one
// implementation, never two (item 13's correction over assigning at the two creation call sites,
// which an imported `.sanmap` area never reaches).
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

enum : int { kAreaColorChannelCount = 4 };

// The one area the engine requires. Moved here from AreasTab_List_UI.h (ARCH §14.18 item 12) so
// ResolveAreaColor can pin its color without depending on a tab header. AreasTab_List_UI.h
// re-exports this name by inclusion, so `IsPlayableArea`/`EnsurePlayableArea`/every other existing
// call site keeps compiling with zero changes.
inline constexpr const char* kPlayableAreaName = "PlayableArea";

// ARCH §14.18 item 16 — the single flat `kDefaultAreaColor` no longer means one thing, so it is
// retired and split in two: the pinned reserved color, and the fill alpha every area (palette or
// Playable) still shares.
inline constexpr float kPlayableAreaColor[kAreaColorChannelCount] = { 0.0f, 1.0f, 0.0f, 0.35f };
inline constexpr float kDefaultAreaFillAlpha = 0.35f;

// ARCH §14.18 item 11 — 16 hand-authored entries, RGB only (the fill alpha is
// kDefaultAreaFillAlpha above, not per-entry). Every entry has at least one channel at exactly
// 0.00 and one at exactly 1.00: under Overlay (the MapAreas layer's default blend mode), a source
// channel of 0.5 is the blend IDENTITY (d <= 0.5 -> 2*d*0.5 = d; d > 0.5 -> 1-2(1-d)(1-0.5) = d) —
// a mid-gray area would be literally invisible, so no entry sits anywhere near gray. The ±30°
// neighbourhood of pure green (120°) is excluded and reserved for PlayableArea; the 16 hues run
// 153.75° -> 75.00° in +18.75° steps, spanning the remaining 300° of the wheel. Full saturation,
// full value. Spectrum order, for a human auditing the table — the ASSIGNMENT cycle below does
// NOT walk this in order (item 13).
inline constexpr int kAreaPaletteEntryCount = 16;
inline constexpr float kAreaPaletteColors[kAreaPaletteEntryCount][3] = {
    { 0.00f, 1.00f, 0.56f },   //  0 Spring Aqua   153.75
    { 0.00f, 1.00f, 0.88f },   //  1 Turquoise     172.50
    { 0.00f, 0.81f, 1.00f },   //  2 Cyan          191.25
    { 0.00f, 0.50f, 1.00f },   //  3 Azure         210.00
    { 0.00f, 0.19f, 1.00f },   //  4 Blue          228.75
    { 0.13f, 0.00f, 1.00f },   //  5 Indigo        247.50
    { 0.44f, 0.00f, 1.00f },   //  6 Violet        266.25
    { 0.75f, 0.00f, 1.00f },   //  7 Purple        285.00
    { 1.00f, 0.00f, 0.94f },   //  8 Magenta       303.75
    { 1.00f, 0.00f, 0.63f },   //  9 Fuchsia       322.50
    { 1.00f, 0.00f, 0.31f },   // 10 Rose          341.25
    { 1.00f, 0.00f, 0.00f },   // 11 Red             0.00
    { 1.00f, 0.31f, 0.00f },   // 12 Vermilion      18.75
    { 1.00f, 0.63f, 0.00f },   // 13 Orange         37.50
    { 1.00f, 0.94f, 0.00f },   // 14 Yellow         56.25
    { 0.75f, 1.00f, 0.00f },   // 15 Lime           75.00
};

// ARCH §14.18 item 13 — gcd(7, 16) == 1, so the cycle visits all 16 entries before repeating, and
// consecutively CREATED areas land at least 112.5° apart (versus the 18.75° a naive in-order walk
// would give) — the whole reason the stride exists. 16 (not e.g. 12) is chosen specifically so the
// wrap below is a compile-time bitmask, never a runtime division (Constitution §3).
inline constexpr int kAreaPaletteStride = 7;

// A UI-only color, keyed by area NAME (STEP21 ruling #4). The member initializer is an INERT
// placeholder (ARCH §14.18 item 16) — never a palette entry and never Green — so a construction
// path that forgot to route through ResolveAreaColor is not silently plausible.
struct AreaColorEntry {
    std::string name;
    float color[kAreaColorChannelCount] = { 0.0f, 0.0f, 0.0f, kDefaultAreaFillAlpha };
};

// Finds the color entry for `areaName`, or appends a fresh one on first touch — the same
// linear-scan idiom `NameIsTakenBefore` already uses. This is the ONE funnel every area reaches
// (the tab's swatch, the canvas, and PreviewComposite::BuildMapAreaConfigurations), so the
// palette-assignment rule lives here, once, covering created, imported, and migrated areas alike
// (ARCH §14.18 item 13).
inline float* ResolveAreaColor(std::vector<AreaColorEntry>& areaColors, const std::string& areaName) {
    for (AreaColorEntry& entry : areaColors)
        if (entry.name == areaName) return entry.color;
    AreaColorEntry entry;
    entry.name = areaName;
    if (areaName == kPlayableAreaName) {
        // The pinned reserved area consumes NO ordinal — it cannot collide with any palette entry
        // by construction (item 11's ±30° exclusion), so it needs no collision check either.
        entry.color[0] = kPlayableAreaColor[0]; entry.color[1] = kPlayableAreaColor[1];
        entry.color[2] = kPlayableAreaColor[2]; entry.color[3] = kPlayableAreaColor[3];
    } else {
        // The ordinal is DERIVED, never stored: no counter, no serialization, no new state — the
        // table's own contents answer it. A deleted area leaves its entry behind, so the next area
        // gets a FRESH ordinal rather than reusing the dead color — the desirable direction. No
        // "scan for an unused color" pass: it buys nothing before area 17 and costs a rule.
        int ordinal = 0;
        for (const AreaColorEntry& existing : areaColors)
            if (existing.name != kPlayableAreaName) ++ordinal;
        const float* const paletteColor =
            kAreaPaletteColors[(ordinal * kAreaPaletteStride) & (kAreaPaletteEntryCount - 1)];
        entry.color[0] = paletteColor[0]; entry.color[1] = paletteColor[1];
        entry.color[2] = paletteColor[2]; entry.color[3] = kDefaultAreaFillAlpha;
    }
    areaColors.push_back(entry);
    return areaColors.back().color;
}

} // namespace Ui
} // namespace SanmapGen
```

## 2. Modified: `src/ui/AreasTab_List_UI.h` (full file)

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
//
// ARCH_14_18_AreaLiveBlendFidelityAndPalette.md item 12 — `kPlayableAreaName` (formerly defined
// directly below) has ALSO moved into `AreaColorTable_UI.h`, for the same "one funnel, no upward
// dependency on a tab header" reason as the color table itself. It is re-exported here by the same
// inclusion, so `IsPlayableArea`/`EnsurePlayableArea`/every existing call site is unaffected.
//
// STEP212 — `AreaLockEntry`/`ResolveAreaLocked` (the per-area lock, replacing the retired global
// `AreasTabState::bAreasLocked`) live in the equally minimal sibling `AreaLockTable_UI.h`, included
// and re-exported here for the identical reason: every existing `#include "AreasTab_List_UI.h"`
// call site keeps compiling with zero new includes needed. UNLIKE the color table, the lock table's
// owner stays tab-side (`AreasTabState::areaLocks`) — it has no composite-side reader at all.
#pragma once
#include <string>
#include <vector>
#include "AreaColorTable_UI.h"
#include "AreaLockTable_UI.h"
#include "ColorSwatch_UI.h"
#include "RtToggleWidget_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MapArea_PARAMS.h"

namespace SanmapGen {
namespace Ui {

static_assert(kAreaColorChannelCount == kColorSwatchChannelCount,
             "AreaColorEntry's channel count must match the swatch widget's own, or DrawColorSwatch "
             "would read/write past the array ResolveAreaColor hands it.");

// `kPlayableAreaName` itself now lives in AreaColorTable_UI.h (ARCH §14.18 item 12), re-exported
// here by the #include above — nothing below needs its own copy.

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

## 3. Modified: `src/ui/AreasTab_UI.cpp`

Replace the PlayableArea re-pin block (currently lines 57-70):

Currently:
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

New:
```cpp
    // ARCH §14.17 item 10 / §14.18 item 16 — PlayableArea is always Green and non-editable: re-pin
    // its color before drawing (the swatch below is the only OTHER path that could ever set a
    // PlayableArea color) and disable the control so a designer cannot pick a different one.
    // `kDefaultAreaColor` is retired — `kPlayableAreaColor` is the pinned reserved color now that
    // ordinary areas draw from the 16-entry palette instead.
    float* const color = ResolveAreaColor(areaColors, area.name);
    if (IsPlayableArea(area)) {
        color[0] = kPlayableAreaColor[0]; color[1] = kPlayableAreaColor[1];
        color[2] = kPlayableAreaColor[2]; color[3] = kPlayableAreaColor[3];
        ImGui::BeginDisabled();
        DrawColorSwatch("Color", color, state.colorOptions, state.colorToggle);
        ImGui::EndDisabled();
    } else {
        bCommitted = DrawColorSwatch("Color", color, state.colorOptions,
                                     state.colorToggle).bCommitted || bCommitted;
    }
```

## 4. Modified: `src/ui/AreasTab_UI_Test.cpp`

Replace `RunAreaColorResolutionChecks` (currently lines 118-135):

Currently:
```cpp
// STEP21 ruling #4: color has no `_PARAMS` home, so it lives in a UI-only side table keyed by
// area NAME. `ResolveAreaColor` finds an existing entry or appends a default one.
void RunAreaColorResolutionChecks() {
    std::vector<AreaColorEntry> areaColors;
    float* const firstResolve = ResolveAreaColor(areaColors, "Base");
    Check(areaColors.size() == 1u, "the first touch of a name appends one entry");
    Check(firstResolve[0] == 0.0f && firstResolve[1] == 1.0f && firstResolve[2] == 0.0f
          && firstResolve[3] == 0.35f,
          "a fresh entry defaults to Green/0.35 (ARCH_14_17_MapAreaFieldLayer.md §14.17 item 10)");

    firstResolve[0] = 0.2f;
    float* const secondResolve = ResolveAreaColor(areaColors, "Base");
    Check(areaColors.size() == 1u, "resolving the same name again appends nothing");
    Check(secondResolve[0] == 0.2f, "and returns the SAME entry, edits intact");

    ResolveAreaColor(areaColors, "Other");
    Check(areaColors.size() == 2u, "a different name gets its own entry");
}
```

New:
```cpp
// STEP21 ruling #4 / ARCH §14.18 items 11-13: color has no `_PARAMS` home, so it lives in a
// UI-only side table keyed by area NAME. `ResolveAreaColor` finds an existing entry or appends a
// fresh one, now drawing from the 16-entry distinct-color palette (by ordinal, stride 7) instead
// of the retired flat Green default.
void RunAreaColorResolutionChecks() {
    std::vector<AreaColorEntry> areaColors;
    float* const firstResolve = ResolveAreaColor(areaColors, "Base");
    Check(areaColors.size() == 1u, "the first touch of a name appends one entry");
    // ordinal 0 -> kAreaPaletteColors[0] (Spring Aqua), alpha kDefaultAreaFillAlpha.
    Check(firstResolve[0] == kAreaPaletteColors[0][0] && firstResolve[1] == kAreaPaletteColors[0][1]
          && firstResolve[2] == kAreaPaletteColors[0][2] && firstResolve[3] == kDefaultAreaFillAlpha,
          "a fresh entry's first ordinal defaults to the palette's own entry 0, Spring Aqua "
          "(ARCH_14_18_AreaLiveBlendFidelityAndPalette.md items 11/13)");

    firstResolve[0] = 0.2f;
    float* const secondResolve = ResolveAreaColor(areaColors, "Base");
    Check(areaColors.size() == 1u, "resolving the same name again appends nothing");
    Check(secondResolve[0] == 0.2f, "and returns the SAME entry, edits intact");

    ResolveAreaColor(areaColors, "Other");
    Check(areaColors.size() == 2u, "a different name gets its own entry");
}

// ARCH §14.18 item 13 — the stride-7 assignment cycle, verified against the ruling's own worked
// sequence (ordinals 0..3 -> table indices 0, 7, 14, 5: Spring Aqua, Purple, Yellow, Indigo), and
// the "PlayableArea consumes no ordinal" correction that is the whole reason assignment lives
// inside ResolveAreaColor rather than at either creation call site.
void RunAreaPaletteAssignmentChecks() {
    std::vector<AreaColorEntry> areaColors;
    float* const first  = ResolveAreaColor(areaColors, "AreaOne");
    float* const second = ResolveAreaColor(areaColors, "AreaTwo");
    float* const third  = ResolveAreaColor(areaColors, "AreaThree");
    float* const fourth = ResolveAreaColor(areaColors, "AreaFour");
    Check(first[0] == kAreaPaletteColors[0][0] && first[1] == kAreaPaletteColors[0][1]
          && first[2] == kAreaPaletteColors[0][2], "ordinal 0 -> palette entry 0, Spring Aqua");
    Check(second[0] == kAreaPaletteColors[7][0] && second[1] == kAreaPaletteColors[7][1]
          && second[2] == kAreaPaletteColors[7][2], "ordinal 1 -> palette entry 7, Purple");
    Check(third[0] == kAreaPaletteColors[14][0] && third[1] == kAreaPaletteColors[14][1]
          && third[2] == kAreaPaletteColors[14][2], "ordinal 2 -> palette entry 14, Yellow");
    Check(fourth[0] == kAreaPaletteColors[5][0] && fourth[1] == kAreaPaletteColors[5][1]
          && fourth[2] == kAreaPaletteColors[5][2], "ordinal 3 -> palette entry 5, Indigo");

    // PlayableArea is pinned and consumes NO ordinal: resolving it between two ordinary areas must
    // not shift the next ordinary area's own assignment.
    std::vector<AreaColorEntry> withPlayable;
    ResolveAreaColor(withPlayable, "AreaOne");                 // ordinal 0 -> Spring Aqua
    float* const playable = ResolveAreaColor(withPlayable, kPlayableAreaName);
    Check(playable[0] == kPlayableAreaColor[0] && playable[1] == kPlayableAreaColor[1]
          && playable[2] == kPlayableAreaColor[2] && playable[3] == kPlayableAreaColor[3],
          "PlayableArea always resolves to the pinned reserved color, not a palette entry");
    float* const areaTwo = ResolveAreaColor(withPlayable, "AreaTwo");
    Check(areaTwo[0] == kAreaPaletteColors[7][0] && areaTwo[1] == kAreaPaletteColors[7][1]
          && areaTwo[2] == kAreaPaletteColors[7][2],
          "PlayableArea consumed no ordinal — the next ordinary area still lands on ordinal 1, Purple");
}
```

**`main()`** — add the new call (currently lines 234-247):

Currently:
```cpp
int main() {
    RunPlayableAreaChecks();
    RunSetToMapSizeChecks();
    RunUniqueNameChecks();
    RunSliderAndSelectionChecks();
    RunAreaColorResolutionChecks();
    RunColorRenameRetargetingChecks();
    RunAreaLockResolutionChecks();
    RunLockRenameRetargetingChecks();
    RunFreshAreaSizeChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

New:
```cpp
int main() {
    RunPlayableAreaChecks();
    RunSetToMapSizeChecks();
    RunUniqueNameChecks();
    RunSliderAndSelectionChecks();
    RunAreaColorResolutionChecks();
    RunAreaPaletteAssignmentChecks();
    RunColorRenameRetargetingChecks();
    RunAreaLockResolutionChecks();
    RunLockRenameRetargetingChecks();
    RunFreshAreaSizeChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

`RunColorRenameRetargetingChecks` is **unmodified** — it manually overwrites every channel of its own `ResolveAreaColor(areaColors, "Base")` result before asserting anything, so it is insensitive to what the fresh-entry default is and needs no edit.

No `CMakeLists.txt` change — no new files, and `AreasTab_UI_Test.cpp` is already registered.

---

## ARCH rules invoked
- `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` items 11-16 (Part 2) — this ticket's entire binding law: the palette values and their derivation constraints, the stride/mask assignment rule, the `ResolveAreaColor`-is-the-one-funnel correction, and the `kDefaultAreaColor` split.
- `ARCH_14_17_MapAreaFieldLayer.md` item 9 (unchanged, cited for context) — the pre-existing "no new `#include` on `AreaColorTable_UI.h`" rule, which this ticket preserves exactly while moving `kPlayableAreaName` into the same file.
- Constitution §3 — `(ordinal * kAreaPaletteStride) & (kAreaPaletteEntryCount - 1)` is a compile-time bitmask; no division appears anywhere on this path.
- Constitution §8 — every literal that matters is a named constant (`kAreaPaletteStride`, `kAreaPaletteEntryCount`, `kDefaultAreaFillAlpha`, `kPlayableAreaColor`), never a bare number at a call site.
- Constitution §6 — `AreaColorEntry::color`'s inert placeholder initializer (never a palette entry, never Green) so a path that bypasses `ResolveAreaColor` fails visibly rather than looking plausible.

## Explicit out-of-scope
- **No per-area blend mode of any kind.** Confirmed FORBIDDEN by item 15: blend mode stays a `PreviewFieldLayer` property (one per layer, the single MapAreas layer defaults `Overlay`); `PreviewMapAreaRectangle` carries geometry and color only. No field is added to that struct, and none should be — it would break the ruled 32-byte/std430-stride invariant (§14.17 item 4) and requires a pass-shape restructuring that is "not a small change; not ruled; not to be attempted opportunistically."
- **No change to either creation call site** — `AreasTab_UI.cpp`'s "Add New Area" button (`DrawAreasGlobals`) and `MapCanvas_AreaDragDispatch_UI.cpp`'s `CreateAreaFromDrag` are read-verified in this ticket's drafting to already push a name via `NextAreaName` and nothing else — no color assignment of any kind — and stay exactly that way. This is the ruling's own point: one implementation (inside `ResolveAreaColor`) covers all three area-arrival paths.
- **No new `_PARAMS` field, no `.sanmap` schema change, no `SanGenVersion` bump.** The palette and the ordinal are both pure presentation, computed at resolve time from the table's own runtime contents — nothing here serializes.
- **STEP216 (the tier-gated baked-input uploads)** — zero dependency either direction; not touched.
- **No "scan for an unused color" pass, no persisted per-area ordinal/counter.** Explicitly rejected by item 13 — the ordinal is derived from the table's live contents on every resolve, never stored.

## Acceptance test
1. Full `SanGenV2` build stays clean.
2. `AreasTab_UI_Test` — `ALL PASS`, including the two rewritten/new color functions.
3. `PreviewComposite_MapAreas_UI_Test` continues to pass **unmodified** — every `AreaColorEntry` it constructs sets all four channels explicitly before compositing, so it never observes `ResolveAreaColor`'s default at all.
4. Manual/read-level confirmation (already performed while drafting): `AreasTab_UI.cpp`'s "Add New Area" handler and `MapCanvas_AreaDragDispatch_UI.cpp`'s `CreateAreaFromDrag` are both byte-identical before and after this ticket — grep for `kDefaultAreaColor`/`kPlayableAreaColor`/`ResolveAreaColor` in both files shows zero occurrences in either (color is never touched at creation time).
5. End-to-end (traceable through the funnel, not independently testable without a live import path in this ticket's scope): an area arriving from `EnsurePlayableArea`/an imported `.sanmap`'s `recipe.areas` gets its palette color the first time ANY of the tab draw, the canvas draw, or `BuildMapAreaConfigurations` touches it — because all three call `ResolveAreaColor`, and none of them special-case "was this area just created or did it arrive from disk."

## Interpretation calls made beyond §14.18's ratified text
1. **The mask literal.** The ruling's own pseudocode writes `& 15`; this ticket writes `& (kAreaPaletteEntryCount - 1)` — a compile-time-identical expression, chosen to satisfy Constitution §8's "named constant, never a literal" discipline. Behaviorally there is no difference (still zero runtime division, still masks to the same 16 values); this is a legibility/consistency choice, not a design change.
2. **`RunAreaPaletteAssignmentChecks` is a new test function**, not literally specified by §14.18's text (the ruling gives the worked sequence as prose/a ratified fact, not as a test). Added because the ordinal/stride rule is exactly the kind of "looks right, easy to get the wrap direction wrong" logic Constitution §7 basis-tag discipline argues should be pinned by a test, not just prose.
3. **`AreasTab_List_UI.h`'s comment restructuring** (documenting the `kPlayableAreaName` move) is this ticket's own wording — item 12 states the mechanical fact of the move but not the exact comment text; the new comment mirrors that file's own pre-existing documentation style for the color-table move it already describes one paragraph up.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_14_18_AreaLiveBlendFidelityAndPalette.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_14_17_MapAreaFieldLayer.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaColorTable_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_List_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaLockTable_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDragDispatch_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDraw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Settings_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Prepare_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_MapAreas_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\work_orders\STEP210_AreaCanvasGesture_UI.md` (format/rigor template).
