// MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp — STEP123 acceptance coverage for
// DrawManualMarkerLayerColorOverrideHeaderControl (MarkersTab_ManualLayerRowBody_UI.cpp): the row
// header's own Color Override checkbox + compact swatch. `MarkersTab_ManualLayers_UI_Test.cpp` is
// pure-logic only, no imgui frame (per its own header comment), so this control gets its own
// headless-frame test file, mirroring `ListWidget_TestFrame_UI.h`'s `HeadlessImguiSession`/
// `RunHeadlessFrame` harness the same way `DraggableListWidget_UI_Test.cpp` does. This is a leaf
// imgui function, testable standalone — no `DraggableList::Render` wrapper needed.
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include <cmath>
#include <cstdio>
#include <vector>

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
                                  ManualMarkerLayersState& state,
                                  const std::vector<Params::MarkerLink>& links = {}) {
    FrameResult result;
    RunHeadlessFrame(mouse, kWindowSize, [&] {
        result.origin  = ImGui::GetCursorScreenPos();
        result.boxSize = ResolveWidgetTrackHeight(WidgetStyle());
        DrawManualMarkerLayerColorOverrideHeaderControl(layer, state, result.bAnyCommitted, links);
    });
    return result;
}

// Hover, press, release -- mirrors DraggableList_TestScene_UI.h's ClickAt pattern. `layer`/`state`
// are re-drawn on every frame of the sequence, exactly as the real DraggableList row header does.
// STEP142 converted these controls from plain Checkbox to ImGui::SmallButton (human's own
// instruction: no more checkboxes) — SmallButton (ButtonEx's default flags) reports pressed on the
// RELEASE-while-hovered frame, not the press/mouse-down frame, so the caller reads THAT frame's
// result now (mirrors MarkersTab_Bundles_UI_Test.cpp's own TestManualLeafHeaderExtraDrawsAndFlips-
// Symmetry fix for the identical gotcha).
FrameResult ClickAt(ImVec2 position, Params::MarkerInstanceLayer& layer, ManualMarkerLayersState& state) {
    HeadlessMouseState hover;   hover.position = position;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunHeaderControlFrame(hover, layer, state);
    RunHeaderControlFrame(press, layer, state);
    return RunHeaderControlFrame(release, layer, state);   // SmallButton fires on release, not press
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

// STEP130 (ARCH §19.24) — DrawMarkerLayerSymmetryToggleHeaderControl, standalone: mirrors
// RunCheckboxFlipCheck exactly, one tier over. Clicking flips bSymmetryEnabled and reports the
// commit out-param; `symmetry`'s own fields are untouched by the click (the non-destructive-gate
// claim ARCH §19.24 makes — toggling the ENABLE bit never mutates the configured mask itself).
void RunSymmetryToggleFlipCheck() {
    HeadlessImguiSession session;
    Params::MarkerInstanceLayer layer;
    layer.bSymmetryEnabled = true;
    layer.symmetry.bSymmetryUseGlobal      = false;
    layer.symmetry.symmetryMask            = Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::Radial;
    layer.symmetry.radialSymmetryRepeatCount = 7;

    bool bAnyCommitted = false;
    ImVec2 origin; float boxSize = 0.0f;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        origin  = ImGui::GetCursorScreenPos();
        boxSize = ResolveWidgetTrackHeight(WidgetStyle());
        DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted);
    });
    const ImVec2 checkboxCenter(origin.x + boxSize * 0.5f, origin.y + boxSize * 0.5f);

    HeadlessMouseState hover;   hover.position = checkboxCenter;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    auto runFrame = [&](HeadlessMouseState mouse) {
        bool bCommitted = false;
        RunHeadlessFrame(mouse, kWindowSize, [&] { DrawMarkerLayerSymmetryToggleHeaderControl(layer, bCommitted); });
        return bCommitted;
    };
    runFrame(hover);
    runFrame(press);
    const bool bReleaseCommitted = runFrame(release);   // SmallButton fires on release, not press

    Check(!layer.bSymmetryEnabled, "clicking the Symmetry header checkbox flips bSymmetryEnabled false");
    Check(bReleaseCommitted, "and reports a commit on the release frame (SmallButton)");
    Check(!layer.symmetry.bSymmetryUseGlobal
          && layer.symmetry.symmetryMask == (Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::Radial)
          && layer.symmetry.radialSymmetryRepeatCount == 7,
          "the click never touches layer.symmetry's own configured fields — non-destructive per ARCH §19.24");
}

