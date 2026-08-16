// MaskLayerTabs_UI_Test.cpp — tab-rebuild C2 acceptance: the four mask-layer tabs (Detail Normal,
// Tint, Holes, Smoothness) are a show toggle over ONE hosted Layer Editor, and each owns its own
// state so two stacks on screen cannot share a drag. Pure checks — no imgui frame, no window, no GL
// context.
#include "DetailNormalTab_UI.h"
#include "HolesTab_UI.h"
#include "SmoothnessTab_UI.h"
#include "TintTab_UI.h"
#include "../params/LayerStack_PARAMS.h"
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

// Each tab holds its OWN editor state, so a drag in Tint cannot move the selection in Holes — the
// v1 function-static defect the shared library exists to kill.
void RunIndependentStateChecks() {
    MaskLayerTabState tintState;
    MaskLayerTabState holesState;
    Check(tintState.bShowOverlay && holesState.bShowOverlay, "a mask tab opens showing its overlay");
    Check(tintState.layerSection.bOpen, "with its layer stack expanded");
    Check(!tintState.layerEditor.advancedConstantsSection.bOpen,
          "and the hosted editor keeps Advanced (constants) collapsed");

    tintState.layerEditor.selectedGeoLayerIndex = 2;
    tintState.layerEditor.selectedLayerIndex    = 1;
    tintState.bShowOverlay                      = false;
    Check(holesState.layerEditor.selectedGeoLayerIndex == 0
          && holesState.layerEditor.selectedLayerIndex == 0 && holesState.bShowOverlay,
          "moving one mask tab's selection leaves every other tab alone");
}

// The hosted editor edits the caller's stack in place: the tab adds no stack of its own
// (MaskLayerTab_UI.h SCOPE NOTE 1).
void RunHostedStackChecks() {
    Params::LayerStack tintLayers;
    MaskLayerTabState state;
    tintLayers.geoLayers.resize(1);
    tintLayers.geoLayers[0].layers.resize(3);
    state.layerEditor.selectedGeoLayerIndex = 0;
    state.layerEditor.selectedLayerIndex    = 2;
    Check(SelectedLayerEditorLayer(tintLayers, state.layerEditor)
          == &tintLayers.geoLayers[0].layers[2],
          "the tab's editor state selects into the stack the caller handed it");

    state.layerEditor.selectedLayerIndex = 7;                 // a selection the stack cannot honour
    Check(SelectedLayerEditorLayer(tintLayers, state.layerEditor) == nullptr,
          "a selection past the end answers nothing, never a neighbouring layer");
}

// Detail Normal is the one mask tab with an extra control: the texture size.
void RunDetailNormalSizeChecks() {
    Check(DetailNormalSizeIndexOf(1024) == 2, "the offered sizes map onto their dropdown rows");
    Check(DetailNormalSizeIndexOf(768) == -1, "a size the dropdown does not list shows nothing picked");
    Check(DetailNormalSizeAtIndex(4) == 4096, "the last row is 4096");
    Check(DetailNormalSizeAtIndex(-1) == 0 && DetailNormalSizeAtIndex(kDetailNormalSizeCount) == 0,
          "an out-of-range row answers 'no size', never a neighbour");

    DetailNormalTabState state;
    Check(DetailNormalSizeOf(state) == 1024, "the tab opens on the v1 default size");
    state.detailNormalSizeIndex = 4;
    Check(DetailNormalSizeOf(state) == 4096, "and follows the picked row");
    state.detailNormalSizeIndex = -1;                          // nothing picked
    Check(DetailNormalSizeOf(state) == 1024, "an unpicked row falls back to the default, never zero");
    Check(state.maskLayerTab.bShowOverlay,
          "and it carries the same shared mask-layer state the other three do");
}

} // namespace

int main() {
    RunIndependentStateChecks();
    RunHostedStackChecks();
    RunDetailNormalSizeChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
