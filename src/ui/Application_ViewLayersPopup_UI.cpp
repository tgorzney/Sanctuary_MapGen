// Application_ViewLayersPopup_UI.cpp — the "View" toolbar popup's body: two independent,
// non-crossing DraggableList sections over PreviewCompositeSettings::fieldLayers (terrain,
// composited) and OverlayLayerSettings::overlayLayers (screen-space, STEP51). Layer: UI.
// ARCH_14_07_ViewToolbar.md §14.7 retires the primary toolbar's manual "Regenerate" trigger (already
// gone — STEP55) in favor of this click-to-open popup; §14.2's "toolbar never adds/removes/blends/
// hides only" contract is enforced by only ever handing a signal to ApplyViewLayerSignal
// (Application_ViewLayersPopup_UI.h), never Delete.
#include "Application_UI.h"
#include "Application_ViewLayersPopup_UI.h"
#include <cstdio>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

const char* const previewLayerKindNames[] = {
    "HeightRamp", "StratumSplat", "Flow", "Accumulation", "Water", "Slope", "MapAreas"
};
// PreviewBlendMode's own enum order (PreviewComposite_Settings_UI.h) — distinct from
// Params::HeightBlendMode's label set (LayersTab_UI.cpp's blendModeNames); do not merge the two.
// STEP200: Subtract..HardLight are the v1 parity additions, appended after Minimum to match the
// enum's own append-only order.
const char* const previewBlendModeNames[] = {
    "Replace", "AlphaBlend", "Add", "Multiply", "Maximum", "Minimum",
    "Subtract", "Divide", "Overlay", "Screen", "SoftLight", "HardLight"
};
static_assert(IM_ARRAYSIZE(previewBlendModeNames) == kPreviewBlendModeCount,
             "previewBlendModeNames must name every PreviewBlendMode enumerator, in enum order");
// STEP200 fix approach point 1 — a named fixed width for the popup's own fixed-width items (the
// blend combo, the opacity slider), matching v1's own `SetNextItemWidth(100)` before its combo
// (Widget_MapCanvas.cpp:69). An unconstrained item width inside the auto-fit BeginPopup window is
// what fed the reported runaway-growth defect.
constexpr float kFixedItemWidthPixels = 100.0f;

// Terrain (composited) section — reorder + per-row blend-mode dropdown (a real GPU blend-equation
// switch into the composite shader, §14.7 "unchanged by this ruling"). Returns true if the RECIPE-
// adjacent composite settings moved (needs a driver notification), matching
// Application_LeftColumn_UI.cpp:57-60's existing "mutate PreviewCompositeSettings then
// NotifyParametersChanged()" precedent — this IS presentation state, not PARAMS, but the driver
// still derives its own tier (B, full recomposite, §14.8) from that one call. Blend-mode picks are a
// raw ImGui::Combo, the same choice LayersTab_UI.cpp's own DrawEnumSetting makes for a static enum
// table with no shared-widget equivalent (its own header comment: "no shared-library equivalent to
// compose from" for a dropdown).
bool DrawTerrainSection(std::vector<PreviewFieldLayer>& fieldLayers) {
    char rowLabel[48] = { 0 };
    bool bBlendModeChanged = false;
    const DraggableListSignal signal = DraggableList<PreviewFieldLayer>::Render(
        "ViewListField", fieldLayers,
        [&](int rowIndex) {
            const PreviewFieldLayer& layer = fieldLayers[static_cast<std::size_t>(rowIndex)];
            const int kindIndex = static_cast<int>(layer.kind);
            const char* const kindName = (kindIndex >= 0
                && kindIndex < IM_ARRAYSIZE(previewLayerKindNames)) ?
                previewLayerKindNames[kindIndex] : "Unknown";
            std::snprintf(rowLabel, sizeof(rowLabel), "%s", kindName);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = layer.bEnabled;
            return row;
        },
        [&](int rowIndex) {
            PreviewFieldLayer& layer = fieldLayers[static_cast<std::size_t>(rowIndex)];
            int blendIndex = static_cast<int>(layer.blendMode);
            // STEP200: no visible label (v1 parity, Widget_MapCanvas.cpp:68-70's "##blend") and a
            // fixed width (point 1) — both required to stop the popup's auto-fit growth loop.
            ImGui::SetNextItemWidth(kFixedItemWidthPixels);
            if (ImGui::Combo("##blend", &blendIndex, previewBlendModeNames,
                             IM_ARRAYSIZE(previewBlendModeNames))) {
                layer.blendMode = static_cast<PreviewBlendMode>(blendIndex);
                bBlendModeChanged = true;
            }
        },
        -1, DraggableListRowLayout::Flat);
    return ApplyViewLayerSignal(fieldLayers, signal) || bBlendModeChanged;
}

