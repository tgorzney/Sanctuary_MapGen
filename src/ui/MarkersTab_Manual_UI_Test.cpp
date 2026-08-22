// MarkersTab_Manual_UI_Test.cpp — STEP49 acceptance: the manual markers editor's PURE logic —
// group/instance selection, row labels, the Spawn-group army picker resolve, the per-group name
// uniqueness scoping, and the X/Z slider bounds. No imgui frame, no window, no GL context.
#include "MarkersTab_Manual_UI.h"
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

Params::MarkerInstanceGroup MakeGroup(const char* name) {
    Params::MarkerInstanceGroup group;
    group.name = name;
    return group;
}

Params::MarkerTransform MakeInstance(const char* name, const char* alias = "") {
    Params::MarkerTransform transform;
    transform.name  = name;
    transform.alias = alias;
    return transform;
}

Params::Army MakeArmy(const char* name, const char* alias = "") {
    Params::Army army;
    army.name  = name;
    army.alias = alias;
    return army;
}

void RunGroupSelectionChecks() {
    std::vector<Params::MarkerInstanceGroup> markers;
    Check(SelectedMarkerGroup(markers, -1) == nullptr, "an empty roster selects no group");
    markers.push_back(MakeGroup("Spawn"));
    Check(SelectedMarkerGroup(markers, 0) == &markers[0], "the selected group is reachable");
    Check(SelectedMarkerGroup(markers, 3) == nullptr, "an index past the last row selects nothing");

    Check(ResolvedMarkerGroupSelection(2, 5) == 2, "a selection inside the list is kept");
    Check(ResolvedMarkerGroupSelection(4, 3) == 2, "one past the end falls back to the last row");
    Check(ResolvedMarkerGroupSelection(0, 0) == -1, "an emptied list selects nothing");
}

void RunInstanceSelectionChecks() {
    std::vector<Params::MarkerTransform> transforms;
    Check(SelectedMarkerInstance(transforms, -1) == nullptr, "an empty group selects no instance");
    transforms.push_back(MakeInstance("Mex 0"));
    Check(SelectedMarkerInstance(transforms, 0) == &transforms[0], "the selected instance is reachable");

    Check(ResolvedMarkerInstanceSelection(1, 3) == 1, "a selection inside the list is kept");
    Check(ResolvedMarkerInstanceSelection(5, 2) == 1, "one past the end falls back to the last row");
    Check(ResolvedMarkerInstanceSelection(0, 0) == -1, "an emptied group selects nothing");
}

void RunRowLabelChecks() {
    Params::MarkerInstanceGroup group;
    Check(std::string(MarkerGroupRowLabel(group)) == "Marker Type", "an unnamed group falls back");
    group.name = "Alloys";
    Check(std::string(MarkerGroupRowLabel(group)) == "Alloys", "a named group shows its own name");

    Params::MarkerTransform transform;
    Check(std::string(MarkerInstanceRowLabel(transform)) == "Marker",
          "an unnamed, unaliased instance falls back");
    transform.name = "Mex 0";
    Check(std::string(MarkerInstanceRowLabel(transform)) == "Mex 0", "the name shows with no alias");
    transform.alias = "Front Mex";
    Check(std::string(MarkerInstanceRowLabel(transform)) == "Front Mex",
          "the alias wins over the name once set");
}

void RunSpawnGroupChecks() {
    Check(IsSpawnMarkerGroup(MakeGroup("Spawn")), "the reserved group name is recognized");
    Check(!IsSpawnMarkerGroup(MakeGroup("Alloys")), "any other group name is not");
    Check(!IsSpawnMarkerGroup(MakeGroup("")), "an empty name is not the Spawn group");
}

