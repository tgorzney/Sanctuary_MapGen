// MarkersTab_ManualInstanceListRows_UI_Test.cpp — STEP126 headless-imgui acceptance coverage for
// DrawLayerRowBody's new per-Layer instance list (Open Q7). Mirrors
// MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's HeadlessImguiSession/RunHeadlessFrame
// harness (ListWidget_TestFrame_UI.h). This is a leaf imgui function, testable standalone — no
// DraggableList::Render wrapper needed.
//
// Row positions are located the same way MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's
// own SwatchCenter helper locates a sibling widget: from the LAST item's own rect (imgui's own
// GetItemRectMin/Max after the call — the last-drawn instance row, or "(none)" when the layer has
// no matching instances) plus the block's own known ItemSpacing, rather than reaching into imgui
// internals to enumerate every item drawn this frame.
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// DrawLayerRowBody draws its full body above the Instances list (name, color override, icon scale,
// grid snap, symmetry section) before the two Selectable rows this test clicks -- tall enough that a
// too-short window clips them out of the hoverable/clickable area even though GetItemRectMin/Max
// still reports a valid (but off-window) rect. 700px gives comfortable margin over the ~470px this
// fixture's two-row case actually reaches.
const ImVec2 kWindowSize = ImVec2(400.0f, 700.0f);

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

struct FrameResult {
    ImVec2 lastItemMin;
    ImVec2 lastItemMax;
    ImGuiID lastItemId  = 0;
    bool    bReturned   = false;
};

FrameResult RunRowBodyFrame(HeadlessMouseState mouse, Params::MarkerInstanceLayer& layer,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers,
                            const ManualInstanceLayerIndex_UI& instanceIndex, ManualMarkerLayersState& state,
                            int& selectedManualInstanceIdentifier) {
    FrameResult result;
    const Params::Geometry geometry;
    Params::MarkerSymmetryFixSettings symmetryFixSettings;
    RunHeadlessFrame(mouse, kWindowSize, [&] {
        result.bReturned = DrawLayerRowBody(layer, /*layerIndex=*/0, markerLayers, markers, geometry,
                                            Params::SymmetryAxis::None, 3, symmetryFixSettings, state,
                                            instanceIndex, selectedManualInstanceIdentifier);
        result.lastItemMin = ImGui::GetItemRectMin();
        result.lastItemMax = ImGui::GetItemRectMax();
        result.lastItemId  = ImGui::GetItemID();
    });
    return result;
}

FrameResult ClickAt(ImVec2 position, Params::MarkerInstanceLayer& layer,
                    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                    std::vector<Params::MarkerInstanceGroup>& markers,
                    const ManualInstanceLayerIndex_UI& instanceIndex, ManualMarkerLayersState& state,
                    int& selectedManualInstanceIdentifier) {
    HeadlessMouseState hover;   hover.position = position;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunRowBodyFrame(hover, layer, markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier);
    const FrameResult pressResult =
        RunRowBodyFrame(press, layer, markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier);
    RunRowBodyFrame(release, layer, markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier);
    return pressResult;
}

// Two transforms tagged layerIndex 0 (distinct instanceIdentifiers 100/101) and one transform
// tagged layerIndex 1 (identifier 200, a DIFFERENT layer). Clicking each of layer 0's own rows sets
// selectedManualInstanceIdentifier to that row's own identifier and nothing else; the function's own
// return value stays false (a selection click never triggers MakeNamesUnique).
void RunInstanceRowClickChecks() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Resources";
    Params::MarkerTransform first;  first.name = "A";  first.layerIndex = 0; first.instanceIdentifier = 100;
    Params::MarkerTransform second; second.name = "B"; second.layerIndex = 0; second.instanceIdentifier = 101;
    Params::MarkerTransform other;  other.name = "C";  other.layerIndex = 1; other.instanceIdentifier = 200;
    markers[0].transforms.push_back(first);
    markers[0].transforms.push_back(second);
    markers[0].transforms.push_back(other);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    Check(instanceIndex.instancesByLayerIndex.at(0).size() == 2u,
          "the index maps exactly 2 (group,transform) pairs to layerIndex 0 — the layerIndex-1 transform is excluded");

    ManualMarkerLayersState state;
    int selectedManualInstanceIdentifier = -1;

    const FrameResult settle = RunRowBodyFrame(HeadlessMouseState(), markerLayers[0], markerLayers, markers,
                                               instanceIndex, state, selectedManualInstanceIdentifier);
    Check(settle.lastItemId != 0, "the last-drawn item is a real, interactive Selectable row, not inert text");
    const float rowHeight  = settle.lastItemMax.y - settle.lastItemMin.y;
    const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
    const ImVec2 secondRowCenter((settle.lastItemMin.x + settle.lastItemMax.x) * 0.5f,
                                 (settle.lastItemMin.y + settle.lastItemMax.y) * 0.5f);
    const ImVec2 firstRowCenter(secondRowCenter.x, secondRowCenter.y - (rowHeight + rowSpacing));

    const FrameResult firstClick = ClickAt(firstRowCenter, markerLayers[0], markerLayers, markers,
                                           instanceIndex, state, selectedManualInstanceIdentifier);
    Check(selectedManualInstanceIdentifier == 100, "clicking the first row selects its own instanceIdentifier (100)");
    Check(!firstClick.bReturned, "a selection click never sets the function's own commit return value");

    const FrameResult secondClick = ClickAt(secondRowCenter, markerLayers[0], markerLayers, markers,
                                            instanceIndex, state, selectedManualInstanceIdentifier);
    Check(selectedManualInstanceIdentifier == 101, "clicking the SECOND row updates to its own instanceIdentifier (101), not additive/toggled");
    Check(!secondClick.bReturned, "and still reports no commit");
}

// A layer with zero matching instances in the index renders "(none)" — the last item is inert
// (imgui text widgets push no interactive ID), never a clickable Selectable.
void RunNoInstancesRendersNoneChecks() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    Params::MarkerTransform onlyOther;
    onlyOther.layerIndex = 1;   // never layerIndex 0
    onlyOther.instanceIdentifier = 5;
    markers[0].transforms.push_back(onlyOther);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    Check(instanceIndex.instancesByLayerIndex.find(0) == instanceIndex.instancesByLayerIndex.end(),
          "layerIndex 0 has no entry at all in the index (find() == end(), not an empty vector)");

    ManualMarkerLayersState state;
    int selectedManualInstanceIdentifier = -1;
    const FrameResult settle = RunRowBodyFrame(HeadlessMouseState(), markerLayers[0], markerLayers, markers,
                                               instanceIndex, state, selectedManualInstanceIdentifier);
    Check(settle.lastItemId == 0, "the last-drawn item is inert (\"(none)\" text), not an interactive Selectable");

    const ImVec2 rowCenter((settle.lastItemMin.x + settle.lastItemMax.x) * 0.5f,
                           (settle.lastItemMin.y + settle.lastItemMax.y) * 0.5f);
    ClickAt(rowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier);
    Check(selectedManualInstanceIdentifier == -1, "clicking where a row would have been does nothing — no row exists");
}

} // namespace

int main() {
    RunInstanceRowClickChecks();
    RunNoInstancesRendersNoneChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
