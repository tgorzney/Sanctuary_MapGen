// Application_ViewLayersPopup_UI_Test.cpp — STEP54 acceptance, part 1: ApplyViewLayerSignal's
// structural rule proven headlessly (no imgui frame) for both list element types the View popup
// drives — PreviewFieldLayer (terrain) and OverlayLayer_UI (overlays). Part 2 (the real
// "ViewListField"/"ViewListOverlay" payload-identifier cross-section rejection, which needs a live
// DraggableList<T>::Render drag-drop path) is Application_ViewLayersPopup_CrossSection_UI_Test.cpp;
// main() lives here.
#include "Application_ViewLayersPopup_UI.h"
#include "PreviewComposite_Settings_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include <cstdio>
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// §14.2's "toolbar never adds/removes" rule: Delete must no-op for both element types.
void TestDeleteIsNoopBothTypes() {
    std::vector<PreviewFieldLayer> fieldLayers(2);
    DraggableListSignal deleteSignal;
    deleteSignal.kind = DraggableListSignalKind::Delete;
    deleteSignal.sourceRowIndex = 0;
    Check(!ApplyViewLayerSignal(fieldLayers, deleteSignal) && fieldLayers.size() == 2u,
          "Delete is a no-op for PreviewFieldLayer (View toolbar never adds/removes)");

    std::vector<OverlayLayer_UI> overlayLayers(2);
    Check(!ApplyViewLayerSignal(overlayLayers, deleteSignal) && overlayLayers.size() == 2u,
          "Delete is a no-op for OverlayLayer_UI (View toolbar never adds/removes)");
}

void TestToggleVisibilityFlipsBEnabled() {
    std::vector<PreviewFieldLayer> fieldLayers(2);
    DraggableListSignal toggleSignal;
    toggleSignal.kind = DraggableListSignalKind::ToggleVisibility;
    toggleSignal.sourceRowIndex = 1;
    Check(ApplyViewLayerSignal(fieldLayers, toggleSignal) && !fieldLayers[1].bEnabled,
          "ToggleVisibility flips PreviewFieldLayer::bEnabled and returns true");

    std::vector<OverlayLayer_UI> overlayLayers(2);
    Check(ApplyViewLayerSignal(overlayLayers, toggleSignal) && !overlayLayers[1].bEnabled,
          "ToggleVisibility flips OverlayLayer_UI::bEnabled and returns true");
}

// ToggleLock/Select belong to state neither element type carries — both fall through unapplied.
void TestLockAndSelectAreNoop() {
    std::vector<PreviewFieldLayer> fieldLayers(2);
    DraggableListSignal lockSignal;
    lockSignal.kind = DraggableListSignalKind::ToggleLock;
    lockSignal.sourceRowIndex = 0;
    Check(!ApplyViewLayerSignal(fieldLayers, lockSignal), "ToggleLock is a no-op (not owned by this widget)");

    DraggableListSignal selectSignal;
    selectSignal.kind = DraggableListSignalKind::Select;
    selectSignal.sourceRowIndex = 0;
    Check(!ApplyViewLayerSignal(fieldLayers, selectSignal), "Select is a no-op (not owned by this widget)");
}

void TestReorderDelegatesToApplyDraggableListSignal() {
    std::vector<PreviewFieldLayer> fieldLayers(3);
    fieldLayers[0].kind = PreviewLayerKind::HeightRamp;
    fieldLayers[1].kind = PreviewLayerKind::Flow;
    fieldLayers[2].kind = PreviewLayerKind::Water;
    DraggableListSignal reorderSignal;
    reorderSignal.kind = DraggableListSignalKind::Reorder;
    reorderSignal.sourceRowIndex = 0;
    reorderSignal.targetRowIndex = 2;
    Check(ApplyViewLayerSignal(fieldLayers, reorderSignal) &&
          fieldLayers[2].kind == PreviewLayerKind::HeightRamp,
          "Reorder moves the dragged element onto the target index");
}

} // namespace

namespace SanmapGen {
namespace Ui {
// Returns its own failure count; Application_ViewLayersPopup_CrossSection_UI_Test.cpp.
int RunViewLayersCrossSectionAcceptance();
// Returns its own failure count; Application_ViewLayersPopup_FlatRowLayout_UI_Test.cpp (STEP200).
int RunViewLayersFlatRowLayoutAcceptance();
}
}

int main() {
    TestDeleteIsNoopBothTypes();
    TestToggleVisibilityFlipsBEnabled();
    TestLockAndSelectAreNoop();
    TestReorderDelegatesToApplyDraggableListSignal();
    failureCount += SanmapGen::Ui::RunViewLayersCrossSectionAcceptance();
    failureCount += SanmapGen::Ui::RunViewLayersFlatRowLayoutAcceptance();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