void RunArmyPickerChecks() {
    std::vector<Params::Army> armies;
    armies.push_back(MakeArmy("Player1"));
    armies.push_back(MakeArmy("Player2", "Commander Two"));

    Check(ResolvedSpawnMarkerArmyPickIndex(armies, "Player1") == 0,
          "a name matching the first army resolves to its index");
    Check(ResolvedSpawnMarkerArmyPickIndex(armies, "Player2") == 1,
          "a name matching the second army resolves to its index, alias notwithstanding");
    Check(ResolvedSpawnMarkerArmyPickIndex(armies, "NoSuchArmy") == -1,
          "a name matching no current army resolves to -1 (a stale pick), not a wrong row");
    Check(ResolvedSpawnMarkerArmyPickIndex({}, "Player1") == -1, "an empty roster always resolves to -1");

    Check(std::string(ArmyPickerRowLabel(armies[0])) == "Player1",
          "an army with no alias is shown by its real name");
    Check(std::string(ArmyPickerRowLabel(armies[1])) == "Commander Two",
          "an army with an alias is shown by the friendlier alias");
    Check(std::string(ArmyPickerRowLabel(Params::Army())) == "Army",
          "an entirely blank army still shows something, never an empty row");
}

// The `.sanmap` inner dictionary key only needs uniqueness WITHIN its own outer group — two
// different groups may each carry an instance named "Mex 0" without colliding on export.
void RunPerGroupUniqueNameScopingChecks() {
    Params::MarkerInstanceGroup alloys = MakeGroup("Alloys");
    alloys.transforms.push_back(MakeInstance("Mex 0"));
    alloys.transforms.push_back(MakeInstance("Mex 0"));
    Params::MarkerInstanceGroup spawn = MakeGroup("Spawn");
    spawn.transforms.push_back(MakeInstance("Mex 0"));   // same name, DIFFERENT group: no clash

    Check(MakeNamesUnique(alloys.transforms), "a real clash WITHIN one group's roster is repaired");
    Check(alloys.transforms[0].name == "Mex 0" && alloys.transforms[1].name == "Mex 0_1",
          "the first instance keeps the name, the later one is suffixed");
    Check(spawn.transforms[0].name == "Mex 0",
          "a same-named instance in a DIFFERENT group is untouched — scoping is per-group");
}

// Group names ARE global — `recipe.markers` itself is a dictionary keyed by group name.
void RunGroupNameUniquenessChecks() {
    std::vector<Params::MarkerInstanceGroup> markers;
    markers.push_back(MakeGroup("Outpost"));
    markers.push_back(MakeGroup("Outpost"));
    Check(MakeNamesUnique(markers), "colliding group names are repaired");
    Check(markers[0].name == "Outpost" && markers[1].name == "Outpost_1",
          "the first group keeps the name, the later one is suffixed");
}

void RunPositionRangeChecks() {
    const ScalarSliderRange range = MarkerPositionHorizontalSliderRange(512);
    Check(range.minimumValue == -512.0f && range.maximumValue == 1024.0f,
          "the X/Z bounds carry one map width of slack on each side, same reasoning as "
          "AreaOriginSliderRange");
    Check(range.increment <= 0.0f, "unlike an area's origin, a marker's position is continuous");
    Check(MarkerPositionHorizontalSliderRange(0).maximumValue >= 2.0f,
          "a nonsense map size still yields a usable track");

    const ManualMarkersState state;
    Check(state.positionElevationRange.minimumValue < 0.0f
          && state.positionElevationRange.maximumValue > 128.0f,
          "the elevation placeholder clears the default terrainMaxHeight (128) with headroom");
}

void RunFreshInstanceNameChecks() {
    Check(NextMarkerInstanceName(0) == "Marker_0", "a fresh instance is seeded with a visible name");
    Check(NextMarkerInstanceName(2) == "Marker_2", "the seed counts the roster it is joining");
}

} // namespace

int main() {
    RunGroupSelectionChecks();
    RunInstanceSelectionChecks();
    RunRowLabelChecks();
    RunSpawnGroupChecks();
    RunArmyPickerChecks();
    RunPerGroupUniqueNameScopingChecks();
    RunGroupNameUniquenessChecks();
    RunPositionRangeChecks();
    RunFreshInstanceNameChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
