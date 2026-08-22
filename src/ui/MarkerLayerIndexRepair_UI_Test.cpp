// MarkerLayerIndexRepair_UI_Test.cpp — acceptance coverage for the two `layerIndex` repair
// functions (STEP81 part (a)) plus the `layerId`-never-touched regression guard, `NextMarkerLayerId`
// across an add/delete cycle, and the Combo-binding defect part (b)'s ruling exists to prevent.
// Pure logic only — no imgui frame, no window, no GL context. Mirrors the assertion shape of
// PropsTab_UI_Test.cpp's `RunPropLayerRemovalChecks`/`RunPropLayerReorderRenumberChecks`.
#include "Combo_UI.h"
#include "DraggableListWidget_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkerLayerIndexRepair_UI.h"
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

// Two marker groups, four transforms across three layers, in recipe order: 0, 1, 0, 2 — the same
// shape PropsTab_UI_Test.cpp's `MakePropsWithLayers` uses, so both groups are walked.
std::vector<Params::MarkerInstanceGroup> MakeMarkersWithLayers() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].transforms.resize(3);
    markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].layerIndex = 1;
    markers[0].transforms[2].layerIndex = 0;
    markers[1].transforms.resize(1);
    markers[1].transforms[0].layerIndex = 2;
    return markers;
}

int MarkerCountAcrossGroups(const std::vector<Params::MarkerInstanceGroup>& markers) {
    int count = 0;
    for (const auto& group : markers) count += static_cast<int>(group.transforms.size());
    return count;
}

// A removed layer CLAMPS every referencing transform to layer 0 — the instance is never dropped.
void RunMarkerLayerRemovalChecks() {
    std::vector<Params::MarkerInstanceGroup> markers = MakeMarkersWithLayers();
    const int countBeforeRemoval = MarkerCountAcrossGroups(markers);
    Check(ClampMarkerLayerIndicesForRemovedLayer(markers, 1),
          "removing a layer with transforms reports the move");
    Check(MarkerCountAcrossGroups(markers) == countBeforeRemoval,
          "not one marker transform is dropped - a marker losing its layer tag is still a real marker");
    Check(markers[0].transforms[0].layerIndex == 0 && markers[0].transforms[2].layerIndex == 0,
          "layers BELOW the removed one keep their index");
    Check(markers[1].transforms[0].layerIndex == 1, "and every layer above it shifts down one");

    markers = MakeMarkersWithLayers();
    Check(ClampMarkerLayerIndicesForRemovedLayer(markers, 0),
          "removing layer 0 clamps every transform that named it");
    Check(markers[0].transforms[0].layerIndex == 0 && markers[0].transforms[2].layerIndex == 0,
          "the orphaned transforms clamp to layer 0, not -1 or any sentinel");
    Check(markers[0].transforms[1].layerIndex == 0 && markers[1].transforms[0].layerIndex == 1,
          "every surviving layer above the removed one shifts down one");

    markers = MakeMarkersWithLayers();
    Check(!ClampMarkerLayerIndicesForRemovedLayer(markers, -1),
          "a signal about no layer at all changes nothing");
    std::vector<Params::MarkerInstanceGroup> emptyMarkers;
    Check(!ClampMarkerLayerIndicesForRemovedLayer(emptyMarkers, 0), "and an empty recipe reports no move");
}

// Both drag directions, and the agreement with `ApplyDraggableListSignal`'s own reorder result on
// a parallel vector — the two moves must stay in lockstep.
void RunMarkerLayerReorderRenumberChecks() {
    // Downward (below-target): layer 0 dragged onto layer 2 (of 3 total).
    std::vector<Params::MarkerInstanceGroup> markers = MakeMarkersWithLayers();
    std::vector<int> layerIdentity = { 100, 101, 102 };   // parallel "layers" vector, by identity
    DraggableListSignal downwardSignal;
    downwardSignal.kind = DraggableListSignalKind::Reorder;
    downwardSignal.sourceRowIndex = 0;
    downwardSignal.targetRowIndex = 2;
    Check(RenumberMarkerLayerIndicesForReorder(markers, 0, 2, 3),
          "a downward reorder (source below target) reports the move");
    Check(ApplyDraggableListSignal(layerIdentity, downwardSignal),
          "and the parallel layers vector itself reports a move too");
    Check(markers[0].transforms[0].layerIndex == 2 && markers[0].transforms[2].layerIndex == 2,
          "both transforms that named the dragged layer now name its new (target) slot");
    Check(markers[0].transforms[1].layerIndex == 0, "layer 1's transform shifts down into layer 0's old slot");
    Check(markers[1].transforms[0].layerIndex == 1, "layer 2's transform shifts down into layer 1's old slot");
    Check(layerIdentity[2] == 100, "layer identity 100 (originally row 0) now sits at row 2, agreeing with the renumber");

    // Upward (above-target): layer 2 dragged onto layer 0.
    markers = MakeMarkersWithLayers();
    layerIdentity = { 100, 101, 102 };
    DraggableListSignal upwardSignal;
    upwardSignal.kind = DraggableListSignalKind::Reorder;
    upwardSignal.sourceRowIndex = 2;
    upwardSignal.targetRowIndex = 0;
    Check(RenumberMarkerLayerIndicesForReorder(markers, 2, 0, 3),
          "an upward reorder (source above target) reports the move");
    Check(ApplyDraggableListSignal(layerIdentity, upwardSignal),
          "and the parallel layers vector itself reports a move too");
    Check(markers[1].transforms[0].layerIndex == 0, "the dragged layer's transform now names its new (target) slot");
    Check(markers[0].transforms[0].layerIndex == 1 && markers[0].transforms[2].layerIndex == 1,
          "layer 0's transforms shift up into layer 1's old slot");
    Check(markers[0].transforms[1].layerIndex == 2, "layer 1's transform shifts up into layer 2's old slot");
    Check(layerIdentity[0] == 102, "layer identity 102 (originally row 2) now sits at row 0, agreeing with the renumber");
}

