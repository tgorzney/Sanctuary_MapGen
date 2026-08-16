// StratumsTab_UI.cpp — the imgui composition of the Stratums tab: the overlay toggle, the
// environment pack row, and the nine per-stratum sections. Layer: UI. Each section's body is the
// three panels in their own translation units (StratumsTab_Draw_UI.h); this file owns only the
// shell and the label vocabulary.
#include "StratumsTab_Draw_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The longest header the tab draws is "Stratum 8 - " plus a 48-character name.
enum : int { kStratumSectionLabelCapacity = 80 };

// The environment pack row. The picker never opens a dialog itself — it reports the request and
// the host runs the platform picker (FilePathPicker_UI.h), which is what keeps IO out of the UI.
void DrawEnvironmentPack(StratumsTabState& state) {
    if (!DrawSectionBegin("Environment", state.environmentSection)) return;
    const FilePathPickerResult result =
        DrawFilePathPicker("Environment Pack", state.environmentPackPath, state.environmentPackOptions);
    if (result.bRejectedExtension)
        ImGui::TextUnformatted("That file is not a .sanpack - the environment is unchanged.");
    if (state.assetOptions.environmentCount <= 0)
        ImGui::TextUnformatted("No environments loaded - pick a .sanpack to fill the dropdowns.");
    DrawSectionEnd();
}

// One stratum's collapsing section: the three panels, in the plan's order.
void DrawStratumSection(Params::Stratum& stratum, int stratumIndex, StratumsTabState& state,
                        Pipeline::GenerationAssembler* generationAssembler,
                        Pipeline::PreviewDriver* previewDriver) {
    StratumRowState& row = state.rows[stratumIndex];
    // recipe -> mirrors, unless a picker is mid-edit: reloading under a live drag would fight it.
    if (!row.previewBaseColorToggle.IsCommitDeferred())
        LoadStratumRowValues(stratum, state.assetOptions, row);

    char sectionLabel[kStratumSectionLabelCapacity];
    FormatStratumSectionLabel(stratumIndex, stratum, sectionLabel, kStratumSectionLabelCapacity);

    ImGui::PushID(stratumIndex);
    if (DrawSectionBegin(sectionLabel, row.section)) {
        DrawStratumMaterialPanel(stratum, state, row, previewDriver);
        DrawStratumAppearancePanel(stratum, state, row, previewDriver);
        DrawStratumSoilPanel(stratum, stratumIndex, state, row, generationAssembler, previewDriver);
        DrawSectionEnd();
    }
    ImGui::PopID();
}

} // namespace

void DrawStratumsTab(std::vector<Params::Stratum>& strata, StratumsTabState& state,
                     Pipeline::GenerationAssembler* generationAssembler,
                     Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("stratumsTab");
    // Presentation only (SCOPE NOTE 1): it never trips a pipeline refresh, so it is not notified.
    DrawCheckbox("Show Stratums Overlay", state.bShowStratumOverlay);
    DrawEnvironmentPack(state);

    EnsureStratumPalette(strata);
    for (int stratumIndex = 0; stratumIndex < kStratumsTabStratumCount; ++stratumIndex)
        DrawStratumSection(strata[static_cast<std::size_t>(stratumIndex)], stratumIndex, state,
                           generationAssembler, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