// Overlays (screen-space) section — reorder + per-row opacity, §14.2/§14.13 item 5: opacity, not
// blend mode, and every overlay layer shares ImGui's one global blend equation. §14.8's Tier C
// ("every overlay layer, every frame... opacity... zero GPU recompute") means there is nothing
// expensive here to DEFER, unlike LayersTab_UI.cpp's dial-wrapped scalars: RtToggleWidget_UI's
// realtime-toggle machinery exists to spare an EXPENSIVE recompute during a drag, and an overlay
// redraw is already unconditional and cheap every frame regardless of RT state. A direct
// ImGui::SliderFloat with instant commit is therefore correct here, not a missing dial. Returns true
// if anything moved — reorder, visibility, or opacity — so the caller can bump
// OverlayLayerSettings::layerSettingsRevision, the C2 cache invalidation key STEP53's icon draw pass
// (MapCanvas_IconLayer_Cull_UI.cpp/_Draw_UI.cpp) already reads live.
bool DrawOverlaySection(std::vector<OverlayLayer_UI>& overlayLayers) {
    bool bOpacityChanged = false;
    const DraggableListSignal signal = DraggableList<OverlayLayer_UI>::Render(
        "ViewListOverlay", overlayLayers,
        [&](int rowIndex) {
            const OverlayLayer_UI& layer = overlayLayers[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label    = layer.name.empty() ? "Overlay" : layer.name.c_str();
            row.bVisible = layer.bEnabled;
            return row;
        },
        [&](int rowIndex) {
            OverlayLayer_UI& layer = overlayLayers[static_cast<std::size_t>(rowIndex)];
            // STEP200 fix approach point 1 — fixed width, same defense as the blend combo above.
            ImGui::SetNextItemWidth(kFixedItemWidthPixels);
            if (ImGui::SliderFloat("Opacity", &layer.opacity, 0.0f, 1.0f)) bOpacityChanged = true;
        },
        -1, DraggableListRowLayout::Flat);
    return ApplyViewLayerSignal(overlayLayers, signal) || bOpacityChanged;
}

} // namespace

void Application::DrawViewLayersPopup() {
    ImGui::TextUnformatted("Reorder within a section only: a terrain layer and an overlay layer "
                           "can never cross (not renderable, ARCH_14_07_ViewToolbar.md \xc2\xa7" "14.7).");
    ImGui::SeparatorText("Terrain (composited)");
    const bool bTerrainMoved = DrawTerrainSection(composite.Settings().fieldLayers);
    if (bTerrainMoved) previewDriver.NotifyParametersChanged();

    ImGui::SeparatorText("Overlays (screen-space)");
    // Reorder/visibility/opacity here trip no PreviewDriver tier at all (§14.8 Tier C redraws every
    // frame from current state unconditionally) — instead this bumps the overlay stack's own
    // revision counter, which is the real C2-cache invalidation key STEP53's draw pass already
    // reads (OverlayLayerSettings::layerSettingsRevision, "no mutation site exists yet... the View
    // toolbar is Phase 4" — this popup IS that site).
    if (DrawOverlaySection(overlaySettings.overlayLayers)) overlaySettings.BumpLayerSettingsRevision();
}

} // namespace Ui
} // namespace SanmapGen
