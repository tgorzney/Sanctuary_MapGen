// GenerationEnums_PARAMS.h — shared generation enums (fully-spelled, no abbreviations).
// Layer: PARAMS. Replaces the abbreviated enums in the old Params_Enums.h.
#pragma once

namespace SanmapGen {
namespace Params {

enum class NoiseType {
    OpenSimplex2, OpenSimplex2Smooth, Cellular, Perlin, ValueCubic, Value, None
};

enum class FractalType {
    None, FractionalBrownian, Ridged, PingPong
};

// Heightfield blend (geometry) — distinct from the preview-composite blend enum.
enum class HeightBlendMode {
    Add, Subtract, Multiply, Overlay, Maximum, Minimum
};

// How a stored stratum mask merges with the procedural mask (see MASKING_SPEC).
enum class ImportedMaskMode {
    Disabled, ProceduralStart, StaticOverride
};

// A GeoLayer either contributes material (owns a stratum) or only reshapes height.
enum class GeoLayerMode {
    Material, Shaper
};

// Whether GeoLayers are simulated apart or as one combined stack (LAYER_SYSTEM_SPEC).
enum class SimulationGrouping {
    Separate, Unified
};

} // namespace Params
} // namespace SanmapGen
