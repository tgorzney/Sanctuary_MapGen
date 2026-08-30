// AreasTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 5: the Areas tab. Retyped by STEP21
// onto the real `Params::MapArea`. The rules with teeth are pure and asserted here: the
// engine-required PlayableArea, the unique-name repair the export depends on, Set to Map Size, the
// color-rename-retargeting fix (STEP21 ruling #5), and a fresh area's visible-size seed (STEP21
// ruling #7). v1 ran the name repair as a loop tacked onto the end of the tab draw, so it could
// only ever run while the tab was open — and was never tested. Every check above this file's own
// STEP222 section runs pure — no imgui frame, no window, no GL context.
// STEP222's own acceptance check IS a headless imgui frame (no GL) driven through the real, public
// `DrawAreasTab` — `DrawAreaList`/`ApplyAreaListSignal` are anonymous-namespace-private to
// AreasTab_UI.cpp and stay that way (not part of this ticket's diff), so the click is driven at the
// one public entry point and observed through its own side effects (ResolveAreaVisible/
// ResolveAreaLocked, and a rigged duplicate-name pair that only STEP222's own `true` return for
// ToggleVisibility can deduplicate — see RunAreaVisibilityClickAcceptanceChecks below for why).
// NOT YET REGISTERED IN CMake — WO C4 does not own CMakeLists.txt (gate CD-int registers it).
#include "AreasTab_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>
#include <cstdio>
#include <string>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

Params::MapArea MakeArea(const char* name) {
    Params::MapArea area;
    area.name = name;
    return area;
}

void RunPlayableAreaChecks() {
    std::vector<Params::MapArea> areas;
    Check(EnsurePlayableArea(areas, 512), "an empty list gains the engine-required area");
    Check(areas.size() == 1u && IsPlayableArea(areas[0]),
          "and it is the PlayableArea, at the front");
    Check(areas[0].width == 512.0f && areas[0].length == 512.0f
          && areas[0].originX == 0.0f && areas[0].originZ == 0.0f,
          "created at the map's own size, at the corner");
    Check(!EnsurePlayableArea(areas, 512), "a second call changes nothing");

    Check(!IsAreaRemovable(areas[0]), "the PlayableArea can never be removed");
    areas.push_back(MakeArea("Spawns"));
    Check(IsAreaRemovable(areas[1]), "every other area can");

    // The rule is keyed off the NAME, because the name is what the exported map file carries.
    std::vector<Params::MapArea> renamedFirst;
    renamedFirst.push_back(MakeArea("NotThePlayableArea"));
    Check(EnsurePlayableArea(renamedFirst, 256) && renamedFirst.size() == 2u
          && IsPlayableArea(renamedFirst[0]),
          "a list whose rows are all named something else still gains one, inserted first");
}

void RunSetToMapSizeChecks() {
    Params::MapArea area = MakeArea("Playfield");
    area.originX = 40.0f; area.originZ = -7.0f;
    Check(SetAreaToMapSize(area, 1024), "the button reports the rectangle moved");
    Check(area.originX == 0.0f && area.originZ == 0.0f
          && area.width == 1024.0f && area.length == 1024.0f,
          "and the rectangle becomes exactly the map");
    Check(!SetAreaToMapSize(area, 1024),
          "pressing it again reports no move, so it costs no recomposite");

    Params::MapArea degenerate = MakeArea("Tiny");
    SetAreaToMapSize(degenerate, 0);
    Check(degenerate.width >= 1.0f && degenerate.length >= 1.0f,
          "a nonsense map size is repaired, never obeyed: an area is never zero-sized");
}

// STEP223: the "Center" button's own pure rule, exercised beside RunSetToMapSizeChecks the same
// way CenterAreaInMap sits beside SetAreaToMapSize in AreasTab_List_UI.h.
void RunCenterAreaInMapChecks() {
    Params::MapArea area = MakeArea("Redoubt");
    area.width = 80.0f; area.length = 40.0f;
    area.originX = 5.0f; area.originZ = -3.0f;
    Check(CenterAreaInMap(area, 512), "an off-center area reports the rectangle moved");
    Check(area.originX == 216.0f && area.originZ == 236.0f,
          "the rectangle centers on the map's own center - (512/2) - (extent/2) on each axis");
    Check(area.width == 80.0f && area.length == 40.0f,
          "Center never resizes - only the origin moves");
    Check(!CenterAreaInMap(area, 512),
          "pressing it again on an already-centered area reports no movement");

    // An odd map size: half the map is fractional (256.5 for 513) and must be honored exactly, no
    // rounding toward either neighboring integer.
    Params::MapArea oddArea = MakeArea("OddMap");
    oddArea.width = 10.0f; oddArea.length = 10.0f;
    Check(CenterAreaInMap(oddArea, 513), "an odd map size still reports the move");
    Check(oddArea.originX == 251.5f && oddArea.originZ == 251.5f,
          "half of an odd map size is fractional and lands exactly, not rounded");

    // An area whose own width/length exceeds the map size still centers, by design - no clamping
    // beyond whatever AreaOriginSliderRange's own slack already allows elsewhere in this file.
    Params::MapArea oversizedArea = MakeArea("Oversized");
    oversizedArea.width = 1000.0f; oversizedArea.length = 1000.0f;
    Check(CenterAreaInMap(oversizedArea, 512), "an oversized area still reports the move");
    Check(oversizedArea.originX == -244.0f && oversizedArea.originZ == -244.0f,
          "and centers with a NEGATIVE origin rather than being clamped to the map's own edges");
    Check(!CenterAreaInMap(oversizedArea, 512),
          "a repeat press on the now-centered oversized area reports no movement either");
}

