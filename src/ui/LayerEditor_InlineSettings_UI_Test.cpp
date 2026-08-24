// LayerEditor_InlineSettings_UI_Test.cpp — STEP104 acceptance: the Layer Editor's per-row inline
// settings (Fix part 1) and the "Add GeoLayer" button's real click-through once repositioned onto
// the "GeoLayers" section header's reserved-right-width gap (Fix part 2). The one imgui-including
// half of the LayerEditor_UI_Test binary — mirrors MapCanvas_MarkerDrag_UI_Test.cpp's own "one live
// headless imgui frame, no window/GL" technique; every other translation unit in this binary stays
// pure. main() lives in LayerEditor_UI_Test.cpp.
#include "LayerEditor_Draw_UI.h"
#include "LayerEditor_TestSupport_UI.h"
#include "LayerEditor_UI.h"
#include "Section_UI.h"
#include <cstdio>
#include <imgui.h>
#include <string>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

constexpr unsigned long long kInlineSettingsFontAtlasIdentifier = 0xF0000105ull;

// One imgui frame with no renderer backend, mirroring MapCanvas_Render_UI_Test.cpp's own
// BeginHeadlessFrame exactly: the font atlas is built the legacy way (GetTexDataAsRGBA32 auto-adds
// a default font if none exists) and the frame is only rendered into draw data / item rects, never
// presented. `io.DisplaySize`/`DeltaTime` are harmless to re-set every frame, same as that precedent.
void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 512.0f);
    io.DeltaTime   = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr;
    int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kInlineSettingsFontAtlasIdentifier));
    ImGui::NewFrame();
}

// The exact row label DrawGroupLayerList (LayerEditor_Group_UI.cpp) builds for a row, replicated
// here ONLY so this test can pre-seed ImGui's own per-header open/closed storage without having to
// drive a mouse-click sequence just to collapse a row.
std::string RowLabelFor(const Params::Layer& layer) {
    char rowLabel[72] = { 0 };
    std::snprintf(rowLabel, sizeof(rowLabel), "%s (stratum %d)", LayerEditorRowLabel(layer), layer.stratumIndex);
    return std::string(rowLabel);
}

// DraggableList<Params::Layer>::Render pushes "layers" -> rowIndex before drawing a row's own
// CollapsingHeader (DraggableListWidget_UI.h) — replicated at the SAME id-stack depth this test
// calls DrawLayerEditorGroupBody from (the window's top level, no extra wrapping PushID), so the
// preset lands on the exact id CollapsingHeader itself will read.
void SetRowCollapsingHeaderOpen(int rowIndex, const Params::Layer& layer, bool bOpen) {
    ImGui::PushID("layers");
    ImGui::PushID(rowIndex);
    const ImGuiID headerId = ImGui::GetID(RowLabelFor(layer).c_str());
    ImGui::PopID();
    ImGui::PopID();
    ImGui::GetStateStorage()->SetInt(headerId, bOpen ? 1 : 0);
}

Params::LayerStack TwoLayerGeoStack() {
    Params::LayerStack layerStack;
    layerStack.geoLayers.resize(1);
    layerStack.geoLayers[0].layers.resize(2);
    layerStack.geoLayers[0].layers[0].frequency     = 0.01f;
    layerStack.geoLayers[0].layers[0].levelsShadows = 0.11f;
    layerStack.geoLayers[0].layers[1].frequency     = 0.99f;
    layerStack.geoLayers[0].layers[1].levelsShadows = 0.77f;
    return layerStack;
}

