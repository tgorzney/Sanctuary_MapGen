// StratumsTab_Appearance_UI.cpp — one stratum's appearance panel: the three color swatches, the
// mask remap window, the tile sizes and the normal/height detail. Layer: UI.
// TAB_REBUILD_PLAN "6 · Stratums" (the second half of a stratum section). Every color is a
// PICKER-ONLY swatch — no RGBA number fields — which is the plan's standing rule and the
// ColorSwatch widget's default.
#include "StratumsTab_Draw_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The preview base color is stored as three loose floats on the stratum, so the picker edits the
// row's RGBA mirror and the commit writes it back (StratumsTab_UI.h).
void DrawPreviewBaseColor(Params::Stratum& stratum, StratumRowState& row,
                          Pipeline::PreviewDriver* previewDriver) {
    ColorSwatchOptions options;                      // alpha stays off: a terrain tint has none
    const WidgetChange change = DrawColorSwatch("Preview Base Color", row.previewBaseColorMirror,
                                                options, row.previewBaseColorToggle);
    if (!change.bValueChanged) return;
    const bool bMoved = StoreStratumRowValues(row, stratum);
    NotifyStratumsTabChange(bMoved && change.bCommitted, previewDriver);
}

// The two shader remap colors live on the appearance sub-struct as RGBA already, so the picker
// edits them in place with no mirror.
void DrawRemapColor(const char* label, float color[kColorSwatchChannelCount],
                    RealtimeToggle& realtimeToggle, Pipeline::PreviewDriver* previewDriver) {
    ColorSwatchOptions options;
    NotifyStratumsTabChange(DrawColorSwatch(label, color, options, realtimeToggle).bCommitted,
                            previewDriver);
}

constexpr const char* kMaskRemapChannelSuffixes[Params::kStratumColorChannelCount] = { "X", "Y", "Z", "W" };

// The ONE surface-weight remap (ARCH §7.2.5), applied by the Mask stage. Held ordered here, per
// channel, so a window can never be inverted by a slider. The field is a 4-component Vector4 to
// match the `.sanmap` shape (StratumAppearance_PARAMS.h), so each channel gets its own row and its
// own RealtimeToggle — a drag on one channel must not move or affect any other.
void DrawMaskRemapWindow(Params::Stratum& stratum, StratumsTabState& state, StratumRowState& row,
                         Pipeline::PreviewDriver* previewDriver) {
    for (int channel = 0; channel < Params::kStratumColorChannelCount; ++channel) {
        NotifyStratumsTabChange(
            DrawStratumsTabScalarChannel(StratumsTabScalar::MaskRemapMinimum, channel,
                                         kMaskRemapChannelSuffixes[channel], stratum.maskRemapMinimum[channel],
                                         row.maskRemapMinimumToggles[channel], state).bCommitted,
            previewDriver);
        NotifyStratumsTabChange(
            DrawStratumsTabScalarChannel(StratumsTabScalar::MaskRemapMaximum, channel,
                                         kMaskRemapChannelSuffixes[channel], stratum.maskRemapMaximum[channel],
                                         row.maskRemapMaximumToggles[channel], state).bCommitted,
            previewDriver);
        if (stratum.maskRemapMaximum[channel] < stratum.maskRemapMinimum[channel])
            stratum.maskRemapMaximum[channel] = stratum.maskRemapMinimum[channel];
    }
}

} // namespace

void DrawStratumAppearancePanel(Params::Stratum& stratum, StratumsTabState& state, StratumRowState& row,
                                Pipeline::PreviewDriver* previewDriver) {
    Params::StratumAppearance& appearance = stratum.appearance;
    ImGui::PushID("appearance");
    DrawPreviewBaseColor(stratum, row, previewDriver);
    DrawRemapColor("Diffuse Remap", appearance.diffuseRemapColor, row.diffuseRemapColorToggle,
                   previewDriver);
    DrawRemapColor("Far Color Remap", appearance.farColorRemapColor, row.farColorRemapColorToggle,
                   previewDriver);

    DrawMaskRemapWindow(stratum, state, row, previewDriver);

    DrawStratumsTabScalarRow(StratumsTabScalar::TileCount, stratum.tileCount, state, row, previewDriver);
    DrawStratumsTabScalarRow(StratumsTabScalar::FarTileCount, appearance.farTileCount,
                             state, row, previewDriver);
    DrawStratumsTabScalarRow(StratumsTabScalar::TriplanarTileCount, appearance.triplanarTileCount,
                             state, row, previewDriver);
    DrawStratumsTabScalarRow(StratumsTabScalar::FarTriplanarTileCount,
                             appearance.farTriplanarTileCount, state, row, previewDriver);

    DrawStratumsTabScalarRow(StratumsTabScalar::NormalScale, appearance.normalScale,
                             state, row, previewDriver);
    DrawStratumsTabScalarRow(StratumsTabScalar::FarNormalScale, appearance.farNormalScale,
                             state, row, previewDriver);
    DrawStratumsTabScalarRow(StratumsTabScalar::NormalFarNearBlend, appearance.normalFarNearBlend,
                             state, row, previewDriver);
    DrawStratumsTabScalarRow(StratumsTabScalar::HeightFarNearBlend, appearance.heightFarNearBlend,
                             state, row, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
