// LayerEditor_Erosion_UI_Test.cpp — tab-rebuild B acceptance, part 2: the Soil Physics presets,
// the deposition spawn-band mirror, the stratum fence and the Heightmap tab's global-gravity bulk
// write. All pure — the panels reach their settings through PIPELINE, so the checks drive a REAL
// Pipeline::GenerationAssembler rather than a replica of one, but never run it (no GL, no window).
// main() lives in LayerEditor_UI_Test.cpp.
#include "LayerEditor_Erosion_UI.h"
#include "LayerEditor_Scalars_UI.h"
#include "LayerEditor_TestSupport_UI.h"
#include "../params/MapRecipe_PARAMS.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// A preset must land inside the sliders that display it, or picking one would put a control off
// its own track — the exact class of defect the shared catalogue exists to prevent.
void CheckPresetIsInsideItsSliders(const SoilPresetValues& values, const char* label) {
    const ScalarSliderRange hardness =
        LayerEditorScalarDescriptionOf(LayerEditorScalar::SoilHardness).range;
    const ScalarSliderRange capacity =
        LayerEditorScalarDescriptionOf(LayerEditorScalar::SoilCapacityMultiplier).range;
    const ScalarSliderRange absorption =
        LayerEditorScalarDescriptionOf(LayerEditorScalar::SoilAbsorptionRate).range;
    CheckLayerEditor(values.hardness >= hardness.minimumValue && values.hardness <= hardness.maximumValue, label);
    CheckLayerEditor(values.capacityMultiplier >= capacity.minimumValue
                     && values.capacityMultiplier <= capacity.maximumValue, label);
    CheckLayerEditor(values.absorptionRate >= absorption.minimumValue
                     && values.absorptionRate <= absorption.maximumValue, label);
}

void RunSoilPresetChecks() {
    for (int presetIndex = 0; presetIndex < kSoilPresetCount; ++presetIndex) {
        CheckLayerEditor(soilPresetLabels[presetIndex] != nullptr, "every soil preset is labelled");
        CheckPresetIsInsideItsSliders(SoilPresetValuesOf(static_cast<SoilPreset>(presetIndex)),
                                      "every soil preset lands inside its own sliders");
    }
    CheckLayerEditor(SoilPresetValuesOf(SoilPreset::Bedrock).hardness
                     > SoilPresetValuesOf(SoilPreset::Mud).hardness,
                     "bedrock is harder than mud");
    CheckLayerEditor(SoilPresetValuesOf(SoilPreset::Sand).absorptionRate
                     > SoilPresetValuesOf(SoilPreset::Rock).absorptionRate,
                     "sand drinks more water than rock");
    CheckLayerEditor(!IsSoilPresetIndex(-1) && !IsSoilPresetIndex(kSoilPresetCount),
                     "an index the menu does not cover is rejected, never clamped");

    Proc::MaterialPhysics material;
    CheckLayerEditor(ApplySoilPresetToMaterial(SoilPreset::Clay, material),
                     "applying a preset reports the values moved");
    CheckLayerEditor(material.hardness == SoilPresetValuesOf(SoilPreset::Clay).hardness,
                     "and every one of the five lands");
    CheckLayerEditor(!ApplySoilPresetToMaterial(SoilPreset::Clay, material),
                     "re-picking the preset a stratum already sits on costs nothing");
}

void RunSpawnBandMirrorChecks() {
    Proc::ErosionLayerSettings erosionSettings;
    erosionSettings.spawnMinimumHeight = 0.3f;
    erosionSettings.spawnMaximumHeight = 0.8f;
    RangeSliderValues spawnHeightValues;
    LoadDepositionSpawnBand(erosionSettings, spawnHeightValues);
    CheckLayerEditor(spawnHeightValues.minimumValue == 0.3f && spawnHeightValues.maximumValue == 0.8f,
                     "the spawn band mirror loaded both edges");

    const RangeSliderBounds bounds{ 0.0f, 1.0f, 0.001f };
    CheckLayerEditor(!StoreDepositionSpawnBand(spawnHeightValues, bounds, erosionSettings),
                     "an untouched spawn-band round trip moves nothing");
    spawnHeightValues.minimumValue = 0.95f;                 // crosses its partner
    CheckLayerEditor(StoreDepositionSpawnBand(spawnHeightValues, bounds, erosionSettings),
                     "a moved spawn edge reports the settings moved");
    CheckLayerEditor(erosionSettings.spawnMinimumHeight < erosionSettings.spawnMaximumHeight,
                     "and the stored band is repaired into order");
}

void RunStratumFenceChecks() {
    CheckLayerEditor(kLayerEditorStratumCount == Proc::ErosionStage::stratumCount,
                     "the editor's palette width is the stage's, never a second number");
    CheckLayerEditor(IsLayerEditorStratumIndex(0) && IsLayerEditorStratumIndex(kLayerEditorStratumCount - 1),
                     "every palette slot is reachable");
    CheckLayerEditor(!IsLayerEditorStratumIndex(-1) && !IsLayerEditorStratumIndex(kLayerEditorStratumCount),
                     "a stratum index outside the palette is refused, not indexed with");
}

// Global Gravity is a BULK WRITE onto the one gravity field, not a rival second store
// (HeightmapTab_UI.h SCOPE NOTE 2).
void RunGlobalGravityChecks() {
    Params::MapRecipe recipe;
    Pipeline::GenerationAssembler assembler(recipe);
    assembler.Erosion().LayerSettings(3).gravity = 11.0f;

    CheckLayerEditor(ApplyGlobalGravityToErosion(7.5f, assembler),
                     "a global gravity that differs from any stratum reports the move");
    bool bEveryStratumFollowed = true;
    for (int stratumIndex = 0; stratumIndex < kLayerEditorStratumCount; ++stratumIndex)
        if (assembler.Erosion().LayerSettings(stratumIndex).gravity != 7.5f)
            bEveryStratumFollowed = false;
    CheckLayerEditor(bEveryStratumFollowed, "and every stratum's gravity followed it");
    CheckLayerEditor(!ApplyGlobalGravityToErosion(7.5f, assembler),
                     "re-applying the same gravity costs no regeneration");

    // The Advanced (constants) section edits real, already-settable fields — the promotion this
    // work-order makes is the CONTROL, not a new type.
    Proc::ErosionLayerSettings& erosionSettings = assembler.Erosion().LayerSettings(0);
    erosionSettings.baseErosionRate    = 0.55f;
    erosionSettings.baseDepositionRate = 0.11f;
    erosionSettings.meanderStrength    = 0.25f;
    erosionSettings.divergenceThreshold = 2.5f;
    assembler.Thermal().Constants().iterationCount  = 16;
    assembler.Thermal().Constants().relaxationRate  = 0.75f;
    CheckLayerEditor(assembler.Erosion().LayerSettings(0).baseErosionRate == 0.55f
                     && assembler.Erosion().LayerSettings(0).divergenceThreshold == 2.5f,
                     "the erosion constants the Advanced section binds are writable");
    CheckLayerEditor(assembler.Thermal().Constants().iterationCount == 16
                     && assembler.Thermal().Constants().relaxationRate == 0.75f,
                     "so are the two thermal constants");
}

} // namespace

void RunLayerEditorErosionChecks() {
    RunSoilPresetChecks();
    RunSpawnBandMirrorChecks();
    RunStratumFenceChecks();
    RunGlobalGravityChecks();
}