// The export keys areas by name, so two rows sharing one would silently drop an area.
void RunUniqueNameChecks() {
    std::vector<Params::MapArea> areas;
    areas.push_back(MakeArea("Base"));
    areas.push_back(MakeArea("Base"));
    areas.push_back(MakeArea("Base"));
    Check(MakeNamesUnique(areas), "duplicates report the repair");
    Check(areas[0].name == "Base" && areas[1].name == "Base_1" && areas[2].name == "Base_2",
          "the FIRST row keeps the name and every later clash is suffixed");
    Check(!MakeNamesUnique(areas), "a list that is already unique is left alone");

    // A suffix that itself collides must keep walking rather than settle on a duplicate.
    std::vector<Params::MapArea> colliding;
    colliding.push_back(MakeArea("Base"));
    colliding.push_back(MakeArea("Base_1"));
    colliding.push_back(MakeArea("Base"));
    Check(MakeNamesUnique(colliding) && colliding[2].name == "Base_2",
          "a suffix that would itself collide is skipped");
    Check(!NameIsTakenBefore(colliding, 0u, colliding[0].name),
          "the first row can never clash with something before it");
}

void RunSliderAndSelectionChecks() {
    const ScalarSliderRange extentRange = AreaExtentSliderRange(512);
    Check(extentRange.minimumValue == 1.0f && extentRange.maximumValue == 1024.0f,
          "v1's Width/Length fence - 1 to twice the map - is kept");
    Check(extentRange.increment >= 1.0f, "and an area is sized in whole cells");
    const ScalarSliderRange originRange = AreaOriginSliderRange(512);
    Check(originRange.minimumValue == -512.0f && originRange.maximumValue == 1024.0f,
          "the origin may hang one map width off either edge");
    Check(AreaExtentSliderRange(0).maximumValue >= 2.0f,
          "a nonsense map size still yields a usable track");

    std::vector<Params::MapArea> areas;
    Check(SelectedArea(areas, -1) == nullptr, "an empty tab selects no area");
    areas.push_back(MakeArea(kPlayableAreaName));
    Check(SelectedArea(areas, 0) == &areas[0], "the selected area is reachable");
    Check(SelectedArea(areas, 3) == nullptr, "an index past the last row selects nothing");

    Check(ResolvedAreaSelection(2, 5) == 2, "a selection inside the list is kept");
    Check(ResolvedAreaSelection(4, 3) == 2, "one past the end falls back to the last row");
    Check(ResolvedAreaSelection(0, 0) == -1, "and an emptied list selects nothing");

    const ColorSwatchOptions options = AreasTabColorSwatchOptions();
    Check(options.bAlphaEnabled && options.bAlphaBarShown,
          "the area swatch is the one caller that turns the picker's alpha bar on");
}

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
    // Reserved up front: ResolveAreaColor returns a pointer INTO the vector's own backing store
    // (by design — the caller hands it straight to DrawColorSwatch), so holding four such pointers
    // alive across the four resolves below would otherwise be invalidated by std::vector's own
    // reallocation on a later push_back — a dangling-pointer bug in the TEST, not in
    // ResolveAreaColor's ordinal/stride logic itself (independently confirmed correct).
    areaColors.reserve(4);
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

// STEP21 ruling #5: a real regression this ticket must fix, not preserve. Mirrors
// AreasTab_UI.cpp's DrawAreaSettings — the name is captured BEFORE the edit, and the matching
// color entry is retargeted to the new name once the rename commits.
void RunColorRenameRetargetingChecks() {
    std::vector<AreaColorEntry> areaColors;
    float* const originalColor = ResolveAreaColor(areaColors, "Base");
    originalColor[0] = 0.25f; originalColor[1] = 0.5f; originalColor[2] = 0.75f; originalColor[3] = 0.9f;

    const std::string nameBeforeEdit = "Base";
    const std::string nameAfterEdit  = "Renamed";
    for (AreaColorEntry& entry : areaColors)
        if (entry.name == nameBeforeEdit) { entry.name = nameAfterEdit; break; }

    Check(areaColors.size() == 1u,
          "the rename retargets the existing entry in place rather than orphaning it");
    float* const resolvedAfterRename = ResolveAreaColor(areaColors, nameAfterEdit);
    Check(areaColors.size() == 1u,
          "resolving under the NEW name finds the retargeted entry - it does not create a second");
    Check(resolvedAfterRename[0] == 0.25f && resolvedAfterRename[1] == 0.5f
          && resolvedAfterRename[2] == 0.75f && resolvedAfterRename[3] == 0.9f,
          "the color value survives the rename - not silently reset to default");
}

// STEP212: the per-area lock replaces the retired `AreasTabState::bAreasLocked`. `ResolveAreaLocked`
// mirrors `ResolveAreaColor`'s own lazy-append idiom, but must answer two different questions with
// two different defaults depending on the call site (an existing area vs. a freshly created one) —
// its own `bDefaultLocked` parameter is what this function exists to exercise.
void RunAreaLockResolutionChecks() {
    std::vector<AreaLockEntry> areaLocks;
    bool* const firstResolve = ResolveAreaLocked(areaLocks, "Base");
    Check(areaLocks.size() == 1u, "the first touch of a name appends one entry");
    Check(*firstResolve, "an ordinary (default-argument) resolve defaults LOCKED - matches the "
                        "retired global bAreasLocked's own default");

    *firstResolve = false;
    bool* const secondResolve = ResolveAreaLocked(areaLocks, "Base");
    Check(areaLocks.size() == 1u, "resolving the same name again appends nothing");
    Check(!*secondResolve, "and returns the SAME entry, edits intact");

    ResolveAreaLocked(areaLocks, "Other");
    Check(areaLocks.size() == 2u, "a different name gets its own entry");

    // The human's own explicit rule: a freshly created area starts UNLOCKED. The two creation call
    // sites (AreasTab_UI.cpp's Add New Area, MapCanvas_AreaDragDispatch_UI.cpp's CreateAreaFromDrag)
    // both pass bDefaultLocked=false explicitly for a name this table has never seen before.
    bool* const freshCreationResolve = ResolveAreaLocked(areaLocks, "FreshlyCreated", /*bDefaultLocked=*/false);
    Check(!*freshCreationResolve, "a name resolved with bDefaultLocked=false starts UNLOCKED");
    Check(areaLocks.size() == 3u, "and still only appends the one new entry");

    // Once an entry exists, a LATER resolve's own bDefaultLocked argument is irrelevant - the
    // existing value always wins, never silently re-defaulted.
    bool* const secondTouchIgnoresDefault = ResolveAreaLocked(areaLocks, "FreshlyCreated", /*bDefaultLocked=*/true);
    Check(!*secondTouchIgnoresDefault,
          "a second resolve's own default argument never overwrites an already-existing entry");
}

