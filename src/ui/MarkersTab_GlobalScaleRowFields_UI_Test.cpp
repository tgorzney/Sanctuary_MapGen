// MarkersTab_GlobalScaleRowFields_UI_Test.cpp — STEP121 acceptance: the direct-binding resolver
// between a Markers-tab Global scale row and its `Params::GlobalMarkerSettings` fields, driven
// headless (no imgui frame, window or GL context). Sibling TU to MarkersTab_UI_Test.cpp, which
// owns main() and the shared `Check`/`failureCount` (ARCH §1.5 — one binary, split translation
// units; kept in its own TU rather than growing MarkersTab_UI_Test.cpp past the ARCH §1.5 hard
// ceiling, which was NOT one of this ticket's own pre-approved ceiling exceptions).
#include "MarkersTab_UI.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

extern int failureCount;
void Check(bool bCondition, const char* label);

namespace {

// The direct-binding resolver is the whole contract between a global scale row and
// `Params::GlobalMarkerSettings`: row index -> that category's own scale/color/icon-name fields,
// and an out-of-range row resolves every pointer to null rather than a stale/aliased one.
void RunResolveGlobalMarkerScaleRowFieldsChecks() {
    Params::GlobalMarkerSettings settings;
    const GlobalMarkerScaleRowFields alloy = ResolveGlobalMarkerScaleRowFields(settings, 0);
    Check(alloy.scale == &settings.scaleAlloy && alloy.color == settings.colorAlloy
          && alloy.iconName == &settings.iconNameAlloy && alloy.selectColor == settings.selectColorAlloy,
          "row 0 resolves to the Alloy fields, including selectColor (STEP134)");
    const GlobalMarkerScaleRowFields plasma = ResolveGlobalMarkerScaleRowFields(settings, 1);
    Check(plasma.scale == &settings.scalePlasma && plasma.iconName == &settings.iconNamePlasma
          && plasma.selectColor == settings.selectColorPlasma,
          "row 1 resolves to the Plasma fields, including selectColor");
    const GlobalMarkerScaleRowFields spawn = ResolveGlobalMarkerScaleRowFields(settings, 2);
    Check(spawn.scale == &settings.scaleSpawn && spawn.iconName == &settings.iconNameSpawn
          && spawn.selectColor == settings.selectColorSpawn,
          "row 2 resolves to the Spawn fields, including selectColor");
    const GlobalMarkerScaleRowFields outOfRange = ResolveGlobalMarkerScaleRowFields(settings, 3);
    Check(outOfRange.scale == nullptr && outOfRange.color == nullptr && outOfRange.iconName == nullptr
          && outOfRange.selectColor == nullptr,
          "an out-of-range row resolves every pointer to null, not a stale/aliased one");
}

} // namespace

void RunGlobalMarkerScaleRowFieldsAcceptanceChecks() {
    RunResolveGlobalMarkerScaleRowFieldsChecks();
}
