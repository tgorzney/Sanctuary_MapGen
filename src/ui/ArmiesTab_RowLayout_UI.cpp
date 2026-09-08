// ArmiesTab_RowLayout_UI.cpp — see ArmiesTab_RowLayout_UI.h. STEP250: the per-army settings row,
// split out of ArmiesTab_UI.cpp for the ARCH §1.5 size ceiling.
#include "ArmiesTab_RowLayout_UI.h"
#include "Combo_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawArmySettings(std::vector<Params::Army>& armies, int armyIndex, ArmiesTabState& state) {
    Params::Army& army = armies[static_cast<std::size_t>(armyIndex)];

    // Engine ID — machine-owned (STEP76 ruling 2), no input, unchanged from before this ticket.
    ImGui::TextDisabled("Engine ID: %s", army.name.c_str());
    ImGui::SameLine();

    // Name — no visible label; hint text substitutes (ruling 5). displayName only, never `name`.
    TextInputRules displayNameRules;
    displayNameRules.maximumLength = 48;
    displayNameRules.bAllowEmpty   = true;    // display-only; blank is legal, ArmyRowLabel falls back
    DrawTextInput("Name", army.displayName, displayNameRules, WidgetStyle(), "Army Name",
                 /*bLabelHidden=*/true, /*fixedWidthPixels=*/140.0f);
    ImGui::SameLine();

    // STEP250: Army::alias's text box is REMOVED from this tab (ruling 1). Do NOT re-add a
    // DrawTextInput("Alias", ...) call here -- the field itself, its PARAMS member, and its IO
    // round-trip are untouched; only this tab's edit path is gone.

    // Team Color — no label, no RT (ruling 2), fixed small swatch.
    ColorSwatchOptions teamColorOptions = state.armyColorOptions;
    teamColorOptions.bLabelHidden          = true;
    teamColorOptions.bRealtimeToggleHidden = true;
    teamColorOptions.swatchWidth           = 28.0f;
    DrawColorSwatch("Team Color", army.armyColor, teamColorOptions, state.armyColorToggle);
    ImGui::SameLine();

    // Faction — no label (ruling 4), fixed width. Same-frame local int mirror: a combo pick commits
    // immediately, so it carries no RealtimeToggle of its own (STEP20 ruling #6).
    int factionIndex = static_cast<int>(army.faction);
    ComboOptions factionOptions;
    factionOptions.labels           = armyFactionLabels;
    factionOptions.count            = kArmyFactionCount;
    factionOptions.bLabelHidden     = true;
    factionOptions.fixedWidthPixels = 90.0f;
    if (DrawCombo("Faction", factionIndex, factionOptions).bCommitted)
        army.faction = static_cast<Params::Faction>(factionIndex);
    ImGui::SameLine();

    // Starting Alloys / Starting Energy — keep their labels (explicit instruction), lose RT
    // (ruling 3), compact single-line form.
    ImGui::TextUnformatted("Starting Alloys");
    ImGui::SameLine();
    DrawSliderScalarCompact("Starting Alloys", army.alloys, state.alloysRange, state.alloysToggle,
                            /*trackWidthPixels=*/100.0f, /*fieldWidthPixels=*/60.0f, WidgetStyle(),
                            "%.0f", /*bShowRealtimeToggle=*/false);
    ImGui::SameLine();
    ImGui::TextUnformatted("Starting Energy");
    ImGui::SameLine();
    DrawSliderScalarCompact("Starting Energy", army.energy, state.energyRange, state.energyToggle,
                            /*trackWidthPixels=*/100.0f, /*fieldWidthPixels=*/60.0f, WidgetStyle(),
                            "%.0f", /*bShowRealtimeToggle=*/false);

    // Mirror button — unchanged visibility rule (CanMirrorArmy), just joins the row.
    ImGui::SameLine();
    DrawMirrorArmyButton(armies, armyIndex, state.pendingMirrorSourceArmyIndex,
                         state.mirrorConfirmDialogState);
}

} // namespace Ui
} // namespace SanmapGen
