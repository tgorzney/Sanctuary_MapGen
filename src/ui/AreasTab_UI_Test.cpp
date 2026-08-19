// AreasTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 5: the Areas tab. Retyped by STEP21
// onto the real `Params::MapArea`. The rules with teeth are pure and asserted here: the
// engine-required PlayableArea, the unique-name repair the export depends on, Set to Map Size, the
// color-rename-retargeting fix (STEP21 ruling #5), and a fresh area's visible-size seed (STEP21
// ruling #7). v1 ran the name repair as a loop tacked onto the end of the tab draw, so it could
// only ever run while the tab was open — and was never tested. No imgui frame, no window, no GL
// context.
// NOT YET REGISTERED IN CMake — WO C4 does not own CMakeLists.txt (gate CD-int registers it).
#include "AreasTab_UI.h"
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

    const AreasTabState state;
    Check(state.bAreasLocked, "the tab opens locked, as v1 did");
}

// STEP21 ruling #4: color has no `_PARAMS` home, so it lives in a UI-only side table keyed by
// area NAME. `ResolveAreaColor` finds an existing entry or appends a default one.
void RunAreaColorResolutionChecks() {
    std::vector<AreaColorEntry> areaColors;
    float* const firstResolve = ResolveAreaColor(areaColors, "Base");
    Check(areaColors.size() == 1u, "the first touch of a name appends one entry");
    Check(firstResolve[0] == 1.0f && firstResolve[1] == 1.0f && firstResolve[2] == 1.0f
          && firstResolve[3] == 0.35f,
          "a fresh entry defaults to the same color MapAreaRectangle used to (white, translucent)");

    firstResolve[0] = 0.2f;
    float* const secondResolve = ResolveAreaColor(areaColors, "Base");
    Check(areaColors.size() == 1u, "resolving the same name again appends nothing");
    Check(secondResolve[0] == 0.2f, "and returns the SAME entry, edits intact");

    ResolveAreaColor(areaColors, "Other");
    Check(areaColors.size() == 2u, "a different name gets its own entry");
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

} // namespace

int main() {
    RunPlayableAreaChecks();
    RunSetToMapSizeChecks();
    RunUniqueNameChecks();
    RunSliderAndSelectionChecks();
    RunAreaColorResolutionChecks();
    RunColorRenameRetargetingChecks();
    RunFreshAreaSizeChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
