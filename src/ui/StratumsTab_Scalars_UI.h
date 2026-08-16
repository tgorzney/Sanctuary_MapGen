// StratumsTab_Scalars_UI.h — the Stratums tab's catalogue of bounded numbers. Layer: UI.
// Accuracy class: Visual (it names limits; it computes nothing).
//
// Same device as LayerEditor_Scalars_UI.h, for the same reason: the tab draws fifteen sliders per
// stratum and nine strata, so declaring a range + an Ui::RealtimeToggle member per control would
// blow the ARCH §1.5 ceiling and scatter the limits across three translation units. Every limit is
// named ONCE here; the tab state carries flat arrays indexed by this enum. The limits stay
// per-instance tweakable (Constitution §8) — the table is the DEFAULT a StratumsTabState copies.
//
// The enumerator names are the plan's control names, fully spelled (ARCH §1.1).
#pragma once
#include "SliderScalar_UI.h"

namespace SanmapGen {
namespace Ui {

enum class StratumsTabScalar : int {
    // The one surface-weight remap (Params::Stratum, consumed by the Mask stage)
    MaskRemapMinimum, MaskRemapMaximum,
    // Tiling — the near tile count is Params::Stratum::tileCount, the rest are the appearance's
    FarTileCount, TriplanarTileCount, FarTriplanarTileCount, TileCount,
    // Normal / height detail and the far-to-near crossfade
    NormalScale, FarNormalScale, NormalFarNearBlend, HeightFarNearBlend,
    // Soil physics (Params::Stratum::soilPhysics)
    SoilHardness, SoilFriction, SoilCohesion, SoilCapacityMultiplier, SoilAbsorptionRate,
    Count
};

inline constexpr int kStratumsTabScalarCount = static_cast<int>(StratumsTabScalar::Count);

// One row of the catalogue. Mirrors LayerEditorScalarDescription so the two tables read alike;
// no Stratums control is an integer, so this one carries no `bInteger` flag to get wrong.
struct StratumsTabScalarDescription {
    const char*       label       = "";
    ScalarSliderRange range;
    const char*       valueFormat = "%.3f";
};

// The catalogue row for one control. An out-of-range enumerator answers a safe empty row rather
// than reading off the end of the table (Constitution §6).
const StratumsTabScalarDescription& StratumsTabScalarDescriptionOf(StratumsTabScalar scalar);

} // namespace Ui
} // namespace SanmapGen