// No-op: source == target, and an out-of-range source (ARCH_01_05_FileSizeCeilings.md §1.5 — split
// out of RunMarkerLayerReorderRenumberChecks to keep both functions under the 40-line ceiling).
void RunMarkerLayerReorderNoOpChecks() {
    std::vector<Params::MarkerInstanceGroup> markers = MakeMarkersWithLayers();
    Check(!RenumberMarkerLayerIndicesForReorder(markers, 1, 1, 3), "dropping a row back on itself reports no move");
    Check(markers[0].transforms[1].layerIndex == 1, "and changes nothing");
    Check(!RenumberMarkerLayerIndicesForReorder(markers, -1, 1, 3), "a signal about no layer at all changes nothing");
    Check(!RenumberMarkerLayerIndicesForReorder(markers, 5, 1, 3),
          "an out-of-range source is rejected rather than trusted");
}

// Regression guard for the `layerIndex`/`layerId` conflation this ticket warns about twice: a
// `markerLayers` fixture with non-contiguous ids must come out byte-identical, since neither repair
// function even takes `markerLayers` as a parameter — they only ever touch `markers`.
void RunLayerIdNeverTouchedChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(3);
    markerLayers[0].layerId = 5;
    markerLayers[1].layerId = 0;
    markerLayers[2].layerId = 9;

    std::vector<Params::MarkerInstanceGroup> markers = MakeMarkersWithLayers();
    ClampMarkerLayerIndicesForRemovedLayer(markers, 1);
    RenumberMarkerLayerIndicesForReorder(markers, 0, 2, 3);

    Check(markerLayers[0].layerId == 5 && markerLayers[1].layerId == 0 && markerLayers[2].layerId == 9,
          "layerId {5, 0, 9} is byte-identical after both repairs run - layerId is never renumbered");
}

// STEP60 §2 Ruling 1: a newly created layer's `layerId` derives as max-plus-one across the current
// in-memory `markerLayers`, never a stored counter - id reuse after a delete is explicitly not a
// hazard.
void RunNextMarkerLayerIdAddDeleteCycleChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(3);
    markerLayers[0].layerId = 5;
    markerLayers[1].layerId = 0;
    markerLayers[2].layerId = 9;
    Check(NextMarkerLayerId(markerLayers) == 10, "ids {5, 0, 9} mint 10 - max-plus-one, not count-based");

    markerLayers.erase(markerLayers.begin() + 2);   // delete the layer carrying id 9
    Check(NextMarkerLayerId(markerLayers) == 6,
          "deleting the 9 row then re-deriving from what remains ({5, 0}) mints 6, not 10 again");
}

// The specific defect part (b)'s ruling exists to prevent: `StepComboInteraction` must not let an
// empty options list write `-1` into a live `layerIndex` - the mirror resolves to -1, but the
// `>= 0` store guard leaves the caller's real value untouched.
void RunComboBindingGuardChecks() {
    ComboOptions emptyOptions;   // labels == nullptr, count == 0
    Params::MarkerTransform transform;
    transform.layerIndex = 0;

    int pickedLayerIndex = transform.layerIndex;   // mirror, not a direct bind
    const WidgetChange change = StepComboInteraction(pickedLayerIndex, emptyOptions, -1);
    Check(pickedLayerIndex == -1, "the mirror resolves to -1 against an empty options list");
    Check(!change.bCommitted || pickedLayerIndex == -1,
          "whatever StepComboInteraction reports, the resolved mirror is -1");

    if (pickedLayerIndex >= 0) transform.layerIndex = pickedLayerIndex;
    Check(transform.layerIndex == 0,
          "the >= 0 store guard leaves transform.layerIndex untouched when the mirror resolved to -1");
}

} // namespace

int main() {
    RunMarkerLayerRemovalChecks();
    RunMarkerLayerReorderRenumberChecks();
    RunMarkerLayerReorderNoOpChecks();
    RunLayerIdNeverTouchedChecks();
    RunNextMarkerLayerIdAddDeleteCycleChecks();
    RunComboBindingGuardChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