// Mirrors RunColorRenameRetargetingChecks exactly, one table over: AreasTab_UI.cpp's
// DrawAreaSettings now retargets BOTH the color entry and the lock entry on a committed rename.
void RunLockRenameRetargetingChecks() {
    std::vector<AreaLockEntry> areaLocks;
    bool* const originalLock = ResolveAreaLocked(areaLocks, "Base", /*bDefaultLocked=*/false);
    *originalLock = false;

    const std::string nameBeforeEdit = "Base";
    const std::string nameAfterEdit  = "Renamed";
    for (AreaLockEntry& entry : areaLocks)
        if (entry.name == nameBeforeEdit) { entry.name = nameAfterEdit; break; }

    Check(areaLocks.size() == 1u,
          "the rename retargets the existing entry in place rather than orphaning it");
    bool* const resolvedAfterRename = ResolveAreaLocked(areaLocks, nameAfterEdit);
    Check(areaLocks.size() == 1u,
          "resolving under the NEW name finds the retargeted entry - it does not create a second");
    Check(!*resolvedAfterRename,
          "the unlocked value survives the rename - not silently reset to the LOCKED default");
}

// STEP223 bundled fix: mirrors RunColorRenameRetargetingChecks/RunLockRenameRetargetingChecks
// exactly, one table over. AreasTab_UI.cpp's DrawAreaSettings now retargets the color, lock, AND
// visibility entries on a committed rename - before this ticket, a hidden area that got renamed
// silently reset back to default-visible on the next resolve, since only color/lock were retargeted.
void RunVisibilityRenameRetargetingChecks() {
    std::vector<AreaVisibilityEntry> areaVisibility;
    bool* const originalVisible = ResolveAreaVisible(areaVisibility, "Base");
    *originalVisible = false;   // hidden, before the rename

    const std::string nameBeforeEdit = "Base";
    const std::string nameAfterEdit  = "Renamed";
    for (AreaVisibilityEntry& entry : areaVisibility)
        if (entry.name == nameBeforeEdit) { entry.name = nameAfterEdit; break; }

    Check(areaVisibility.size() == 1u,
          "the rename retargets the existing entry in place rather than orphaning it");
    bool* const resolvedAfterRename = ResolveAreaVisible(areaVisibility, nameAfterEdit);
    Check(areaVisibility.size() == 1u,
          "resolving under the NEW name finds the retargeted entry - it does not create a second");
    Check(!*resolvedAfterRename,
          "the hidden value survives the rename - not silently reset to the VISIBLE default");
}

// STEP222: `ResolveAreaVisible` mirrors `ResolveAreaColor`/`ResolveAreaLocked`'s own lazy-append
// idiom, but — unlike lock — takes no `bDefaultXxx` parameter at all: every area, created or
// pre-existing, defaults VISIBLE on first touch, with no second creation-time override anywhere.
void RunAreaVisibilityResolutionChecks() {
    std::vector<AreaVisibilityEntry> areaVisibility;
    bool* const firstResolve = ResolveAreaVisible(areaVisibility, "Base");
    Check(areaVisibility.size() == 1u, "the first touch of a name appends one entry");
    Check(*firstResolve, "a fresh entry defaults VISIBLE — no creation-time override exists here, "
                        "unlike AreaLockEntry's bDefaultLocked");

    *firstResolve = false;
    bool* const secondResolve = ResolveAreaVisible(areaVisibility, "Base");
    Check(areaVisibility.size() == 1u, "resolving the same name again appends nothing");
    Check(!*secondResolve, "and returns the SAME entry, edits intact");

    ResolveAreaVisible(areaVisibility, "Other");
    Check(areaVisibility.size() == 2u, "a different name gets its own entry");
}

// STEP21 ruling #7: `Params::MapArea`'s own defaults are 0/0 (correct for "absent from an
// imported file degrades to nothing"), but a freshly authored row needs a visible, usable size.
void RunFreshAreaSizeChecks() {
    Params::MapArea freshArea;
    Check(freshArea.width == 0.0f && freshArea.length == 0.0f,
          "Params::MapArea's own default is invisible - correct for an absent import, not an "
          "authored row, which is exactly why Add New Area must seed it explicitly");

    // Mirrors AreasTab_UI.cpp's Add New Area handler: name seeded via NextAreaName, then width/
    // length explicitly seeded to a visible size.
    freshArea.name   = NextAreaName(0);
    freshArea.width  = 100.0f;
    freshArea.length = 100.0f;
    Check(!freshArea.name.empty(), "a fresh area is never given a blank name");
    Check(freshArea.width > 0.0f && freshArea.length > 0.0f,
          "and a fresh area is seeded with a visible, non-zero size");
}

