// GeoLayer_PARAMS.h — a group of height layers (the "GeoLayer").
// Layer: PARAMS. Groups Layers and carries group-level options: Material vs Shaper
// mode, erode-below, and the group's blend into the stack (LAYER_SYSTEM_SPEC).
// Settings only.
#pragma once
#include <string>
#include <vector>
#include "GenerationEnums_PARAMS.h"
#include "Layer_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct GeoLayer {
    std::string        name;
    bool               bEnabled    = true;
    GeoLayerMode       mode        = GeoLayerMode::Material;
    bool               bErodeBelow = false;
    HeightBlendMode    blendMode   = HeightBlendMode::Add;
    int                stratumIndex = 0;      // stratum this group owns (Material mode)
    std::vector<Layer> layers;
};

} // namespace Params
} // namespace SanmapGen