// STEP130, Part B Verify: both controls drawn in sequence (the real call-site composition,
// `[Symmetry toggle][Color Override]` sharing one combined width) land at distinct, non-overlapping
// X positions — the Symmetry checkbox's own right edge sits strictly left of Color Override's origin.
void RunCombinedControlsNonOverlappingPositionCheck() {
    HeadlessImguiSession session;
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    bool bAnyCommitted = false;

    ImVec2 symmetryOrigin, colorOverrideOrigin;
    float boxSize = 0.0f;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        symmetryOrigin = ImGui::GetCursorScreenPos();
        boxSize        = ResolveWidgetTrackHeight(WidgetStyle());
        DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted);
        ImGui::SameLine();
        colorOverrideOrigin = ImGui::GetCursorScreenPos();
        DrawManualMarkerLayerColorOverrideHeaderControl(layer, state, bAnyCommitted);
    });

    Check(colorOverrideOrigin.x >= symmetryOrigin.x + boxSize,
          "Color Override's own origin sits at/after the Symmetry checkbox's own right edge — no overlap");
    Check(std::fabs(colorOverrideOrigin.y - symmetryOrigin.y) < 0.001f,
          "both controls share the same row (same Y)");
}

// STEP239, ARCH §19.31 Mechanism A — a Link-bound Layer's own Color Override control goes inert
// (added to the pre-existing bUseGroupColor disable) and displays the RESOLVED (Link-owned) state,
// never the Layer's own raw fields, exactly the same "inert, not merely gray" proof
// RunInertWhileGroupColorForcedCheck already gives for bUseGroupColor, one condition over.
void RunInertWhileLinkedShowsResolvedColorCheck() {
    HeadlessImguiSession session;
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    layer.bColorOverrideEnabled = false;   // the Layer's OWN field disagrees with the Link on purpose
    layer.linkIdentifier        = 7;
    std::vector<Params::MarkerLink> links(1);
    links[0].identifier             = 7;
    links[0].bColorOverrideEnabled  = true;

    // A click at the checkbox's own known coordinates must do nothing while linked — mirrors
    // RunInertWhileGroupColorForcedCheck's own click-through proof exactly.
    const FrameResult settle = RunHeaderControlFrame(HeadlessMouseState(), layer, state, links);
    const ImVec2 checkboxCenter(settle.origin.x + settle.boxSize * 0.5f, settle.origin.y + settle.boxSize * 0.5f);
    HeadlessMouseState hover;   hover.position = checkboxCenter;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunHeaderControlFrame(hover, layer, state, links);
    RunHeaderControlFrame(press, layer, state, links);
    const FrameResult clicked = RunHeaderControlFrame(release, layer, state, links);

    Check(!layer.bColorOverrideEnabled,
         "a Link-bound Layer's own COL toggle stays inert to a click at the same coordinates");
    Check(!clicked.bAnyCommitted, "and reports no commit either — mirrors the bUseGroupColor case exactly");

    layer.linkIdentifier = -1;   // un-linked: the control reverts to editable, own-field behavior
    const FrameResult unlinkedClicked = ClickAt(checkboxCenter, layer, state);
    Check(layer.bColorOverrideEnabled,
         "once un-linked, the SAME control is editable again and flips the Layer's own field");
    (void)unlinkedClicked;
}

// STEP241, ARCH §19.31 correction — DrawMarkerLayerIconSizeHeaderControl: a Link-bound Layer's own
// iconScale goes inert (BeginDisabled), mirroring RunInertWhileLinkedShowsResolvedColorCheck's own
// click-through proof exactly, using SliderScalar_UI_Test.cpp's own "click on the right 3/4 of the
// track" technique to attempt a real value-changing interaction.
void RunIconSizeInertWhileLinkedCheck() {
    HeadlessImguiSession session;
    const ImVec2 windowSize(400.0f, 100.0f);
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    layer.iconScale      = 1.0f;
    layer.linkIdentifier = 7;
    std::vector<Params::MarkerLink> links(1);
    links[0].identifier = 7;
    links[0].iconScale  = 5.0f;

    bool bAnyCommitted = false;
    ImVec2 rowOrigin;
    RunHeadlessFrame(HeadlessMouseState(), windowSize, [&] {
        rowOrigin = ImGui::GetCursorScreenPos();
        DrawMarkerLayerIconSizeHeaderControl(layer, state, bAnyCommitted, links);
    });

    const float trackHeight = ResolveWidgetTrackHeight(WidgetStyle());
    const ImVec2 clickPoint(rowOrigin.x + kMarkerLayerIconSizeTrackWidthPixels * 0.9f,
                            rowOrigin.y + trackHeight * 0.5f);
    HeadlessMouseState hover;   hover.position = clickPoint;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunHeadlessFrame(hover, windowSize, [&] {
        DrawMarkerLayerIconSizeHeaderControl(layer, state, bAnyCommitted, links);
    });
    RunHeadlessFrame(press, windowSize, [&] {
        DrawMarkerLayerIconSizeHeaderControl(layer, state, bAnyCommitted, links);
    });
    RunHeadlessFrame(release, windowSize, [&] {
        DrawMarkerLayerIconSizeHeaderControl(layer, state, bAnyCommitted, links);
    });

    Check(layer.iconScale == 1.0f,
         "a Link-bound Layer's own iconScale is untouched by a click-drag on the (disabled) track");
    Check(!bAnyCommitted, "and reports no commit either");
}

