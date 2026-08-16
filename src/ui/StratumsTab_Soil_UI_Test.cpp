// StratumsTab_Soil_UI_Test.cpp — tab-rebuild C2 acceptance, part 2: soil physics now has a
// settings home on `Params::Stratum` (ARCH §7.1) and the sim's `Proc::MaterialPhysics` record is a
// ONE-WAY mirror of it. The checks drive a REAL Pipeline::GenerationAssembler rather than a replica
// of one, but never run it (no GL, no window). main() lives in StratumsTab_UI_Test.cpp.
#include "StratumsTab_SoilPhysics_UI.h"
#include "StratumsTab_TestSupport_UI.h"
#include "../params/MapRecipe_PARAMS.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// A preset must land inside the sliders that display it, or picking one would put a control off
// its own track — the same fence the Layer Editor's catalogue keeps.
void CheckPresetIsInsideItsSliders(const SoilPresetValues& values) {
    const ScalarSliderRange hardness =
        StratumsTabScalarDescriptionOf(StratumsTabScalar::SoilHardness).range;
    const ScalarSliderRange capacity =
        StratumsTabScalarDescriptionOf(StratumsTabScalar::SoilCapacityMultiplier).range;
    const ScalarSliderRange absorption =
        StratumsTabScalarDescriptionOf(StratumsTabScalar::SoilAbsorptionRate).range;
    CheckStratumsTab(values.hardness >= hardness.minimumValue
                     && values.hardness <= hardness.maximumValue,
                     "every soil preset lands inside the tab's own hardness slider");
    CheckStratumsTab(values.capacityMultiplier >= capacity.minimumValue
                     && values.capacityMultiplier <= capacity.maximumValue,
                     "and inside its capacity slider");
    CheckStratumsTab(values.absorptionRate >= absorption.minimumValue
                     && values.absorptionRate <= absorption.maximumValue,
                     "and inside its absorption slider");
}

void RunPresetChecks() {
    for (int presetIndex = 0; presetIndex < kSoilPresetCount; ++presetIndex)
        CheckPresetIsInsideItsSliders(SoilPresetValuesOf(static_cast<SoilPreset>(presetIndex)));

    Params::StratumSoilPhysics soilPhysics;
    CheckStratumsTab(ApplySoilPresetToStratum(SoilPreset::Clay, soilPhysics),
                     "applying a preset to the SETTINGS reports the values moved");
    CheckStratumsTab(soilPhysics.hardness == SoilPresetValuesOf(SoilPreset::Clay).hardness
                     && soilPhysics.absorptionRate == SoilPresetValuesOf(SoilPreset::Clay).absorptionRate,
                     "and every one of the five lands on the recipe");
    CheckStratumsTab(!ApplySoilPresetToStratum(SoilPreset::Clay, soilPhysics),
                     "re-picking the preset a stratum already sits on costs nothing");
}

// The push is settings -> record, and only that direction (StratumsTab_SoilPhysics_UI.h).
void RunPipelinePushChecks() {
    Params::MapRecipe recipe;
    EnsureStratumPalette(recipe.strata);
    Pipeline::GenerationAssembler assembler(recipe);
    CheckStratumsTab(!ApplyStratumSoilPhysicsToErosion(recipe.strata, assembler),
                     "the settings defaults already match the record, so a fresh push is free");

    recipe.strata[2].soilPhysics.hardness  = 0.9f;
    recipe.strata[2].soilPhysics.bErodable = false;
    CheckStratumsTab(ApplyStratumSoilPhysicsToErosion(recipe.strata, assembler),
                     "pushing a changed palette reports the sim's record moved");
    CheckStratumsTab(assembler.Erosion().Material(2).hardness == 0.9f
                     && !assembler.Erosion().Material(2).bErodable,
                     "and the stratum's soil reached the record the kernels read");
    CheckStratumsTab(!ApplyStratumSoilPhysicsToErosion(recipe.strata, assembler),
                     "a second push of the same settings costs no regeneration");

    // The mirror is never read back, so touching the record cannot rewrite the recipe behind the
    // designer's back — which is what makes this a mirror and not a rival store.
    assembler.Erosion().Material(2).hardness = 0.1f;
    CheckStratumsTab(recipe.strata[2].soilPhysics.hardness == 0.9f,
                     "the recipe stays the authority when the runtime record is touched");

    // A recipe shorter than the palette pushes what it has and indexes nothing past the end.
    std::vector<Params::Stratum> shortPalette(2);
    shortPalette[1].soilPhysics.friction = 0.33f;
    CheckStratumsTab(ApplyStratumSoilPhysicsToErosion(shortPalette, assembler),
                     "a short palette still pushes the strata it carries");
    CheckStratumsTab(assembler.Erosion().Material(1).friction == 0.33f,
                     "and lands them on the right slots");
}

} // namespace

void RunStratumsTabSoilChecks() {
    RunPresetChecks();
    RunPipelinePushChecks();
}
