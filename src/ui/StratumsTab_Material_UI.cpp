// StratumsTab_Material_UI.cpp — one stratum's identity panel: name, the environment/material
// dropdowns, the three texture pickers and the 3-state mask mode. Layer: UI.
// TAB_REBUILD_PLAN "6 · Stratums" (the first half of a stratum section).
#include "StratumsTab_Draw_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// One dropdown over a borrowed sanpack catalogue. The PICKED LABEL is copied into the setting, so
// the recipe stores the material's name and never a row index into a list that can change under it
// (StratumsTab_UI.h SCOPE NOTE 3).
void DrawAssetNameCombo(const char* label, std::string& assetName, int& selectedIndex,
                        const char* const* labels, int count,
                        Pipeline::PreviewDriver* previewDriver) {
    ComboOptions options;
    options.labels     = labels;
    options.count      = count;
    options.emptyLabel = count > 0 ? "Select..." : "<no pack loaded>";
    const WidgetChange change = DrawCombo(label, selectedIndex, options);
    if (!change.bValueChanged) return;
    assetName = StratumOptionLabelAt(selectedIndex, labels, count);
    NotifyStratumsTabChange(change.bCommitted, previewDriver);
}

// One texture path row. The extension fence lives in the picker; a rejected file leaves the setting
// untouched and says so, rather than handing a loader something it cannot read (Constitution §6).
void DrawTexturePathRow(const char* label, std::string& texturePath, const StratumsTabState& state,
                        Pipeline::PreviewDriver* previewDriver) {
    const FilePathPickerResult result = DrawFilePathPicker(label, texturePath, state.textureOptions);
    if (result.bRejectedExtension) ImGui::TextUnformatted("Unsupported texture format - unchanged.");
    NotifyStratumsTabChange(result.change.bCommitted, previewDriver);
}

// The 3-state mask mode, drawn as v1 drew it: one button showing the current state that cycles
// Disabled -> Procedural Start -> Static Override. The cycle itself is pure
// (StratumsTab_Options_UI.h), so what the button does is assertable with no imgui frame.
void DrawMaskModeToggle(Params::Stratum& stratum, Pipeline::PreviewDriver* previewDriver) {
    if (!ImGui::Button(ImportedMaskModeLabel(stratum.importedMaskMode))) return;
    stratum.importedMaskMode = NextImportedMaskMode(stratum.importedMaskMode);
    NotifyStratumsTabChange(true, previewDriver);
}

} // namespace

void DrawStratumMaterialPanel(Params::Stratum& stratum, StratumsTabState& state, StratumRowState& row,
                              Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("material");
    NotifyStratumsTabChange(DrawCheckbox("Enabled", stratum.bEnabled).bCommitted, previewDriver);
    NotifyStratumsTabChange(
        DrawTextInput("Name", stratum.appearance.name, StratumNameRules()).bCommitted, previewDriver);

    DrawAssetNameCombo("Environment", stratum.appearance.environmentName, row.environmentIndex,
                       state.assetOptions.environmentLabels, state.assetOptions.environmentCount,
                       previewDriver);
    DrawAssetNameCombo("Material", stratum.appearance.materialName, row.materialIndex,
                       state.assetOptions.materialLabels, state.assetOptions.materialCount,
                       previewDriver);

    DrawTexturePathRow("Albedo", stratum.appearance.albedoTexturePath, state, previewDriver);
    DrawTexturePathRow("Normal", stratum.appearance.normalTexturePath, state, previewDriver);
    DrawTexturePathRow("Composite", stratum.appearance.compositeTexturePath, state, previewDriver);

    DrawMaskModeToggle(stratum, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
