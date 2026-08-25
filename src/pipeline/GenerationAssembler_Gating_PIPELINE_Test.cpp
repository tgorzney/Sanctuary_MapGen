// GenerationAssembler_Gating_PIPELINE_Test.cpp — STEP152 acceptance: Erosion/Thermal/
// FlowAccumulation's `run` closures are gated behind `Params::LayerStack::HasActiveProceduralLayer()`
// (GenerationAssembler_Stages_PIPELINE.cpp) -- skipped entirely once nothing in the flattened stack
// is a live, unbaked, non-None-recipe layer, and running normally the moment one exists. Proved on
// the OUTPUT of a real `assembler.Run()` pass (not a mock), the same "effect on the terrain, not
// merely a counter" discipline GenerationAssembler_Outputs_PIPELINE_Test.cpp's
// CheckSimulationChangedTerrain already uses for its own control pipeline.
#include "GenerationAssembler_TestScene_PIPELINE.h"
#include <cstdio>

using namespace SanmapGen;
using namespace AssemblerTest;

void AssemblerCheck(bool bCondition, const char* label);   // GenerationAssembler_PIPELINE_Test.cpp

namespace {

constexpr int bakedLayerIdentifier = 5;

// A diagonal ramp -- real slope for Erosion/Thermal to act on if they run, unlike a flat/zero
// image where "gated off" and "ran but had nothing to do" would look identical.
Data::FloatField MakeRampImage() {
    Data::FloatField image(vertexSize, vertexSize);
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            image.Set(x, y, static_cast<float>(x + y) / static_cast<float>(2 * (vertexSize - 1)));
    return image;
}

// A single-GeoLayer, single-Layer stack whose one layer is either baked (inactive) or live
// (active) against the SAME identifier, so both scenarios share one recipe shape.
Params::MapRecipe MakeSingleLayerRecipe(bool bLayerBaked) {
    Params::MapRecipe recipe = MakeRecipe(9191u);
    recipe.layerStack.geoLayers.clear();
    Params::GeoLayer group;
    Params::Layer layer;
    layer.layerIdentifier = bakedLayerIdentifier;
    layer.bBaked           = bLayerBaked;
    group.layers.push_back(layer);
    recipe.layerStack.geoLayers.push_back(group);
    return recipe;
}

void SeedBakedImage(Pipeline::GenerationAssembler& assembler) {
    Data::BakedLayerImage& entry =
        Data::FindOrAddBakedLayerImage(assembler.BakedLayerImages(), bakedLayerIdentifier);
    entry.image = MakeRampImage();
}

// The reference a genuinely no-op Erosion/Thermal settles to: zeroed at the STAGE'S OWN settings
// (Proc::ErosionLayerSettings::bEnabled / Proc::ThermalConstants::iterationCount), independent of
// PIPELINE's own gate -- the same control CheckSimulationChangedTerrain already relies on.
void RunManuallyUnsimmed(Pipeline::GenerationAssembler& control, bool bSeedBakedImage) {
    ConfigureStages(control);
    if (bSeedBakedImage) SeedBakedImage(control);
    control.Erosion().LayerSettings(0).bEnabled  = false;
    control.Thermal().Constants().iterationCount = 0;
    control.Run();
}

bool FieldIsAllZero(const Data::FloatField& field) {
    for (std::size_t index = 0; index < field.CellCount(); ++index)
        if (field.Data()[index] != 0.0f) return false;
    return true;
}

float FieldMaximum(const Data::FloatField& field) {
    float maximum = 0.0f;
    for (std::size_t index = 0; index < field.CellCount(); ++index)
        if (field.Data()[index] > maximum) maximum = field.Data()[index];
    return maximum;
}

} // namespace

void RunGatingChecks() {
    // --- inactive: the only layer is baked against a real, non-flat image, so
    // HasActiveProceduralLayer() is false. Erosion/Thermal must be TRUE no-ops (their `run`
    // closure never executes) and FlowAccumulation must never write accumulation/flow at all.
    Params::MapRecipe inactiveRecipe = MakeSingleLayerRecipe(/*bLayerBaked=*/true);
    AssemblerCheck(!inactiveRecipe.layerStack.HasActiveProceduralLayer(),
                   "a single baked layer with no other recipe leaves nothing active");

    Pipeline::GenerationAssembler inactiveAssembler(inactiveRecipe);
    ConfigureStages(inactiveAssembler);
    SeedBakedImage(inactiveAssembler);
    // ConfigureStages leaves Erosion ENABLED (its normal default) -- the gate, not this layer
    // setting, is what must keep Erosion/Thermal from touching the terrain here.
    inactiveAssembler.Run();

    Pipeline::GenerationAssembler inactiveControl(inactiveRecipe);
    RunManuallyUnsimmed(inactiveControl, /*bSeedBakedImage=*/true);

    AssemblerCheck(FieldChecksum(inactiveAssembler.Fields().heightfield)
                 == FieldChecksum(inactiveControl.Fields().heightfield),
                 "with no active procedural layer, Erosion/Thermal never touch the heightfield -- "
                 "the gated Run() matches a manually-unsimmed control exactly");
    AssemblerCheck(FieldIsAllZero(inactiveAssembler.Fields().accumulation)
                 && FieldIsAllZero(inactiveAssembler.Fields().flow),
                 "and FlowAccumulation never runs either -- accumulation/flow stay at NoiseBlend's "
                 "own untouched zero fill (only FlowAccumulation ever writes them)");

    // --- active: the SAME identifier, but the layer is live (unbaked), so
    // HasActiveProceduralLayer() is true and the sim block must actually run.
    Params::MapRecipe activeRecipe = MakeSingleLayerRecipe(/*bLayerBaked=*/false);
    AssemblerCheck(activeRecipe.layerStack.HasActiveProceduralLayer(),
                   "the same layer, unbaked with a real recipe, counts as active");

    Pipeline::GenerationAssembler activeAssembler(activeRecipe);
    ConfigureStages(activeAssembler);
    activeAssembler.Run();   // unbaked layer generates live noise -- no baked image needed

    Pipeline::GenerationAssembler activeControl(activeRecipe);
    RunManuallyUnsimmed(activeControl, /*bSeedBakedImage=*/false);

    AssemblerCheck(FieldChecksum(activeAssembler.Fields().heightfield)
                 != FieldChecksum(activeControl.Fields().heightfield),
                 "with an active procedural layer, Erosion/Thermal DO run -- the gated Run() "
                 "diverges from the manually-unsimmed control");
    AssemblerCheck(FieldMaximum(activeAssembler.Fields().accumulation) > 0.0f,
                   "and FlowAccumulation runs too -- accumulation is actually written");
}
