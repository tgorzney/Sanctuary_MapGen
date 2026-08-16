// LayerEditor_Scalars_UI.h — the Layer Editor's catalogue of bounded numbers. Layer: UI.
// Accuracy class: Visual (it names limits; it computes nothing).
//
// The Layer Editor draws ~35 sliders. Declaring a ScalarSliderRange and an Ui::RealtimeToggle
// member per control would push LayerEditor_UI.h past the ARCH §1.5 ceiling and scatter the
// limits across four translation units, so every limit is named ONCE here, in one table, and the
// editor state carries two flat arrays indexed by this enum. The limits stay per-instance
// tweakable (Constitution §8): the table is the DEFAULT seed a LayerEditorState copies, not a
// constant the draw path reads behind the caller's back.
//
// The enumerator names are the plan's control names, fully spelled (ARCH §1.1).
#pragma once
#include "SliderScalar_UI.h"

namespace SanmapGen {
namespace Ui {

enum class LayerEditorScalar : int {
    // Per-GeoLayer group
    GroupStratumIndex,
    // Per-layer identity and stack combine
    StratumIndex, Opacity, HeightBlendContrast,
    // Per-layer noise source
    NoiseFrequency, NoiseOctaveCount, NoiseGain, NoiseLacunarity,
    NoiseWeightedStrength, NoisePingPongStrength, NoiseCellularJitter,
    // Per-layer density shaping
    LandDensity, PlateauDensity, MountainDensity, RampDensity,
    // Soil physics of the layer's stratum
    SoilHardness, SoilFriction, SoilCohesion, SoilCapacityMultiplier, SoilAbsorptionRate,
    // Hydraulic erosion
    ErosionDropletCount, ErosionMaximumLifetime, ErosionEvaporationRate, ErosionFluidViscosity,
    ErosionCarryingCapacityScale, ErosionGravity,
    // Precipitation
    RainNoiseFrequency, RainNoiseOctaveCount, RainWindAngleDegrees,
    // Deposition
    DepositionInitialSedimentLoad,
    // Advanced (constants)
    BaseErosionRate, BaseDepositionRate, MeanderStrength, DivergenceThreshold,
    ThermalIterationCount, ThermalRelaxationRate,
    Count
};

inline constexpr int kLayerEditorScalarCount = static_cast<int>(LayerEditorScalar::Count);

// One row of the catalogue. `bInteger` selects the integer twin of the slider (SliderScalar_UI.h),
// so a droplet count can never land between whole droplets.
struct LayerEditorScalarDescription {
    const char*       label       = "";
    ScalarSliderRange range;
    const char*       valueFormat = "%.3f";
    bool              bInteger    = false;
};

// The catalogue row for one control. An out-of-range enumerator answers a safe empty row rather
// than reading off the end of the table (Constitution §6).
const LayerEditorScalarDescription& LayerEditorScalarDescriptionOf(LayerEditorScalar scalar);

} // namespace Ui
} // namespace SanmapGen
