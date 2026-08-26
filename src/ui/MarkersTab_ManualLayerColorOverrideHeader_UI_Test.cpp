// MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp — STEP123 acceptance coverage for
// DrawManualMarkerLayerColorOverrideHeaderControl (MarkersTab_ManualLayerRowBody_UI.cpp): the row
// header's own Color Override checkbox + compact swatch. `MarkersTab_ManualLayers_UI_Test.cpp` is
// pure-logic only, no imgui frame (per its own header comment), so this control gets its own
// headless-frame test file, mirroring `ListWidget_TestFrame_UI.h`'s `HeadlessImguiSession`/
// `RunHeadlessFrame` harness the same way `DraggableListWidget_UI_Test.cpp` does. This is a leaf
// imgui function, testable standalone — no `DraggableList::Render` wrapper needed.
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

const ImVec2 kWindowSize = ImVec2(300.0f, 100.0f);

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// One frame's geometry (the control's own top-left corner and the checkbox's own square size --
// both needed to compute a synthetic click point) plus the function's own out-param.
struct FrameResult {
    ImVec2 origin;
    float  boxSize      = 0.0f;
    bool   bAnyCommitted = false;
};

FrameResult RunHeaderControlFrame(HeadlessMouseState mouse, Params::MarkerInstanceLayer& layer,
                                  ManualMarkerLayersState& state) {
    FrameResult result;
    RunHeadlessFrame(mouse, kWindowSize, [&] {
        result.origin  = ImGui::GetCursorScreenPos();
        result.boxSize = ResolveWidgetTrackHeight(WidgetStyle());
        DrawManualMarkerLayerColorOverrideHeaderControl(layer, state, result.bAnyCommitted);
    });
    return result;
}

// Hover, press, release -- mirrors DraggableList_TestScene_UI.h's ClickAt pattern. A plain checkbox
// has no AllowOverlap hover requirement, but matching the established click helper shape costs
// nothing (per the ticket). `layer`/`state` are re-drawn on every frame of the sequence, exactly as
// the real DraggableList row header does.
FrameResult ClickAt(ImVec2 position, Params::MarkerInstanceLayer& layer, ManualMarkerLayersState& state) {
    HeadlessMouseState hover;   hover.position = position;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunHeaderControlFrame(hover, layer, state);
    const FrameResult pressResult = RunHeaderControlFrame(press, layer, state);
    RunHeaderControlFrame(release, layer, state);
    return pressResult;   // TickBoxWasClicked uses IsItemClicked(), which fires on the PRESS frame
}

// Clicking the checkbox flips the real field, independent of any row-expand state (this function
// has no expand state at all -- it draws unconditionally), and reports the commit out-param.
void RunCheckboxFlipCheck() {
    HeadlessImguiSession session;
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    layer.bColorOverrideEnabled = false;

    const FrameResult settle = RunHeaderControlFrame(HeadlessMouseState(), layer, state);
    const ImVec2 checkboxCenter(settle.origin.x + settle.boxSize * 0.5f,
                                settle.origin.y + settle.boxSize * 0.5f);

    const FrameResult clicked = ClickAt(checkboxCenter, layer, state);

    Check(layer.bColorOverrideEnabled, "clicking the header checkbox flips bColorOverrideEnabled");
    Check(clicked.bAnyCommitted, "and the function's bAnyCommitted out-param went true");
}

// state.bUseGroupColor wraps the whole control in BeginDisabled -- a click at the SAME coordinates
// must do nothing, proving the control actually goes inert, not merely visually grayed.
void RunInertWhileGroupColorForcedCheck() {
    HeadlessImguiSession session;
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    layer.bColorOverrideEnabled = false;
    state.bUseGroupColor = true;

    const FrameResult settle = RunHeaderControlFrame(HeadlessMouseState(), layer, state);
    const ImVec2 checkboxCenter(settle.origin.x + settle.boxSize * 0.5f,
                                settle.origin.y + settle.boxSize * 0.5f);

    const FrameResult clicked = ClickAt(checkboxCenter, layer, state);

    Check(!layer.bColorOverrideEnabled,
          "state.bUseGroupColor disables the checkbox -- a click at the same coordinates does nothing");
    Check(!clicked.bAnyCommitted, "and reports no commit either");
}

// The swatch button sits one checkbox-width + ItemSpacing to the right of the checkbox (the
// checkbox's own label is empty, so it contributes no extra width) -- same row, same Y.
ImVec2 SwatchCenter(const FrameResult& settle) {
    const float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
    const float swatchLeftX  = settle.origin.x + settle.boxSize + itemSpacingX;
    const float swatchCenterX = swatchLeftX + kMarkerLayerColorOverrideSwatchWidthPixels * 0.5f;
    const float rowCenterY    = settle.origin.y + settle.boxSize * 0.5f;
    return ImVec2(swatchCenterX, rowCenterY);
}

bool AnyPopupOpen() {
    return ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
}

// With bColorOverrideEnabled == false the swatch is disabled (not hidden) -- a click on its known
// position must not open the picker popup. With bColorOverrideEnabled == true the SAME coordinates
// (layout is unaffected by the enabled flag) must be clickable and open the picker.
void RunSwatchDisabledEnabledCheck() {
    HeadlessImguiSession session;
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    layer.bColorOverrideEnabled = false;

    const FrameResult settle = RunHeaderControlFrame(HeadlessMouseState(), layer, state);
    const ImVec2 swatchCenter = SwatchCenter(settle);

    ClickAt(swatchCenter, layer, state);
    RunHeaderControlFrame(HeadlessMouseState(), layer, state);   // settle: read IsPopupOpen the frame after
    Check(!AnyPopupOpen(),
          "clicking the swatch while bColorOverrideEnabled is false does not open the picker (disabled)");

    layer.bColorOverrideEnabled = true;
    ClickAt(swatchCenter, layer, state);
    RunHeaderControlFrame(HeadlessMouseState(), layer, state);   // settle: read IsPopupOpen the frame after
    Check(AnyPopupOpen(),
          "and IS clickable once bColorOverrideEnabled is true -- opens the picker popup");
}

} // namespace

int main() {
    RunCheckboxFlipCheck();
    RunInertWhileGroupColorForcedCheck();
    RunSwatchDisabledEnabledCheck();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