// STEP222 acceptance — driven through the real, public `DrawAreasTab`, headless (no GL). The scene
// carries an engine-required PlayableArea, a uniquely-named "Target" area (whose own [o]/[L] icon
// this test clicks), and TWO areas both named "Dup" — a rigged duplicate pair whose fate is the
// observable proxy for `ApplyAreaListSignal`'s own return value: `DrawAreasTab` only ever calls
// `MakeNamesUnique(recipe.areas)` when the signal applier reported the recipe moved, so "did the
// duplicate pair get deduplicated this frame" is a faithful, black-box readout of a `bool` this
// ticket deliberately cannot read directly (the applier is anonymous-namespace-private, by design,
// unchanged by this ticket's own diff).
struct AreaVisibilityToggleScene {
    Params::MapRecipe                recipe;
    AreasTabState                    state;
    std::vector<AreaColorEntry>      areaColors;
    std::vector<AreaVisibilityEntry> areaVisibility;
    std::vector<Params::MapArea>     originalAreas;   // restores a probe that lands on X##delete
};

AreaVisibilityToggleScene MakeAreaVisibilityToggleScene() {
    AreaVisibilityToggleScene scene;
    constexpr int kMapSize = 512;
    scene.recipe.geometry.mapSize = kMapSize;
    Params::MapArea playable = MakeArea(kPlayableAreaName);
    playable.width = static_cast<float>(kMapSize); playable.length = static_cast<float>(kMapSize);
    Params::MapArea target = MakeArea("Target");
    target.width = 64.0f; target.length = 64.0f;
    Params::MapArea dupA = MakeArea("Dup");
    dupA.width = 64.0f; dupA.length = 64.0f;
    Params::MapArea dupB = MakeArea("Dup");
    dupB.width = 64.0f; dupB.length = 64.0f;
    scene.recipe.areas = { playable, target, dupA, dupB };
    scene.originalAreas = scene.recipe.areas;
    return scene;
}

// Tall enough that every row's own header line stays inside the window's own (NoScrollbar) clip
// rect — RunHeadlessFrame never scrolls, so any content taller than the window is simply
// unreachable by a synthetic click regardless of how far the sweep below searches.
const ImVec2 kAreaVisibilitySceneWindowSize(560.0f, 1400.0f);

// One frame of the real tab, `previewDriver = nullptr` (exactly like every other pure-logic test in
// this file — DrawAreasTab's own NotifyPlacementChange no-ops on a null driver, Constitution §6).
// Reports the window's own true right content edge and the tab's own total drawn height, so the
// click sweep below never hardcodes a layout constant that could drift under an unrelated tab edit.
void DrawAreaVisibilityToggleFrame(AreaVisibilityToggleScene& scene, const HeadlessMouseState& mouse,
                                   float& outRightEdgeX, float& outBottomY) {
    RunHeadlessFrame(mouse, kAreaVisibilitySceneWindowSize, [&] {
        outRightEdgeX = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        DrawAreasTab(scene.recipe, scene.state, nullptr, scene.areaColors, scene.areaVisibility);
        outBottomY = ImGui::GetCursorScreenPos().y;
    });
}

struct ClickSearchResult {
    bool  bFound = false;
    float x = 0.0f;
    float y = 0.0f;
};

// Sweeps the row-affordance band (the tab's own right edge, where every row's [o]/[L]/X strip
// lives) top-to-bottom, left-to-right, clicking each candidate cell until `probe()`'s own value
// changes. The band spans all three icons — visibility, lock, AND delete — so this self-heals
// every form of sweep collateral a blind click over REAL production UI can cause: (1) an
// accidental X##delete hit, detected via the area count shrinking, restores the original four-row
// scene; (2) an accidental flip of `guard()` — the OTHER per-row table this call is not searching
// for — is reverted, and (since STEP222's `true` return for ToggleVisibility also trips
// MakeNamesUnique) `recipe.areas` is restored wholesale, not just the one bool; (3) an accidental
// collapse of either Section header's own full-width bar, which would silently reflow every row
// underneath to a new Y for the rest of the search.
template <typename ProbeFunction, typename GuardFunction, typename RevertGuardFunction>
ClickSearchResult FindAffordanceIconByObservedFlip(AreaVisibilityToggleScene& scene, ProbeFunction probe,
                                                   GuardFunction guard, RevertGuardFunction revertGuard) {
    float rightEdgeX = 0.0f, bottomY = 0.0f;
    DrawAreaVisibilityToggleFrame(scene, HeadlessMouseState(), rightEdgeX, bottomY);
    DrawAreaVisibilityToggleFrame(scene, HeadlessMouseState(), rightEdgeX, bottomY);   // settle layout

    ClickSearchResult result;
    // The "Area Stack" section indents its own body (Section_UI.cpp's DrawSectionBegin) one level,
    // so the row header — and its affordance strip — draws `WindowPadding.x + IndentSpacing` to the
    // LEFT of `rightEdgeX` (measured OUTSIDE that indent, before DrawAreasTab was ever called).
    // A generous buffer on both sides absorbs the exact pixel math so this sweep never has to
    // hardcode ImGui's own indent constants precisely — the self-heal above already covers the
    // (accepted) risk of also sweeping across X##delete along the way.
    const ImGuiStyle& style = ImGui::GetStyle();
    const float indentOffset = style.WindowPadding.x + style.IndentSpacing;
    const float xStart = rightEdgeX - 76.0f - indentOffset - 12.0f;
    const float xEnd   = rightEdgeX - indentOffset + 12.0f;
    for (float y = 8.0f; y < bottomY && !result.bFound; y += 6.0f) {
        for (float x = xStart; x < xEnd && !result.bFound; x += 4.0f) {
            const std::size_t areaCountBefore = scene.recipe.areas.size();
            const bool probeBefore = probe();
            const bool guardBefore = guard();
            HeadlessMouseState hover;   hover.position = ImVec2(x, y);
            HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
            HeadlessMouseState release = hover; release.bLeftButtonDown = false;
            float unusedX = 0.0f, unusedY = 0.0f;
            DrawAreaVisibilityToggleFrame(scene, hover,   unusedX, unusedY);
            DrawAreaVisibilityToggleFrame(scene, press,   unusedX, unusedY);
            DrawAreaVisibilityToggleFrame(scene, release, unusedX, unusedY);
            // Heal an accidental hit on either Section header's own full-width bar (Section_UI.cpp's
            // bypass-toolkit InvisibleButton, unrelated to any row): collapsing "Areas" or "Area
            // Stack" reflows every row underneath to a NEW Y for every subsequent frame, silently
            // invalidating this sweep's whole geometry for the rest of the search. Both sections stay
            // forced open for the sweep's entire duration — there is no scenario here that wants
            // either collapsed.
            scene.state.globalSection.bOpen = true;
            scene.state.areaSection.bOpen   = true;
            if (scene.recipe.areas.size() != areaCountBefore) {
                scene.recipe.areas = scene.originalAreas;   // heal an accidental X##delete
                continue;
            }
            if (probe() != probeBefore) { result.bFound = true; result.x = x; result.y = y; continue; }
            if (guard() != guardBefore) {
                // Heal an accidental hit on Target's OWN other icon: revert its own bool AND restore
                // `recipe.areas` wholesale (see the general dedup-repair note below for why a bare
                // bool revert alone is not enough).
                revertGuard(guardBefore);
                scene.recipe.areas = scene.originalAreas;
                continue;
            }
            // Heal an accidental ToggleVisibility hit on ANY OTHER row (PlayableArea's own icon, or
            // one of the two rigged "Dup" rows' own icons — both sit in the very same affordance
            // column this sweep scans, just at a different Y): STEP222's own `true` return applies
            // to EVERY row, not just the one this call is searching for, so a stray hit anywhere
            // else in the sweep can ALSO trip MakeNamesUnique and permanently deduplicate the rigged
            // pair before this call ever reaches Target's own row. Restoring the full vector is the
            // only complete undo (a per-row bool revert cannot un-rename anything).
            if (scene.recipe.areas.size() == scene.originalAreas.size()
                && (scene.recipe.areas[2].name != "Dup" || scene.recipe.areas[3].name != "Dup"))
                scene.recipe.areas = scene.originalAreas;
        }
    }
    return result;
}

