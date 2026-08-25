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
#include <cmath>
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

// STEP150 acceptance: DrawGroupSettings no longer draws "Group Stratum Index" (Params::GeoLayer::
// stratumIndex has zero PROC consumer -- only the per-Layer control drives generation). Rendering
// itself is "verified by eye against a live frame, never by test" everywhere else in this library
// (TextInput_UI.cpp / SliderScalar_UI.cpp's own words); presence/ABSENCE of a whole row is the one
// thing this ticket needs asserted, so it is measured by total layout height rather than by eye:
// the group body (Name/Mode/Group Blend Mode/Erode Below/Disabled -- the last STEP152's own
// generation-inclusion checkbox, then Separator + "Add Layer") must cost EXACTLY what those
// controls cost standalone, with nothing left over for a removed (or silently added) row.
void RunGroupStratumIndexRemovedCheck() {
    ImGui::CreateContext();

    Params::LayerStack layerStack;
    layerStack.geoLayers.resize(1);           // no layers inside -- isolates group settings +
                                               // Separator + "Add Layer" from anything a layer row
                                               // would itself add to the height.
    LayerEditorState state;
    state.selectedGeoLayerIndex = 0;
    state.selectedLayerIndex    = -1;

    BeginHeadlessFrame();
    ImGui::Begin("GroupStratumIndexActualWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const float actualStartY = ImGui::GetCursorPosY();
    LayerEditorFrameSignals signals;
    DrawLayerEditorGroupBody(layerStack, 0, state, signals, nullptr, nullptr);
    const float actualHeight = ImGui::GetCursorPosY() - actualStartY;
    ImGui::End();
    ImGui::Render();

    // The SAME four controls, drawn standalone, plus the same Separator + button
    // DrawGroupLayerList always draws first — the exact height the real body would have if it drew
    // ONLY these four settings.
    BeginHeadlessFrame();
    ImGui::Begin("GroupStratumIndexReferenceWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const float referenceStartY = ImGui::GetCursorPosY();
    std::string groupName = "GeoLayer";
    DrawTextInput("Name", groupName, LayerEditorNameRules("GeoLayer"));
    Params::GeoLayerMode mode = Params::GeoLayerMode::Material;
    const char* const geoLayerModeLabels[] = { "Material", "Shaper" };
    DrawLayerEditorEnumRow("Mode", mode, geoLayerModeLabels, IM_ARRAYSIZE(geoLayerModeLabels), nullptr);
    Params::HeightBlendMode blendMode = Params::HeightBlendMode::Add;
    const char* const groupBlendModeLabels[] = { "Add", "Subtract", "Multiply", "Overlay",
                                                 "Maximum", "Minimum" };
    DrawLayerEditorEnumRow("Group Blend Mode", blendMode, groupBlendModeLabels,
                           IM_ARRAYSIZE(groupBlendModeLabels), nullptr);
    bool bErodeBelow = false;
    DrawLayerEditorCheckboxRow("Erode Below", bErodeBelow, nullptr);
    bool bDisabled = false;
    DrawLayerEditorCheckboxRow("Disabled", bDisabled, nullptr);
    ImGui::Separator();
    ImGui::SmallButton("Add Layer");
    const float referenceHeight = ImGui::GetCursorPosY() - referenceStartY;
    ImGui::End();
    ImGui::Render();

    CheckLayerEditor(std::fabs(actualHeight - referenceHeight) < 0.5f,
                     "the group body's own height now matches EXACTLY Name+Mode+GroupBlendMode+"
                     "ErodeBelow+Disabled+Separator+Add Layer -- a lingering Group Stratum Index "
                     "row would make the real body taller than this five-control reference");

    ImGui::DestroyContext();
}

// STEP150 acceptance: the Import RAW picker's bound path is synced from the SELECTED layer's OWN
// bakedImagePath every frame that row draws, not left holding whatever some other row (or an
// earlier frame) last put in the shared scratch string.
void RunImportRawPickerSyncChecks() {
    Params::LayerStack layerStack;
    layerStack.geoLayers.resize(1);
    layerStack.geoLayers[0].layers.resize(2);
    layerStack.geoLayers[0].layers[0].bakedImagePath = "C:/SanGenTest/height0.raw";
    layerStack.geoLayers[0].layers[1].bakedImagePath.clear();   // never imported

    LayerEditorState state;
    state.selectedGeoLayerIndex = 0;
    state.selectedLayerIndex    = 0;
    state.importRawPath = "C:/StaleScratch/whatever-some-other-row-picked.raw";   // poison

    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::Begin("ImportRawSyncWindow1", nullptr, ImGuiWindowFlags_NoSavedSettings);
    LayerEditorFrameSignals signalsRowZero;
    DrawLayerEditorGroupBody(layerStack, 0, state, signalsRowZero, nullptr, nullptr);
    ImGui::End();
    ImGui::Render();
    CheckLayerEditor(state.importRawPath == layerStack.geoLayers[0].layers[0].bakedImagePath,
                     "the picker's bound path is synced from the SELECTED row's own bakedImagePath");

    state.selectedLayerIndex = 1;
    BeginHeadlessFrame();
    ImGui::Begin("ImportRawSyncWindow2", nullptr, ImGuiWindowFlags_NoSavedSettings);
    LayerEditorFrameSignals signalsRowOne;
    DrawLayerEditorGroupBody(layerStack, 0, state, signalsRowOne, nullptr, nullptr);
    ImGui::End();
    ImGui::Render();
    CheckLayerEditor(state.importRawPath.empty(),
                     "and a layer that was never imported syncs to a genuine empty path (the "
                     "picker's own ShortenedFilePathLabel renders that as \"(none)\")");

    ImGui::DestroyContext();
}

// STEP150 acceptance: Noise/Density/HeightBlend (and, drawn the same way, Soil/Erosion) never run
// for a baked layer. Reuses RunInlineSettingsChecks' own poison-the-mirror technique: Height
// Blending's DrawHeightBlendSection only calls LoadLayerEditorValues (which would clear the poison)
// when it actually draws, so the poison surviving a frame IS the proof the whole gated block was
// skipped.
void RunProceduralSectionsGatedWhenBakedChecks() {
    Params::LayerStack layerStack;
    layerStack.geoLayers.resize(1);
    layerStack.geoLayers[0].layers.resize(1);
    Params::Layer& layer = layerStack.geoLayers[0].layers[0];
    layer.levelsShadows = 0.42f;      // a real, distinguishable value a live draw WOULD load
    layer.bBaked = true;

    LayerEditorState state;
    state.selectedGeoLayerIndex = 0;
    state.selectedLayerIndex    = 0;
    state.levelsValues.inputShadows = -999.0f;   // poison

    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::Begin("BakedGateWindow1", nullptr, ImGuiWindowFlags_NoSavedSettings);
    LayerEditorFrameSignals signalsBaked;
    DrawLayerEditorGroupBody(layerStack, 0, state, signalsBaked, nullptr, nullptr);
    ImGui::End();
    ImGui::Render();
    CheckLayerEditor(state.levelsValues.inputShadows == -999.0f,
                     "a baked layer's Height Blending section (and, gated the same way, Noise/"
                     "Density/Soil/Erosion) never draws, so the poisoned mirror survives the frame");

    layer.bBaked = false;
    BeginHeadlessFrame();
    ImGui::Begin("BakedGateWindow2", nullptr, ImGuiWindowFlags_NoSavedSettings);
    LayerEditorFrameSignals signalsUnbaked;
    DrawLayerEditorGroupBody(layerStack, 0, state, signalsUnbaked, nullptr, nullptr);
    ImGui::End();
    ImGui::Render();
    CheckLayerEditor(state.levelsValues.inputShadows == 0.42f,
                     "and the same layer, once unbaked, draws Height Blending again and reloads it");

    ImGui::DestroyContext();
}

// STEP152 acceptance: the group-level "Disabled" checkbox (DrawGroupSettings) reaches
// Params::GeoLayer::bDisabled by REFERENCE, not a copy that never lands on the recipe -- proved
// with a real mouse press over the checkbox's own rect (found by replicating DrawGroupSettings'
// own widget sequence with throwaway locals first, the same "reference" technique
// RunGroupStratumIndexRemovedCheck already relies on -- a checkbox's geometry depends only on its
// label, never its current value, so the two sequences lay out identically).
void RunDisabledCheckboxCommitChecks() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.AddMousePosEvent(-1.0f, -1.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f));
    ImGui::Begin("DisabledCheckboxRectWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    std::string groupName;
    DrawTextInput("Name", groupName, LayerEditorNameRules("GeoLayer"));
    Params::GeoLayerMode mode = Params::GeoLayerMode::Material;
    const char* const geoLayerModeLabels[] = { "Material", "Shaper" };
    DrawLayerEditorEnumRow("Mode", mode, geoLayerModeLabels, IM_ARRAYSIZE(geoLayerModeLabels), nullptr);
    Params::HeightBlendMode blendMode = Params::HeightBlendMode::Add;
    const char* const groupBlendModeLabels[] = { "Add", "Subtract", "Multiply", "Overlay",
                                                 "Maximum", "Minimum" };
    DrawLayerEditorEnumRow("Group Blend Mode", blendMode, groupBlendModeLabels,
                           IM_ARRAYSIZE(groupBlendModeLabels), nullptr);
    bool bErodeBelowReference = false;
    DrawLayerEditorCheckboxRow("Erode Below", bErodeBelowReference, nullptr);
    bool bDisabledReference = false;
    DrawLayerEditorCheckboxRow("Disabled", bDisabledReference, nullptr);
    const ImVec2 checkboxRectMin = ImGui::GetItemRectMin();
    const ImVec2 checkboxRectMax = ImGui::GetItemRectMax();
    ImGui::End();
    ImGui::Render();

    Params::LayerStack layerStack;
    layerStack.geoLayers.resize(1);
    LayerEditorState state;
    state.selectedGeoLayerIndex = 0;
    state.selectedLayerIndex    = -1;
    CheckLayerEditor(!layerStack.geoLayers[0].bDisabled, "a fresh GeoLayer starts enabled for generation");

    auto drawRealFrame = [&]() {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f));
        ImGui::Begin("DisabledCheckboxRectWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
        LayerEditorFrameSignals signals;
        DrawLayerEditorGroupBody(layerStack, 0, state, signals, nullptr, nullptr);
        ImGui::End();
        ImGui::Render();
    };

    const float checkboxCenterX = (checkboxRectMin.x + checkboxRectMax.x) * 0.5f;
    const float checkboxCenterY = (checkboxRectMin.y + checkboxRectMax.y) * 0.5f;

    // Settle: mouse arrives, not yet pressed.
    io.AddMousePosEvent(checkboxCenterX, checkboxCenterY);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    drawRealFrame();
    CheckLayerEditor(!layerStack.geoLayers[0].bDisabled, "hovering the checkbox does not commit yet");

    // Press: TickBoxWasClicked reads ImGui::IsItemClicked(), which (unlike a Button's own return
    // value) fires on the PRESS frame, not release -- the commit lands here.
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    drawRealFrame();

    CheckLayerEditor(layerStack.geoLayers[0].bDisabled,
                     "a real mouse press over the Disabled checkbox's own rect reaches "
                     "Params::GeoLayer::bDisabled");

    ImGui::DestroyContext();
}

