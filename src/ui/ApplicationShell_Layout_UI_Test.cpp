// ApplicationShell_Layout_UI_Test.cpp — tab-rebuild WO E acceptance, part 1: the left column IS the
// v1 layout, and it is asserted rather than eyeballed. Three group headers, eighteen rows in the
// order TAB_REBUILD_PLAN "Layout (keep v1 shape)" states, and the `[O]`/`[ ]` toggle on every row
// v1 gave one to (and on neither SYSTEM row, which v1 drew with a plain Selectable).
// Headless: the catalogue is pure data, so no imgui frame, no window and no GL context.
#include "Application_UI.h"
#include "ApplicationShell_TestSupport_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// The plan's own list, written out here so the test fails when the catalogue drifts from the
// work-order rather than agreeing with itself.
const char* const expectedTerrainLabels[] = {
    "Symmetry", "Heightmap", "Slope", "Flow", "Accumulation", "Stratums", "Detail Normal",
    "Tint", "Holes", "Smoothness"
};
const char* const expectedEnvironmentLabels[] = {
    "Water", "Atmosphere", "Markers", "Armies", "Props", "Areas"
};
const char* const expectedSystemLabels[] = { "Performance", "Files" };

bool LabelsMatch(const char* left, const char* right) {
    for (int index = 0; left[index] != '\0' || right[index] != '\0'; ++index)
        if (left[index] != right[index]) return false;
    return true;
}

// Walks the catalogue in draw order and confirms one group's rows, in order, with no extras.
void CheckGroupOrder(ApplicationPanelGroup group, const char* const* expectedLabels,
                     int expectedCount, const char* groupName) {
    Check(ApplicationPanelCountOfGroup(group) == expectedCount, groupName);
    int matchedCount = 0;
    for (int panelIndex = 0; panelIndex < kApplicationPanelCount; ++panelIndex) {
        const ApplicationPanelEntry& entry = applicationPanelEntries[panelIndex];
        if (entry.group != group) continue;
        Check(matchedCount < expectedCount && LabelsMatch(entry.label, expectedLabels[matchedCount]),
              entry.label);
        ++matchedCount;
    }
    Check(matchedCount == expectedCount, groupName);
}

void RunCatalogueChecks() {
    Check(kApplicationPanelCount == 18, "the left column hosts all eighteen tabs");
    Check(kApplicationPanelGroupCount == 3, "under exactly three group headers");
    CheckGroupOrder(ApplicationPanelGroup::TerrainAndLayers, expectedTerrainLabels, 10,
                    "TERRAIN & LAYERS carries ten rows in the plan's order");
    CheckGroupOrder(ApplicationPanelGroup::Environment, expectedEnvironmentLabels, 6,
                    "ENVIRONMENT carries six rows in the plan's order");
    CheckGroupOrder(ApplicationPanelGroup::System, expectedSystemLabels, 2,
                    "SYSTEM carries two rows in the plan's order");
    for (const ApplicationPanelEntry& entry : applicationPanelEntries)
        Check(entry.bHasVisibilityToggle == (entry.group != ApplicationPanelGroup::System),
              "every row outside SYSTEM keeps v1's [O]/[ ] toggle");
    Check(ApplicationPanelEntryOf(ApplicationPanel::Files) != nullptr, "every panel resolves");
    Check(ApplicationPanelEntryOf(static_cast<ApplicationPanel>(-1)) == nullptr,
          "and a panel outside the enum resolves to nothing");
}

} // namespace

int main() {
    RunCatalogueChecks();
    RunShellVisibilityChecks();
    RunShellExecutionChecks();
    RunShellAppSettingsChecks();

    if (previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", previewTestFailureCount);
    return 1;
}