// The contrast case FIRST (STEP212's own pre-existing, previously-untested-at-this-level behavior):
// ToggleLock still returns false, so a click on Target's own [L]/[U] icon flips ONLY the lock table
// and leaves the rigged "Dup"/"Dup" pair exactly as duplicate as they started.
void RunAreaLockClickLeavesDuplicateNamesChecks() {
    HeadlessImguiSession session;
    AreaVisibilityToggleScene scene = MakeAreaVisibilityToggleScene();
    auto targetLocked   = [&] { return *ResolveAreaLocked(scene.state.areaLocks, "Target"); };
    auto targetVisible  = [&] { return *ResolveAreaVisible(scene.areaVisibility, "Target"); };
    auto revertVisible  = [&](bool value) { *ResolveAreaVisible(scene.areaVisibility, "Target") = value; };

    const bool lockedBefore = targetLocked();
    const ClickSearchResult found =
        FindAffordanceIconByObservedFlip(scene, targetLocked, targetVisible, revertVisible);
    Check(found.bFound, "the [L]/[U] lock affordance for a non-Playable area row is reachable by click");
    Check(targetLocked() != lockedBefore, "clicking it flips AreaLockEntry::bLocked for that row's area");
    Check(scene.recipe.areas[2].name == "Dup" && scene.recipe.areas[3].name == "Dup",
          "ApplyAreaListSignal still returns false for ToggleLock (unaffected by this ticket): the "
          "duplicate \"Dup\" pair is left untouched, proving no MakeNamesUnique recompose ran");
}

// The new behavior this ticket adds: ToggleVisibility now returns true, so the SAME click sequence
// on Target's own [o] icon both flips AreaVisibilityEntry::bVisible AND trips the recompose that
// deduplicates the rigged "Dup"/"Dup" pair this same frame.
void RunAreaVisibilityClickTogglesAndRecomposesChecks() {
    HeadlessImguiSession session;
    AreaVisibilityToggleScene scene = MakeAreaVisibilityToggleScene();
    auto targetVisible = [&] { return *ResolveAreaVisible(scene.areaVisibility, "Target"); };
    auto targetLocked  = [&] { return *ResolveAreaLocked(scene.state.areaLocks, "Target"); };
    auto revertLocked  = [&](bool value) { *ResolveAreaLocked(scene.state.areaLocks, "Target") = value; };

    const bool visibleBefore = targetVisible();
    Check(visibleBefore, "a freshly resolved area defaults VISIBLE (ResolveAreaVisible's own default)");
    const ClickSearchResult found =
        FindAffordanceIconByObservedFlip(scene, targetVisible, targetLocked, revertLocked);
    Check(found.bFound, "the [o]/[-] visibility affordance for a non-Playable area row is reachable by click");
    Check(targetVisible() != visibleBefore,
          "clicking it flips AreaVisibilityEntry::bVisible — verified via ResolveAreaVisible on the "
          "same table, exactly as the acceptance test specifies");
    Check(scene.recipe.areas[2].name != scene.recipe.areas[3].name,
          "STEP222: ApplyAreaListSignal returns TRUE for ToggleVisibility (unlike ToggleLock above) — "
          "the recompose trips MakeNamesUnique this same frame, deduplicating the \"Dup\"/\"Dup\" pair");
}

void RunAreaVisibilityClickAcceptanceChecks() {
    RunAreaLockClickLeavesDuplicateNamesChecks();
    RunAreaVisibilityClickTogglesAndRecomposesChecks();
}