// Scenarios 1 + 2 (STEP104 acceptance test): each row's own body shows ITS OWN settings — never
// bled from `state.selectedLayerIndex`, which this test deliberately pins at the OTHER row
// throughout — and a collapsed row draws no settings at all.
void RunInlineSettingsChecks() {
    Params::LayerStack layerStack = TwoLayerGeoStack();
    Params::GeoLayer& group = layerStack.geoLayers[0];
    LayerEditorState state;
    state.selectedGeoLayerIndex = 0;
    state.selectedLayerIndex    = 0;   // pinned at row 0 for every frame below, on purpose

    ImGui::CreateContext();

    // Frame 1: both rows default-open (ImGuiTreeNodeFlags_DefaultOpen, first time each id is
    // seen). If the pre-STEP104 "draw whatever selectedLayerIndex names" behavior had survived,
    // every row would show row 0's values; instead the row-body lambda indexes by its OWN rowIndex.
    BeginHeadlessFrame();
    ImGui::Begin("InlineSettingsTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    LayerEditorFrameSignals signalsBothOpen;
    DrawLayerEditorGroupBody(layerStack, 0, state, signalsBothOpen, nullptr, nullptr);
    ImGui::End();
    ImGui::Render();
    CheckLayerEditor(state.levelsValues.inputShadows == group.layers[1].levelsShadows,
                     "row 1, drawn last, shows its OWN settings, not row 0's ('selected') values");

    // Frame 2: collapse row 1, leave row 0 open. Poison the mirror first so a stray draw is caught.
    state.levelsValues.inputShadows = -999.0f;
    BeginHeadlessFrame();
    ImGui::Begin("InlineSettingsTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    SetRowCollapsingHeaderOpen(1, group.layers[1], false);
    LayerEditorFrameSignals signalsRowOneClosed;
    DrawLayerEditorGroupBody(layerStack, 0, state, signalsRowOneClosed, nullptr, nullptr);
    ImGui::End();
    ImGui::Render();
    CheckLayerEditor(state.levelsValues.inputShadows == group.layers[0].levelsShadows,
                     "with row 1 collapsed, only row 0's OWN (distinct) settings render");
    CheckLayerEditor(state.levelsValues.inputShadows != group.layers[1].levelsShadows,
                     "row 1's now-hidden settings never touch the mirror");

    // Frame 3: collapse BOTH rows. No settings render at all.
    state.levelsValues.inputShadows = -999.0f;
    BeginHeadlessFrame();
    ImGui::Begin("InlineSettingsTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    SetRowCollapsingHeaderOpen(0, group.layers[0], false);
    SetRowCollapsingHeaderOpen(1, group.layers[1], false);
    LayerEditorFrameSignals signalsBothClosed;
    DrawLayerEditorGroupBody(layerStack, 0, state, signalsBothClosed, nullptr, nullptr);
    ImGui::End();
    ImGui::Render();
    CheckLayerEditor(state.levelsValues.inputShadows == -999.0f,
                     "collapsing every row in the GeoLayer draws no settings at all");

    ImGui::DestroyContext();
}

// Scenario 4 (STEP104 acceptance test): the "Add GeoLayer" button, repositioned onto the
// "GeoLayers" header's reserved-right-width gap, still fires the real Add action on a genuine
// simulated click — not merely a positioning assertion. Exercises the SAME sequence
// HeightmapTab_UI.cpp's call site composes: DrawSectionBegin with a reserved gap, SameLine, the
// button, then its OWN this-frame return value fed straight into DrawLayerEditor through
// `bAddGeoLayerRequestedExternally` (bDrawOwnAddGeoLayerButton = false) — mirroring
// CoreInputWidgets_LiveFrame_UI_Test.cpp's real press/release-over-the-item-rect technique. A
// settle frame (mouse moved onto the button, not yet pressed) runs before the press frame — Dear
// ImGui's click-ownership routing for a real Button (unlike a raw InvisibleButton's IsItemActive())
// keys off the item having been visited/hovered on a PRIOR frame, same as a real mouse naturally
// arriving on a widget before it is clicked.
void RunAddGeoLayerButtonClickThroughChecks() {
    Params::LayerStack layerStack;
    LayerEditorState state;
    const std::size_t groupCountBefore = layerStack.geoLayers.size();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    SectionState geoLayerSection;
    ImVec2 buttonRectMin(0.0f, 0.0f), buttonRectMax(0.0f, 0.0f);

    // One frame of the EXACT sequence HeightmapTab_UI.cpp's call site composes. The reserved-width
    // formula (font-metric calls, CalcTextSize/GetStyle) is computed INSIDE the frame, same as
    // HeightmapTab_UI.cpp's own GeoLayerSectionOptions() — imgui has no font/style to measure
    // against before the first NewFrame of a context.
    auto drawFrame = [&]() {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f));
        ImGui::Begin("AddGeoLayerTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
        SectionOptions geoLayerSectionOptions;
        constexpr float kAddButtonSpacingPixels = 8.0f;
        geoLayerSectionOptions.reservedRightWidth =
            ImGui::CalcTextSize("Add GeoLayer").x + ImGui::GetStyle().FramePadding.x * 2.0f
            + kAddButtonSpacingPixels;
        CheckLayerEditor(DrawSectionBegin("GeoLayers", geoLayerSection, geoLayerSectionOptions),
                         "the GeoLayers header opens");
        ImGui::SameLine();
        const bool bClicked = ImGui::SmallButton("Add GeoLayer");
        buttonRectMin = ImGui::GetItemRectMin();
        buttonRectMax = ImGui::GetItemRectMax();
        DrawLayerEditor(layerStack, state, nullptr, nullptr, /*bDrawOwnAddGeoLayerButton=*/false, bClicked);
        DrawSectionEnd();
        ImGui::End();
        ImGui::Render();
        return bClicked;
    };

    // Frame 1: mouse away, purely to learn the button's own rect (SmallButton is called directly
    // here, exactly as HeightmapTab_UI.cpp calls it, so its rect is OUR last item, not buried
    // inside DrawLayerEditor's own widgets).
    io.AddMousePosEvent(-1.0f, -1.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    CheckLayerEditor(!drawFrame(), "not hovered, so no click yet");
    CheckLayerEditor(layerStack.geoLayers.size() == groupCountBefore, "nothing was added");

    const float buttonCenterX = (buttonRectMin.x + buttonRectMax.x) * 0.5f;
    const float buttonCenterY = (buttonRectMin.y + buttonRectMax.y) * 0.5f;

    // Settle frame: move the mouse onto the button without pressing yet.
    io.AddMousePosEvent(buttonCenterX, buttonCenterY);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    drawFrame();

    // Frame 2: press over the button's own rect — held, not yet "clicked" (imgui's default button
    // behavior fires the click on RELEASE, not press).
    io.AddMousePosEvent(buttonCenterX, buttonCenterY);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    CheckLayerEditor(!drawFrame(), "pressing the button does not fire the click on the press frame");
    CheckLayerEditor(layerStack.geoLayers.size() == groupCountBefore, "and still nothing was added");

    // Frame 3: release over the same rect — the real click fires here, and DrawLayerEditor is fed
    // it THIS SAME FRAME (drawFrame's own body), exactly like HeightmapTab_UI.cpp's call site.
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    CheckLayerEditor(drawFrame(), "a real mouse press+release over the button's own rect clicks it");

    CheckLayerEditor(layerStack.geoLayers.size() == groupCountBefore + 1,
                     "the real click reaches RecordLayerEditorAction/AddGeoLayer through the new wiring");
    CheckLayerEditor(state.selectedGeoLayerIndex == static_cast<int>(layerStack.geoLayers.size()) - 1,
                     "and the new group is selected, same as before this ticket moved the button");

    ImGui::DestroyContext();
}

} // namespace

void RunLayerEditorInlineSettingsChecks() {
    RunInlineSettingsChecks();
    RunAddGeoLayerButtonClickThroughChecks();
}
