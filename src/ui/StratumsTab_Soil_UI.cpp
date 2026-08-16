// StratumsTab_Soil_UI.cpp — one stratum's Soil Physics panel. Layer: UI.
// TAB_REBUILD_PLAN "6 · Stratums": "Soil Presets menu + Hardness/Friction/Cohesion/Capacity (same
// as Layer Editor soil)". Unlike the Layer Editor's panel this one edits the RECIPE
// (`Params::Stratum::soilPhysics`, ARCH §7.1) and PUSHES the committed values onto the sim's
// runtime record — see StratumsTab_SoilPhysics_UI.h for why that direction, and only that
// direction, is not a rival store.
#include "StratumsTab_Draw_UI.h"
#include "StratumsTab_SoilPhysics_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// settings -> the sim's record for THIS stratum, run after every committed soil edit. With no
// pipeline bound there is nothing to push onto and the recipe edit simply stands. It never
// notifies: the caller already pays for the commit, and one edit must cost exactly one refresh.
void PushSoilPhysicsToPipeline(const Params::Stratum& stratum, int stratumIndex, bool bCommitted,
                               Pipeline::GenerationAssembler* generationAssembler) {
    if (!bCommitted || generationAssembler == nullptr) return;
    if (stratumIndex < 0 || stratumIndex >= Proc::ErosionStage::stratumCount) return;
    ApplyStratumSoilPhysicsToMaterial(stratum.soilPhysics,
                                      generationAssembler->Erosion().Material(stratumIndex));
}

// The preset row. A pick fills the five numbers below it and is then forgotten — the menu shows no
// "current preset", because the sliders under it are free to leave any preset behind.
void DrawSoilPresetRow(Params::Stratum& stratum, int stratumIndex, StratumRowState& row,
                       Pipeline::GenerationAssembler* generationAssembler,
                       Pipeline::PreviewDriver* previewDriver) {
    ComboOptions options;
    options.labels     = soilPresetLabels;
    options.count      = kSoilPresetCount;
    options.emptyLabel = "Presets...";
    const WidgetChange change = DrawCombo("Preset", row.soilPresetIndex, options);
    if (!change.bValueChanged || !IsSoilPresetIndex(row.soilPresetIndex)) return;
    const bool bMoved =
        ApplySoilPresetToStratum(static_cast<SoilPreset>(row.soilPresetIndex), stratum.soilPhysics);
    NotifyStratumsTabChange(bMoved, previewDriver);
    PushSoilPhysicsToPipeline(stratum, stratumIndex, bMoved, generationAssembler);
}

// One soil slider, with the push already routed. Returns nothing: the notification is the push's.
void DrawSoilScalarRow(StratumsTabScalar scalar, float& value, Params::Stratum& stratum,
                       int stratumIndex, StratumsTabState& state, StratumRowState& row,
                       Pipeline::GenerationAssembler* generationAssembler,
                       Pipeline::PreviewDriver* previewDriver) {
    const WidgetChange change = DrawStratumsTabScalar(scalar, value, state, row);
    NotifyStratumsTabChange(change.bCommitted, previewDriver);
    PushSoilPhysicsToPipeline(stratum, stratumIndex, change.bCommitted, generationAssembler);
}

} // namespace

void DrawStratumSoilPanel(Params::Stratum& stratum, int stratumIndex, StratumsTabState& state,
                          StratumRowState& row, Pipeline::GenerationAssembler* generationAssembler,
                          Pipeline::PreviewDriver* previewDriver) {
    Params::StratumSoilPhysics& soilPhysics = stratum.soilPhysics;
    ImGui::PushID("soilPhysics");
    ImGui::TextUnformatted("Soil Physics");
    if (generationAssembler == nullptr)
        ImGui::TextUnformatted("No pipeline bound - edits stay in the recipe until one is.");
    DrawSoilPresetRow(stratum, stratumIndex, row, generationAssembler, previewDriver);
    DrawSoilScalarRow(StratumsTabScalar::SoilHardness, soilPhysics.hardness, stratum, stratumIndex,
                      state, row, generationAssembler, previewDriver);
    DrawSoilScalarRow(StratumsTabScalar::SoilFriction, soilPhysics.friction, stratum, stratumIndex,
                      state, row, generationAssembler, previewDriver);
    DrawSoilScalarRow(StratumsTabScalar::SoilCohesion, soilPhysics.cohesion, stratum, stratumIndex,
                      state, row, generationAssembler, previewDriver);
    DrawSoilScalarRow(StratumsTabScalar::SoilCapacityMultiplier, soilPhysics.capacityMultiplier,
                      stratum, stratumIndex, state, row, generationAssembler, previewDriver);
    DrawSoilScalarRow(StratumsTabScalar::SoilAbsorptionRate, soilPhysics.absorptionRate, stratum,
                      stratumIndex, state, row, generationAssembler, previewDriver);
    const WidgetChange erodableChange = DrawCheckbox("Erodable", soilPhysics.bErodable);
    NotifyStratumsTabChange(erodableChange.bCommitted, previewDriver);
    PushSoilPhysicsToPipeline(stratum, stratumIndex, erodableChange.bCommitted, generationAssembler);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