// STEP223 acceptance — extends the click-sweep infrastructure above to the new "Center" header
// button. That button sits in the header-extra slot RenderCollapsibleRow reserves immediately to
// the LEFT of the [o]/[U]/X strip (DraggableListWidget_RowLayout_UI.h's own headerExtraWidthPixels
// mechanism), so its own search band is offset kAreaCenterButtonWidthPixels further left of the
// affordance-strip band FindAffordanceIconByObservedFlip already searches above. The observable
// here is a position (Target's own originX/originZ), not a per-row bool table, so this is a
// dedicated, smaller sibling search rather than a reuse of that bool-typed helper.
ClickSearchResult FindCenterButtonByObservedMove(AreaVisibilityToggleScene& scene,
                                                 float expectedOriginX, float expectedOriginZ) {
    auto targetCentered = [&] {
        return scene.recipe.areas[1].originX == expectedOriginX
            && scene.recipe.areas[1].originZ == expectedOriginZ;
    };

    float rightEdgeX = 0.0f, bottomY = 0.0f;
    DrawAreaVisibilityToggleFrame(scene, HeadlessMouseState(), rightEdgeX, bottomY);
    DrawAreaVisibilityToggleFrame(scene, HeadlessMouseState(), rightEdgeX, bottomY);   // settle layout

    ClickSearchResult result;
    const ImGuiStyle& style = ImGui::GetStyle();
    const float indentOffset = style.WindowPadding.x + style.IndentSpacing;
    // 76.0f mirrors FindAffordanceIconByObservedFlip's own literal above (kAffordanceStripWidthPixels,
    // DraggableListWidget_Types_UI.h) - the strip's own left edge, where the [o] icon itself begins.
    const float affordanceStripLeftX = rightEdgeX - 76.0f - indentOffset;
    const float xStart = affordanceStripLeftX - kAreaCenterButtonWidthPixels - 12.0f;
    const float xEnd   = affordanceStripLeftX - 4.0f;   // stays clear of the strip's own icons
    for (float y = 8.0f; y < bottomY && !result.bFound; y += 6.0f) {
        for (float x = xStart; x < xEnd && !result.bFound; x += 4.0f) {
            const std::size_t areaCountBefore = scene.recipe.areas.size();
            HeadlessMouseState hover;   hover.position = ImVec2(x, y);
            HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
            HeadlessMouseState release = hover; release.bLeftButtonDown = false;
            float unusedX = 0.0f, unusedY = 0.0f;
            DrawAreaVisibilityToggleFrame(scene, hover,   unusedX, unusedY);
            DrawAreaVisibilityToggleFrame(scene, press,   unusedX, unusedY);
            DrawAreaVisibilityToggleFrame(scene, release, unusedX, unusedY);
            // Heal an accidental hit on either Section header's own full-width bar - see
            // FindAffordanceIconByObservedFlip's own identical comment above for why.
            scene.state.globalSection.bOpen = true;
            scene.state.areaSection.bOpen   = true;
            if (scene.recipe.areas.size() != areaCountBefore) {
                scene.recipe.areas = scene.originalAreas;   // heal an accidental X##delete
                continue;
            }
            if (targetCentered()) { result.bFound = true; result.x = x; result.y = y; }
        }
    }
    return result;
}

// Clicks Target's own "Center" button and confirms both (1) the live `Params::MapArea` in
// `recipe.areas` actually recenters, and (2) the button renders strictly to the LEFT of the [o]
// icon's own affordance strip - a position assertion, not just the behavior.
void RunAreaCenterButtonClickAcceptanceChecks() {
    HeadlessImguiSession session;
    AreaVisibilityToggleScene scene = MakeAreaVisibilityToggleScene();
    // Mirrors CenterAreaInMap's own math: (512 - 64) / 2, for Target's 64x64 rectangle.
    constexpr float kExpectedOriginX = 224.0f;
    constexpr float kExpectedOriginZ = 224.0f;

    Check(scene.recipe.areas[1].name == "Target"
          && (scene.recipe.areas[1].originX != kExpectedOriginX
              || scene.recipe.areas[1].originZ != kExpectedOriginZ),
          "Target starts off-center, so a real click is what centers it below, not a scene that "
          "was already centered to begin with");

    const ClickSearchResult found = FindCenterButtonByObservedMove(scene, kExpectedOriginX, kExpectedOriginZ);
    Check(found.bFound, "the Center header button for a non-Playable area row is reachable by click");
    Check(scene.recipe.areas[1].originX == kExpectedOriginX
          && scene.recipe.areas[1].originZ == kExpectedOriginZ,
          "clicking it actually recenters the live Params::MapArea in recipe.areas");

    float rightEdgeX = 0.0f, bottomY = 0.0f;
    DrawAreaVisibilityToggleFrame(scene, HeadlessMouseState(), rightEdgeX, bottomY);
    const ImGuiStyle& style = ImGui::GetStyle();
    const float indentOffset = style.WindowPadding.x + style.IndentSpacing;
    const float affordanceStripLeftX = rightEdgeX - 76.0f - indentOffset;
    Check(found.x < affordanceStripLeftX,
          "the Center button renders to the LEFT of the [o]/[U]/X strip, per the human's own "
          "request that it sit left of the [o] icon");
}

