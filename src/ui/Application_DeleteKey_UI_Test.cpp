// Application_DeleteKey_UI_Test.cpp — headless acceptance coverage for the pure logic behind
// `Application::ApplyGlobalDeleteShortcut` (STEP234, DESIGN_MarkerLink_R1.md §1.3):
// `ShouldApplyGlobalDeleteShortcut`'s own gating and `DeleteSelectedManualInstancesAcrossDomains`'s
// own cross-domain partition+erase. The private Application method itself is a thin, untestable-
// without-a-window glue over exactly these two functions (reads `ImGui::GetIO().WantTextInput`/
// `ImGui::IsKeyPressed`/`scenarioEditMode.IsActive()`, then calls straight into them) — no GL/window
// is needed to exercise the actual decision logic the ticket's own Verify section asks for.
#include "Application_DeleteKey_UI.h"
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

OverlayInstanceKey_UI MakeKey(std::int32_t instanceIndex, PlacementCollectionKind_UI collection,
                              bool bManual = true) {
    return OverlayInstanceKey_UI{collection, instanceIndex, /*bValid=*/true, bManual};
}

// ---- Gating: a live text field wins, Scenario Edit Mode wins, otherwise the key press decides.

void RunGatingChecks() {
    Check(!ShouldApplyGlobalDeleteShortcut(/*bWantTextInput=*/true, /*bScenarioEditModeActive=*/false,
                                           /*bDeleteKeyPressed=*/true),
          "a live rename/text field blocks the shortcut even with Delete pressed");
    Check(!ShouldApplyGlobalDeleteShortcut(/*bWantTextInput=*/false, /*bScenarioEditModeActive=*/true,
                                           /*bDeleteKeyPressed=*/true),
          "Scenario Edit Mode's exclusive canvas ownership blocks the shortcut even with Delete pressed");
    Check(!ShouldApplyGlobalDeleteShortcut(false, false, /*bDeleteKeyPressed=*/false),
          "no key press is a no-op with neither gate active");
    Check(ShouldApplyGlobalDeleteShortcut(false, false, true),
          "Delete fires when neither gate is active and the key was pressed");
}

// ---- The cross-domain mutation: empty/all-procedural is a no-op, a mixed selection deletes from
// every domain it touches.

void RunCrossDomainDeleteChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(Params::MarkerTransform{});
    markers[0].transforms[0].instanceIdentifier = 1;
    std::vector<Params::PropInstanceGroup> props(1);
    Params::PropTransform propTransform; propTransform.instanceIdentifier = 2;
    props[0].transforms.push_back(propTransform);
    std::vector<Params::DecalInstanceGroup> decals(1);
    Params::DecalTransform decalTransform; decalTransform.instanceIdentifier = 3;
    decals[0].transforms.push_back(decalTransform);
    const std::vector<Params::MarkerInstanceLayer> markerLayers;
    const std::vector<Params::MarkerLink>          markerLinks;
    const std::vector<Params::PropInstanceLayer>   propLayers;
    const std::vector<Params::DecalInstanceLayer>  decalLayers;

    // An empty selection is a no-op.
    OverlayInstanceKeySet_UI empty;
    Check(!DeleteSelectedManualInstancesAcrossDomains(empty, markers, markerLayers, markerLinks, props,
                                                      propLayers, decals, decalLayers),
          "an empty selection deletes nothing and reports false");
    Check(markers[0].transforms.size() == 1, "an empty selection leaves every domain's roster untouched");

    // An all-PROCEDURAL selection (bManual == false) is a no-op, even naming a real identifier.
    OverlayInstanceKeySet_UI procedural;
    procedural.keys.push_back(MakeKey(1, PlacementCollectionKind_UI::Markers, /*bManual=*/false));
    Check(!DeleteSelectedManualInstancesAcrossDomains(procedural, markers, markerLayers, markerLinks,
                                                      props, propLayers, decals, decalLayers),
          "a procedural-only selection is skipped entirely, never mistaken for a manual identifier");
    Check(markers[0].transforms.size() == 1, "the roster is untouched by the procedural-only selection");

    // A mixed Marker+Prop+Decal selection deletes from all three.
    OverlayInstanceKeySet_UI mixed;
    mixed.keys.push_back(MakeKey(1, PlacementCollectionKind_UI::Markers));
    mixed.keys.push_back(MakeKey(2, PlacementCollectionKind_UI::Props));
    mixed.keys.push_back(MakeKey(3, PlacementCollectionKind_UI::Decals));
    mixed.keys.push_back(MakeKey(99, PlacementCollectionKind_UI::Units));   // Units — silently ignored
    Check(DeleteSelectedManualInstancesAcrossDomains(mixed, markers, markerLayers, markerLinks, props,
                                                     propLayers, decals, decalLayers),
          "a mixed Marker+Prop+Decal selection reports a real deletion occurred");
    Check(markers[0].transforms.empty(), "the targeted marker instance is gone");
    Check(props[0].transforms.empty(), "the targeted prop instance is gone");
    Check(decals[0].transforms.empty(), "the targeted decal instance is gone");
}

} // namespace

int main() {
    RunGatingChecks();
    RunCrossDomainDeleteChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
