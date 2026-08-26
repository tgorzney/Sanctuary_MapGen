// MarkersTab_TypeSectionHideToggle_UI_Test.cpp — STEP133 acceptance: the right-aligned Hide/Unhide
// button composed into each Type-section header (MarkersTab_UI.cpp), one live headless imgui frame
// (no GL, no window), mirroring LayerEditor_InlineSettings_UI_Test.cpp's own "Add GeoLayer" real
// click-through technique exactly — the SAME `SectionOptions::reservedRightWidth` composition
// pattern, one label wider than "Add GeoLayer" ever is. `MarkersTab_UI_Test.cpp` stays pure-logic
// only (its own header comment), so this control gets its own imgui-including binary, mirroring
// MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's own precedent.
#include "MarkerTypeVisibility_UI.h"
#include "Section_UI.h"
#include <cstdio>
#include <cmath>
#include <imgui.h>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

constexpr unsigned long long kFontAtlasIdentifier = 0xF0000133ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 512.0f);
    io.DeltaTime   = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr;
    int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFontAtlasIdentifier));
    ImGui::NewFrame();
}

constexpr float kHideToggleButtonSpacingPixels = 8.0f;   // mirrors MarkersTab_UI.cpp's own constant

// The EXACT composition MarkersTab_UI.cpp's per-type loop runs, reimplemented here for direct
// item-rect measurement — mirrors LayerEditor_InlineSettings_UI_Test.cpp's own "reserved-width
// formula computed INSIDE the frame" precedent (imgui has no font/style to measure against before
// the first NewFrame of a context).
bool DrawOneTypeSectionFrame(SectionState& sectionState, bool bHidden, ImVec2& outButtonRectMin,
                             ImVec2& outButtonRectMax) {
    SectionOptions options;
    const char* label = bHidden ? "Unhide" : "Hide";
    const float buttonWidth = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    options.reservedRightWidth = buttonWidth + kHideToggleButtonSpacingPixels;

    const bool bBodyVisible = DrawSectionBegin("Alloy", sectionState, options);
    bool bClicked = false;
    if (bBodyVisible) {
        ImGui::SameLine();
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        if (availableWidth > buttonWidth)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);
        bClicked = ImGui::SmallButton(label);
        outButtonRectMin = ImGui::GetItemRectMin();
        outButtonRectMax = ImGui::GetItemRectMax();
        DrawSectionEnd();
    }
    return bClicked;
}

// The button's own right edge lands at the SAME X whether the current label is "Hide" or the wider
// "Unhide" — proven by measuring both states directly, not merely trusting the formula's algebra.
void RunRightAlignmentStableAcrossLabelsCheck() {
    ImGui::CreateContext();
    SectionState sectionState;
    ImVec2 hiddenMin, hiddenMax, visibleMin, visibleMax;

    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("HideToggleTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    DrawOneTypeSectionFrame(sectionState, /*bHidden=*/false, visibleMin, visibleMax);   // "Hide"
    ImGui::End();
    ImGui::Render();

    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("HideToggleTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    DrawOneTypeSectionFrame(sectionState, /*bHidden=*/true, hiddenMin, hiddenMax);   // "Unhide"
    ImGui::End();
    ImGui::Render();

    Check(std::fabs(visibleMax.x - hiddenMax.x) < 0.01f,
          "the button's own right edge lands at the same X for \"Hide\" and the wider \"Unhide\"");
    Check(hiddenMax.x - hiddenMin.x > visibleMax.x - visibleMin.x,
          "sanity: \"Unhide\" really is measured wider than \"Hide\" (else the check above is vacuous)");

    ImGui::DestroyContext();
}

// Clicking the button flips the label the NEXT frame draws — proven end to end through
// MarkerTypeVisibility_UI's own SetHidden/IsHidden, the real state the click writes to in
// MarkersTab_UI.cpp.
void RunClickTogglesLabelAndVisibilityStateCheck() {
    ImGui::CreateContext();
    SectionState sectionState;
    MarkerTypeVisibility_UI visibility;
    ImVec2 rectMin, rectMax;

    // Frame 1: settle at "Hide" (visible), learn the button's screen position.
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("HideToggleTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    DrawOneTypeSectionFrame(sectionState, visibility.IsHidden("Alloy"), rectMin, rectMax);
    ImGui::End();
    ImGui::Render();
    const ImVec2 buttonCenter((rectMin.x + rectMax.x) * 0.5f, (rectMin.y + rectMax.y) * 0.5f);

    // Frame 2 (settle): move the mouse onto the button without pressing yet — Dear ImGui's real
    // Button click-ownership routing keys off the item having been visited/hovered on a PRIOR frame.
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(buttonCenter.x, buttonCenter.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("HideToggleTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    DrawOneTypeSectionFrame(sectionState, visibility.IsHidden("Alloy"), rectMin, rectMax);
    ImGui::End();
    ImGui::Render();

    // Frame 3: press over the button's own rect — held, not yet "clicked". A plain ImGui::Button/
    // SmallButton's default flags fire the click on RELEASE, not press (LayerEditor_InlineSettings_
    // UI_Test.cpp's own RunAddGeoLayerButtonClickThroughChecks establishes this exact precedent for
    // the SAME reserved-right-width composition pattern).
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("HideToggleTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bClickedOnPress = DrawOneTypeSectionFrame(sectionState, visibility.IsHidden("Alloy"), rectMin, rectMax);
    ImGui::End();
    ImGui::Render();
    Check(!bClickedOnPress, "pressing the button does not fire the click on the press frame");
    Check(!visibility.IsHidden("Alloy"), "and the state has not moved yet");

    // Frame 4: release over the same rect — the real click fires here, exactly like
    // MarkersTab_UI.cpp's own call site (SetHidden called the SAME frame SmallButton reports true).
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("HideToggleTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bClicked = DrawOneTypeSectionFrame(sectionState, visibility.IsHidden("Alloy"), rectMin, rectMax);
    ImGui::End();
    ImGui::Render();
    if (bClicked) visibility.SetHidden("Alloy", !visibility.IsHidden("Alloy"));   // the real call site's own line

    Check(bClicked, "a real mouse press+release over the button's own rect clicks it");
    Check(visibility.IsHidden("Alloy"), "the click flips the real MarkerTypeVisibility_UI state to hidden");

    // Frame 5: the next frame's own label is "Unhide" now that the state flipped — confirmed
    // indirectly by RunRightAlignmentStableAcrossLabelsCheck's own measurement above; here the state
    // itself is the acceptance surface.
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("HideToggleTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    DrawOneTypeSectionFrame(sectionState, visibility.IsHidden("Alloy"), rectMin, rectMax);
    ImGui::End();
    ImGui::Render();

    ImGui::DestroyContext();
}

} // namespace

int main() {
    RunRightAlignmentStableAcrossLabelsCheck();
    RunClickTogglesLabelAndVisibilityStateCheck();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