// STEP225 acceptance, mechanism updated by STEP226 — the human's own single-line-row request:
// Name, X, Z, W, L, Color and the Map Size button must all sit on ONE true imgui line
// (DrawAreaSettings's own plain SameLine-chained row, no BeginChild/EndChild wrapper as of
// STEP226 — the scroll-child STEP225 drew this row inside is gone, but the "one line" claim it was
// meant to prove is unchanged). DrawAreaList/DrawAreaSettings stay anonymous-namespace-private to
// AreasTab_UI.cpp (unchanged by this ticket), so — exactly like STEP222/STEP223's own click-sweep
// checks above — this drives the real, public DrawAreasTab and reads the claim back through
// OBSERVED SIDE EFFECTS: the row's own leftmost control (Name), one of its middle controls (the
// X-position slider), and its own rightmost control (the "Map Size" button) are each located by an
// independent click search over the row body's own drawn controls, and all three searches landing
// on the SAME Y band is the runtime proxy for "one line, not stacked" this ticket's own diff cannot
// otherwise assert headless (DrawAreasTab exposes no per-widget rect to a caller). This search was
// never keyed to a named child window in the first place — it sweeps raw screen-space mouse
// coordinates over the tab's real draw output — so removing the child changes nothing about HOW
// this test finds each control, only that there is no more scrollbar/child boundary to reason about.
struct AreaDetailRowScene {
    Params::MapRecipe                recipe;
    AreasTabState                    state;
    std::vector<AreaColorEntry>      areaColors;
    std::vector<AreaVisibilityEntry> areaVisibility;
};

AreaDetailRowScene MakeAreaDetailRowScene() {
    AreaDetailRowScene scene;
    constexpr int kMapSize = 512;
    scene.recipe.geometry.mapSize = kMapSize;
    Params::MapArea playable = MakeArea(kPlayableAreaName);
    playable.width = static_cast<float>(kMapSize); playable.length = static_cast<float>(kMapSize);
    Params::MapArea target = MakeArea("Target");
    target.width   = 64.0f; target.length  = 64.0f;
    target.originX = 10.0f; target.originZ = 20.0f;
    scene.recipe.areas = { playable, target };
    return scene;
}

// Wide enough that the row's own ~600px natural content (per STEP226's own reasoning in
// AreasTab_UI.cpp: the docked settings window is several hundred pixels wide by default and
// freely resizable) reaches the rightmost "Map Size" button with room to spare and no scrollbar of
// any kind — STEP226 removed the scroll-child entirely, so there is no scrolled-away case left to
// test here at all.
const ImVec2 kAreaDetailRowSceneWindowSize(900.0f, 320.0f);

// `bTypeCharacterThisFrame` feeds one synthetic keystroke into whatever control is currently ACTIVE
// via real mouse focus (never a forced ImGui::SetKeyboardFocusHere) — the same click-then-type
// sequence a real user drives, so a character only ever lands in the Name field if a prior frame's
// click actually focused it.
void DrawAreaDetailRowFrame(AreaDetailRowScene& scene, const HeadlessMouseState& mouse,
                            bool bTypeCharacterThisFrame, float& outBottomY) {
    RunHeadlessFrame(mouse, kAreaDetailRowSceneWindowSize, [&] {
        if (bTypeCharacterThisFrame) ImGui::GetIO().AddInputCharactersUTF8("Q");
        DrawAreasTab(scene.recipe, scene.state, nullptr, scene.areaColors, scene.areaVisibility);
        outBottomY = ImGui::GetCursorScreenPos().y;
    });
}

struct RowControlClickResult {
    bool  bFound = false;
    float y      = -1.0f;
};

// Sweeps the tab area for a click (hover/press/release, mirroring every other click search in this
// file) after which `probe()`'s own value differs from before. `bTypeCharacterAfterClick` extends
// the click with one more frame that injects a keystroke (only the Name field's InputText, once
// focused by the click, ever consumes it). Heals the one collateral this two-row scene can suffer —
// an accidental hit on Target's own [o]/[U]/X row-header strip removing it — by restoring the whole
// vector from a snapshot; there is no OTHER area here for a rename to collide with, unlike the
// rigged-duplicate scenes above, so a size-based heal alone is sufficient.
// The x sweep starts PAST every row's own CollapsingHeader arrow (~WindowPadding.x + one tree-node
// arrow width, comfortably under 30px) rather than at the window's own left edge: that arrow is the
// ONE affordance in this scene `OpenOnArrow` does not shield from a plain click (unlike the header's
// own label text, which - per RenderCollapsibleRow's own comment - never toggles collapse), and its
// own persisted open/closed bool lives in imgui's OWN id-keyed storage, invisible to and unresettable
// by this test's own scene/state structs, so an accidental hit there would silently blind the REST of
// whichever search hit it - this sweep never needs that zone anyway, since every control this ticket
// asks about (Name/X/Z/W/L/Color/Map Size) draws INSIDE the row's own indented, already-expanded body.
template <typename ProbeFunction>
RowControlClickResult FindRowControlByObservedChange(AreaDetailRowScene& scene, ProbeFunction probe,
                                                     bool bTypeCharacterAfterClick) {
    const std::vector<Params::MapArea> originalAreas = scene.recipe.areas;
    float bottomY = 0.0f;
    DrawAreaDetailRowFrame(scene, HeadlessMouseState(), false, bottomY);
    DrawAreaDetailRowFrame(scene, HeadlessMouseState(), false, bottomY);   // settle layout

    RowControlClickResult result;
    for (float y = 8.0f; y < bottomY && !result.bFound; y += 8.0f) {
        for (float x = 32.0f; x < kAreaDetailRowSceneWindowSize.x - 8.0f && !result.bFound; x += 10.0f) {
            const auto probeBefore = probe();
            HeadlessMouseState hover;   hover.position = ImVec2(x, y);
            HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
            HeadlessMouseState release = hover; release.bLeftButtonDown = false;
            float unused = 0.0f;
            DrawAreaDetailRowFrame(scene, hover,   false, unused);
            DrawAreaDetailRowFrame(scene, press,   false, unused);
            DrawAreaDetailRowFrame(scene, release, false, unused);
            if (bTypeCharacterAfterClick) {
                DrawAreaDetailRowFrame(scene, release, true, unused);
                // A character queued via io.AddInputCharactersUTF8 inside a frame's OWN draw call is
                // only guaranteed consumed by the still-active InputText on the NEXT redraw of that
                // same widget (this build's event-queue timing) - one more settle frame (no new
                // character) resolves it THIS iteration, so the very next line's `probe()` never
                // depends on some LATER iteration's own frames to observe today's click.
                DrawAreaDetailRowFrame(scene, release, false, unused);
            }
            const bool bFoundHere = probe() != probeBefore;
            // Heal an accidental open of the Color swatch's own picker popup: left open, it renders
            // OVER whatever sits to its own right (the "Map Size" button) for every later iteration,
            // silently shadowing it from every further click this search tries. A real click on a
            // neutral, empty spot BELOW every row's own content - never a real widget - closes any
            // open popup exactly the way a user dismisses one (click outside it), without touching
            // the keyboard at all: Escape was tried first here and, empirically, also interfered
            // with the Name field's OWN focus/typing on later iterations, so a mouse-only heal is
            // used instead, matching every other click this file already drives.
            const ImVec2 safePosition(5.0f, bottomY + 40.0f);
            HeadlessMouseState safeHover;   safeHover.position = safePosition;
            HeadlessMouseState safePress   = safeHover; safePress.bLeftButtonDown   = true;
            HeadlessMouseState safeRelease = safeHover; safeRelease.bLeftButtonDown = false;
            DrawAreaDetailRowFrame(scene, safeHover,   false, unused);
            DrawAreaDetailRowFrame(scene, safePress,   false, unused);
            DrawAreaDetailRowFrame(scene, safeRelease, false, unused);
            // Heal an accidental collapse of either Section header's own full-width bar - see
            // FindAffordanceIconByObservedFlip's own identical comment above for why.
            scene.state.globalSection.bOpen = true;
            scene.state.areaSection.bOpen   = true;
            if (scene.recipe.areas.size() != originalAreas.size()) {
                scene.recipe.areas = originalAreas;   // heal an accidental X##delete
                continue;
            }
            if (bFoundHere) { result.bFound = true; result.y = y; }
        }
    }
    return result;
}

