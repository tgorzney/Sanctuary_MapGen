// Application_DeleteKey_UI.h — pure, imgui-free logic behind `Application::ApplyGlobalDeleteShortcut`
// (STEP234, DESIGN_MarkerLink_R1.md §1.3). Split out of the private Application method itself so
// both the keypress-gating rule and the actual cross-domain erase are directly unit-testable with no
// imgui frame/window/GL — the same "pure ApplyViewLayerSignal, testable with no imgui frame" posture
// `Application_ViewLayersPopup_UI.h` already established (STEP54).
#pragma once
#include <vector>
#include "MapCanvas_SelectionSet_UI.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
#include "../params/ScatterInstanceLayer_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// True exactly when the global Delete shortcut fires this frame. A live rename/text field
// (`bWantTextInput`) and Scenario Edit Mode's own exclusive canvas ownership
// (`bScenarioEditModeActive`) both defer — the identical exclusivity gate `DrawCanvasWindow` already
// applies to canvas interaction (Application_Draw_UI.cpp), since Delete is a canvas-selection action.
inline bool ShouldApplyGlobalDeleteShortcut(bool bWantTextInput, bool bScenarioEditModeActive,
                                            bool bDeleteKeyPressed) {
    if (bWantTextInput) return false;
    if (bScenarioEditModeActive) return false;
    return bDeleteKeyPressed;
}

// The actual mutation: partitions `selected`'s MANUAL (bManual) keys by domain (a procedural key or
// a Units key is silently skipped — no persisted identity/out of scope, DESIGN_MarkerLink_R1.md
// §0/§1.5), erases the targeted identifiers from whichever domains had any via
// ManualInstanceDelete_UI.h's per-domain wrappers, and reports whether anything was actually erased
// (a wholly locked or already-stale selection is a legal no-op, never an error). Declared here,
// defined in Application_DeleteKey_UI.cpp (needs ManualInstanceDelete_UI.h's wrappers). `markerLinks`
// (ARCH §21.9, STEP249) threads straight into `DeleteSelectedManualMarkerInstances`'s own new
// parameter — mechanical, Props/Decals have no Link concept to add one for.
bool DeleteSelectedManualInstancesAcrossDomains(
    const OverlayInstanceKeySet_UI& selected,
    std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const std::vector<Params::MarkerLink>& markerLinks,
    std::vector<Params::PropInstanceGroup>& props,
    const std::vector<Params::PropInstanceLayer>& propLayers,
    std::vector<Params::DecalInstanceGroup>& decals,
    const std::vector<Params::DecalInstanceLayer>& decalLayers);

} // namespace Ui
} // namespace SanmapGen
