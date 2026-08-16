// StratumsTab_SoilPhysics_UI.h — the pure half of the Stratums tab's Soil Physics panel.
// Layer: UI. Accuracy class: Visual.
//
// WHERE SOIL PHYSICS LIVES (the point of this file). ARCH §7.1 names the soil physics as one of the
// things reached through `Params::Stratum`, and WO C2 owns that file, so the settings home is
// `Params::Stratum::soilPhysics` — this tab edits the RECIPE, not a runtime record.
// `Proc::MaterialPhysics` (reached through the PIPELINE assembler, the interim contract stated on
// `GenerationAssembler_PIPELINE.h`) is what the sim kernels actually read, so a committed edit is
// PUSHED onto it here — one direction, settings -> record. It is not a rival store: there is exactly
// one authority (the recipe) and one mirror, and the mirror is never read back.
//
// The Layer Editor's Soil Physics panel (WO B) still writes `Proc::MaterialPhysics` directly,
// because it had no PARAMS home when it was written. The two are the same five numbers of the same
// stratum; closing that seam — PIPELINE seeding the record from `Params::Stratum::soilPhysics`, and
// the Layer Editor panel binding to the recipe — is a work-order, not something to do from inside
// this tab (ARCH §8.4).
#pragma once
#include "LayerEditor_SoilPreset_UI.h"
#include "../params/Stratum_PARAMS.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

// Fills the five soil numbers from a preset. Reports whether anything actually moved, so picking
// the preset a stratum is already on costs no regeneration. (The twin of
// `ApplySoilPresetToMaterial`, aimed at the SETTINGS type rather than the runtime record.)
inline bool ApplySoilPresetToStratum(SoilPreset preset, Params::StratumSoilPhysics& soilPhysics) {
    const SoilPresetValues values = SoilPresetValuesOf(preset);
    const bool bMoved = soilPhysics.hardness != values.hardness
                     || soilPhysics.friction != values.friction
                     || soilPhysics.cohesion != values.cohesion
                     || soilPhysics.capacityMultiplier != values.capacityMultiplier
                     || soilPhysics.absorptionRate != values.absorptionRate;
    soilPhysics.hardness           = values.hardness;
    soilPhysics.friction           = values.friction;
    soilPhysics.cohesion           = values.cohesion;
    soilPhysics.capacityMultiplier = values.capacityMultiplier;
    soilPhysics.absorptionRate     = values.absorptionRate;
    return bMoved;
}

// settings -> runtime record, for ONE stratum. Reports whether the record moved, so an unchanged
// push costs no regeneration.
inline bool ApplyStratumSoilPhysicsToMaterial(const Params::StratumSoilPhysics& soilPhysics,
                                              Proc::MaterialPhysics& material) {
    const bool bMoved = material.hardness != soilPhysics.hardness
                     || material.friction != soilPhysics.friction
                     || material.cohesion != soilPhysics.cohesion
                     || material.capacityMultiplier != soilPhysics.capacityMultiplier
                     || material.absorptionRate != soilPhysics.absorptionRate
                     || material.bErodable != soilPhysics.bErodable;
    material.hardness           = soilPhysics.hardness;
    material.friction           = soilPhysics.friction;
    material.cohesion           = soilPhysics.cohesion;
    material.capacityMultiplier = soilPhysics.capacityMultiplier;
    material.absorptionRate     = soilPhysics.absorptionRate;
    material.bErodable          = soilPhysics.bErodable;
    return bMoved;
}

// The whole palette in one call — what the tab runs after a committed soil edit, and what the host
// runs once after loading a recipe. Strata past the record array are ignored rather than indexed
// with (Constitution §6).
inline bool ApplyStratumSoilPhysicsToErosion(const std::vector<Params::Stratum>& strata,
                                             Pipeline::GenerationAssembler& generationAssembler) {
    bool bMoved = false;
    const int stratumCount = static_cast<int>(strata.size()) < Proc::ErosionStage::stratumCount
        ? static_cast<int>(strata.size()) : Proc::ErosionStage::stratumCount;
    for (int stratumIndex = 0; stratumIndex < stratumCount; ++stratumIndex)
        bMoved |= ApplyStratumSoilPhysicsToMaterial(
            strata[static_cast<std::size_t>(stratumIndex)].soilPhysics,
            generationAssembler.Erosion().Material(stratumIndex));
    return bMoved;
}

} // namespace Ui
} // namespace SanmapGen
