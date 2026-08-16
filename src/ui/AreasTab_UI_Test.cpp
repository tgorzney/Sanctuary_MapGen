// AreasTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 5: the Areas tab. The three rules with
// teeth are pure and asserted here: the engine-required PlayableArea, the unique-name repair the
// export depends on, and Set to Map Size. v1 ran the name repair as a loop tacked onto the end of
// the tab draw, so it could only ever run while the tab was open — and was never tested.
// No imgui frame, no window, no GL context.
// NOT YET REGISTERED IN CMake — WO C4 does not own CMakeLists.txt (gate CD-int registers it).
#include "AreasTab_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

MapAreaRectangle MakeArea(const char* name) {
    MapAreaRectangle area;
    area.name = name;
    return area;
}

void RunPlayableAreaChecks() {
    std::vector<MapAreaRectangle> areas;
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
    std::vector<MapAreaRectangle> renamedFirst;
    renamedFirst.push_back(MakeArea("NotThePlayableArea"));
    Check(EnsurePlayableArea(renamedFirst, 256) && renamedFirst.size() == 2u
          && IsPlayableArea(renamedFirst[0]),
          "a list whose rows are all named something else still gains one, inserted first");
}

void RunSetToMapSizeChecks() {
    MapAreaRectangle area = MakeArea("Playfield");
    area.originX = 40.0f; area.originZ = -7.0f;
    Check(SetAreaToMapSize(area, 1024), "the button reports the rectangle moved");
    Check(area.originX == 0.0f && area.originZ == 0.0f
          && area.width == 1024.0f && area.length == 1024.0f,
          "and the rectangle becomes exactly the map");
    Check(!SetAreaToMapSize(area, 1024),
          "pressing it again reports no move, so it costs no recomposite");

    MapAreaRectangle degenerate = MakeArea("Tiny");
    SetAreaToMapSize(degenerate, 0);
    Check(degenerate.width >= 1.0f && degenerate.length >= 1.0f,
          "a nonsense map size is repaired, never obeyed: an area is never zero-sized");
}

// The export keys areas by name, so two rows sharing one would silently drop an area.
void RunUniqueNameChecks() {
    std::vector<MapAreaRectangle> areas;
    areas.push_back(MakeArea("Base"));
    areas.push_back(MakeArea("Base"));
    areas.push_back(MakeArea("Base"));
    Check(MakeAreaNamesUnique(areas), "duplicates report the repair");
    Check(areas[0].name == "Base" && areas[1].name == "Base_1" && areas[2].name == "Base_2",
          "the FIRST row keeps the name and every later clash is suffixed");
    Check(!MakeAreaNamesUnique(areas), "a list that is already unique is left alone");

    // A suffix that itself collides must keep walking rather than settle on a duplicate.
    std::vector<MapAreaRectangle> colliding;
    colliding.push_back(MakeArea("Base"));
    colliding.push_back(MakeArea("Base_1"));
    colliding.push_back(MakeArea("Base"));
    Check(MakeAreaNamesUnique(colliding) && colliding[2].name == "Base_2",
          "a suffix that would itself collide is skipped");
    Check(!AreaNameIsTakenBefore(colliding, 0u, colliding[0].name),
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

    AreasTabState state;
    Check(SelectedArea(state) == nullptr, "an empty tab selects no area");
    state.areas.push_back(MakeArea(kPlayableAreaName));
    state.selectedAreaIndex = 0;
    Check(SelectedArea(state) == &state.areas[0], "the selected area is reachable");
    state.selectedAreaIndex = 3;
    Check(SelectedArea(state) == nullptr, "an index past the last row selects nothing");

    Check(ResolvedAreaSelection(2, 5) == 2, "a selection inside the list is kept");
    Check(ResolvedAreaSelection(4, 3) == 2, "one past the end falls back to the last row");
    Check(ResolvedAreaSelection(0, 0) == -1, "and an emptied list selects nothing");

    const ColorSwatchOptions options = AreasTabColorSwatchOptions();
    Check(options.bAlphaEnabled && options.bAlphaBarShown,
          "the area swatch is the one caller that turns the picker's alpha bar on");
    Check(state.bAreasLocked, "the tab opens locked, as v1 did");
}

} // namespace

int main() {
    RunPlayableAreaChecks();
    RunSetToMapSizeChecks();
    RunUniqueNameChecks();
    RunSliderAndSelectionChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