// STEP241, ARCH §19.31 correction — DrawMarkerLayerGridSnapHeaderControl: both the GRID toggle
// button and the grid-size field go inert while linked. The toggle's own click-through mirrors
// RunInertWhileGroupColorForcedCheck's proven SmallButton shape exactly.
void RunGridSnapInertWhileLinkedCheck() {
    HeadlessImguiSession session;
    const ImVec2 windowSize(400.0f, 100.0f);
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    layer.bGridSnapEnabled       = false;
    layer.gridSnapSizeWorldUnits = 1.0f;
    layer.linkIdentifier         = 7;
    std::vector<Params::MarkerLink> links(1);
    links[0].identifier            = 7;
    links[0].bGridSnapEnabled      = true;
    links[0].gridSnapSizeWorldUnits = 8.0f;

    bool bAnyCommitted = false;
    ImVec2 origin; float boxSize = 0.0f;
    RunHeadlessFrame(HeadlessMouseState(), windowSize, [&] {
        origin  = ImGui::GetCursorScreenPos();
        boxSize = ResolveWidgetTrackHeight(WidgetStyle());
        DrawMarkerLayerGridSnapHeaderControl(layer, state, bAnyCommitted, links);
    });
    const ImVec2 toggleCenter(origin.x + boxSize * 0.5f, origin.y + boxSize * 0.5f);

    HeadlessMouseState hover;   hover.position = toggleCenter;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunHeadlessFrame(hover, windowSize, [&] { DrawMarkerLayerGridSnapHeaderControl(layer, state, bAnyCommitted, links); });
    RunHeadlessFrame(press, windowSize, [&] { DrawMarkerLayerGridSnapHeaderControl(layer, state, bAnyCommitted, links); });
    RunHeadlessFrame(release, windowSize, [&] { DrawMarkerLayerGridSnapHeaderControl(layer, state, bAnyCommitted, links); });

    Check(!layer.bGridSnapEnabled,
         "a Link-bound Layer's own bGridSnapEnabled stays inert to a click at the GRID toggle's coordinates");
    Check(!bAnyCommitted, "and reports no commit either");
}

// STEP241, ARCH §19.31 correction — DrawMarkerLayerSymmetryToggleHeaderControl: the SYM button goes
// inert while linked, mirroring RunSymmetryToggleFlipCheck's own proven click shape exactly.
void RunSymmetryInertWhileLinkedCheck() {
    HeadlessImguiSession session;
    const ImVec2 windowSize(400.0f, 100.0f);
    Params::MarkerInstanceLayer layer;
    layer.bSymmetryEnabled = true;
    layer.linkIdentifier   = 7;
    std::vector<Params::MarkerLink> links(1);
    links[0].identifier        = 7;
    links[0].bSymmetryEnabled  = false;

    bool bAnyCommitted = false;
    ImVec2 origin; float boxSize = 0.0f;
    RunHeadlessFrame(HeadlessMouseState(), windowSize, [&] {
        origin  = ImGui::GetCursorScreenPos();
        boxSize = ResolveWidgetTrackHeight(WidgetStyle());
        DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted, links);
    });
    const ImVec2 symButtonCenter(origin.x + boxSize * 0.5f, origin.y + boxSize * 0.5f);

    HeadlessMouseState hover;   hover.position = symButtonCenter;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunHeadlessFrame(hover, windowSize, [&] { DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted, links); });
    RunHeadlessFrame(press, windowSize, [&] { DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted, links); });
    RunHeadlessFrame(release, windowSize, [&] { DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted, links); });

    Check(layer.bSymmetryEnabled,
         "a Link-bound Layer's own bSymmetryEnabled stays inert (still true) to a click at the SYM button's coordinates");
    Check(!bAnyCommitted, "and reports no commit either");
}

} // namespace

int main() {
    RunCheckboxFlipCheck();
    RunInertWhileGroupColorForcedCheck();
    RunSwatchDisabledEnabledCheck();
    RunSymmetryToggleFlipCheck();
    RunCombinedControlsNonOverlappingPositionCheck();
    RunInertWhileLinkedShowsResolvedColorCheck();
    RunIconSizeInertWhileLinkedCheck();
    RunGridSnapInertWhileLinkedCheck();
    RunSymmetryInertWhileLinkedCheck();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