// STEP152 §7 diagnostics acceptance: the "no active procedural layer" status line draws exactly
// when HasActiveProceduralLayer() is false, adding exactly one status line's worth of height and
// nothing else -- reusing RunGroupStratumIndexRemovedCheck's own height-diff discipline (the
// status line's text itself is "verified by eye against a live frame, never by test", same as
// every other draw-path half in this library).
void RunProceduralGatingDiagnosticChecks() {
    ImGui::CreateContext();

    auto heightOfSingleLayerGroup = [](bool bLayerDisabled) {
        Params::LayerStack layerStack;
        layerStack.geoLayers.resize(1);
        layerStack.geoLayers[0].layers.resize(1);
        layerStack.geoLayers[0].layers[0].bDisabled = bLayerDisabled;
        LayerEditorState state;
        state.selectedGeoLayerIndex = 0;
        state.selectedLayerIndex    = 0;

        BeginHeadlessFrame();
        ImGui::Begin("ProceduralGatingDiagnosticWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
        const float startY = ImGui::GetCursorPosY();
        LayerEditorFrameSignals signals;
        DrawLayerEditorGroupBody(layerStack, 0, state, signals, nullptr, nullptr);
        const float height = ImGui::GetCursorPosY() - startY;
        ImGui::End();
        ImGui::Render();
        return height;
    };

    const float activeHeight   = heightOfSingleLayerGroup(/*bLayerDisabled=*/false);
    const float inactiveHeight = heightOfSingleLayerGroup(/*bLayerDisabled=*/true);
    const float oneTextLineHeight = ImGui::GetTextLineHeightWithSpacing();
    CheckLayerEditor(inactiveHeight > activeHeight,
                     "an inactive stack's row draws the status line the active one does not");
    CheckLayerEditor((inactiveHeight - activeHeight) < oneTextLineHeight * 2.0f,
                     "and it costs exactly one status line, not a whole extra section");

    ImGui::DestroyContext();
}

} // namespace

void RunLayerEditorInlineSettingsChecks() {
    RunInlineSettingsChecks();
    RunAddGeoLayerButtonClickThroughChecks();
    RunGroupStratumIndexRemovedCheck();
    RunImportRawPickerSyncChecks();
    RunProceduralSectionsGatedWhenBakedChecks();
    RunDisabledCheckboxCommitChecks();
    RunProceduralGatingDiagnosticChecks();
}