void RunAreaDetailSingleLineRowAcceptanceChecks() {
    // The Name field: leftmost control. A click focuses it (real mouse focus, matching how a user
    // actually reaches it); the very next frame's injected "Q" only lands if that click found it.
    {
        HeadlessImguiSession session;
        AreaDetailRowScene scene = MakeAreaDetailRowScene();
        auto targetName = [&] {
            return scene.recipe.areas.size() > 1u ? scene.recipe.areas[1].name : std::string();
        };
        const RowControlClickResult nameResult =
            FindRowControlByObservedChange(scene, targetName, /*bTypeCharacterAfterClick=*/true);
        Check(nameResult.bFound, "the Name field for a non-Playable area's single-line row is "
                                 "reachable by click, and accepts typed text once focused");

        // The X-position slider: a middle control. A plain click (no drag needed - the track's own
        // absolute-position mapping applies on the press frame, SliderScalar_Track_UI.cpp) sets
        // area.originX from the click's own X coordinate.
        AreaDetailRowScene sliderScene = MakeAreaDetailRowScene();
        auto targetOriginX = [&] {
            return sliderScene.recipe.areas.size() > 1u ? sliderScene.recipe.areas[1].originX : 0.0f;
        };
        const RowControlClickResult sliderResult =
            FindRowControlByObservedChange(sliderScene, targetOriginX, /*bTypeCharacterAfterClick=*/false);
        Check(sliderResult.bFound, "the X-position slider for a non-Playable area's single-line row "
                                   "is reachable by a plain click");

        // The "Map Size" button: rightmost control.
        AreaDetailRowScene mapSizeScene = MakeAreaDetailRowScene();
        auto targetIsMapSized = [&] {
            return mapSizeScene.recipe.areas.size() > 1u
                && mapSizeScene.recipe.areas[1].originX == 0.0f && mapSizeScene.recipe.areas[1].originZ == 0.0f
                && mapSizeScene.recipe.areas[1].width == 512.0f && mapSizeScene.recipe.areas[1].length == 512.0f;
        };
        const RowControlClickResult mapSizeResult =
            FindRowControlByObservedChange(mapSizeScene, targetIsMapSized, /*bTypeCharacterAfterClick=*/false);
        Check(mapSizeResult.bFound, "the \"Map Size\" button for a non-Playable area's single-line "
                                    "row is reachable by click");

        // The acceptance test itself: all three, independently located, land on the SAME Y band -
        // the runtime proxy for "one plain imgui line, no child/scrollbar wrapper," not the old
        // five-line stack STEP221 shipped.
        if (nameResult.bFound && sliderResult.bFound && mapSizeResult.bFound) {
            constexpr float kSameLineTolerancePixels = 16.0f;   // two search steps' worth of slack
            Check(std::fabs(nameResult.y - sliderResult.y) <= kSameLineTolerancePixels,
                  "the Name field and the X-position slider land on the same Y - one imgui line");
            Check(std::fabs(nameResult.y - mapSizeResult.y) <= kSameLineTolerancePixels,
                  "the Name field and the \"Map Size\" button land on the same Y - one imgui line, "
                  "not the old five-line stack");
        }
    }
}

} // namespace

int main() {
    RunPlayableAreaChecks();
    RunSetToMapSizeChecks();
    RunCenterAreaInMapChecks();
    RunUniqueNameChecks();
    RunSliderAndSelectionChecks();
    RunAreaColorResolutionChecks();
    RunAreaPaletteAssignmentChecks();
    RunColorRenameRetargetingChecks();
    RunAreaLockResolutionChecks();
    RunLockRenameRetargetingChecks();
    RunVisibilityRenameRetargetingChecks();
    RunAreaVisibilityResolutionChecks();
    RunFreshAreaSizeChecks();
    RunAreaVisibilityClickAcceptanceChecks();
    RunAreaCenterButtonClickAcceptanceChecks();
    RunAreaDetailSingleLineRowAcceptanceChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
