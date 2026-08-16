// LayerEditor_Erosion_UI.h — the pure half of the Soil Physics / Hydraulic Erosion / Deposition /
// Advanced-constants panels. Layer: UI. Accuracy class: Visual.
//
// These four panels edit settings that have no `_PARAMS` home yet: soil physics
// (`Proc::MaterialPhysics`), the per-stratum erosion record (`Proc::ErosionLayerSettings`) and the
// thermal constants (`Proc::ThermalConstants`). The ARCH's interim contract for exactly this is
// written on `GenerationAssembler_PIPELINE.h`: "the tweakable constants each one owns are reached
// through these until the remaining *_PARAMS homes exist (UI wiring is M4/M5)". So this header
// includes the PIPELINE assembler — the layer UI is allowed to depend on (ARCH §3.1) — and never a
// PROC stage header of its own. When `ErosionFlow_PARAMS` lands (ARCH §5.2/§7.1), only the two
// accessor expressions below change; every control, limit and test stays as written.
//
// Everything here is pure so the panels' behavior is assertable with no imgui frame: the preset
// fill, the spawn-band mirror, and the stratum-index fence.
#pragma once
#include "LayerEditor_SoilPreset_UI.h"
#include "RangeSliderWidget_UI.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

// How many stratum slots the erosion stage keeps one soil/erosion record for.
inline constexpr int kLayerEditorStratumCount = Proc::ErosionStage::stratumCount;

// True when a layer's stratum index names one of those slots. A recipe written against a wider
// palette is rejected rather than indexed with (Constitution §6).
inline bool IsLayerEditorStratumIndex(int stratumIndex) {
    return stratumIndex >= 0 && stratumIndex < kLayerEditorStratumCount;
}

// Fills the five soil numbers from a preset. Reports whether anything actually moved, so picking
// the preset a stratum is already on costs no regeneration.
inline bool ApplySoilPresetToMaterial(SoilPreset preset, Proc::MaterialPhysics& material) {
    const SoilPresetValues values = SoilPresetValuesOf(preset);
    const bool bMoved = material.hardness != values.hardness
                     || material.friction != values.friction
                     || material.cohesion != values.cohesion
                     || material.capacityMultiplier != values.capacityMultiplier
                     || material.absorptionRate != values.absorptionRate;
    material.hardness           = values.hardness;
    material.friction           = values.friction;
    material.cohesion           = values.cohesion;
    material.capacityMultiplier = values.capacityMultiplier;
    material.absorptionRate     = values.absorptionRate;
    return bMoved;
}

// The deposition spawn band is a min/max pair the erosion record stores as two loose floats, so
// the range slider edits a mirror. settings -> mirror.
inline void LoadDepositionSpawnBand(const Proc::ErosionLayerSettings& erosionSettings,
                                    RangeSliderValues& spawnHeightValues) {
    spawnHeightValues.minimumValue = erosionSettings.spawnMinimumHeight;
    spawnHeightValues.maximumValue = erosionSettings.spawnMaximumHeight;
}

// mirror -> settings, clamped onto the slider's own bounds first. Reports whether it moved.
inline bool StoreDepositionSpawnBand(const RangeSliderValues& spawnHeightValues,
                                     const RangeSliderBounds& spawnHeightBounds,
                                     Proc::ErosionLayerSettings& erosionSettings) {
    const RangeSliderValues band = ClampRangeSliderValues(spawnHeightValues, spawnHeightBounds);
    const bool bMoved = band.minimumValue != erosionSettings.spawnMinimumHeight
                     || band.maximumValue != erosionSettings.spawnMaximumHeight;
    erosionSettings.spawnMinimumHeight = band.minimumValue;
    erosionSettings.spawnMaximumHeight = band.maximumValue;
    return bMoved;
}

// Writes ONE gravity to every stratum's erosion record — how the Heightmap tab's "Global Gravity"
// applies (there is no per-layer `bUseGlobalGravity` flag to opt in with, so the global control is
// a bulk write onto the same field the per-layer slider edits, never a rival second store).
// Reports whether any record moved.
inline bool ApplyGlobalGravityToErosion(float gravity, Pipeline::GenerationAssembler& assembler) {
    bool bMoved = false;
    for (int stratumIndex = 0; stratumIndex < kLayerEditorStratumCount; ++stratumIndex) {
        Proc::ErosionLayerSettings& erosionSettings = assembler.Erosion().LayerSettings(stratumIndex);
        if (erosionSettings.gravity == gravity) continue;
        erosionSettings.gravity = gravity;
        bMoved = true;
    }
    return bMoved;
}

} // namespace Ui
} // namespace SanmapGen
