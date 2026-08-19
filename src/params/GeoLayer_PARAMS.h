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

    // Local symmetry override (SANMAP_FORMAT_SPEC Correction 3), same field names/defaults/position
    // convention as MarkerRule/PropRule/UnitRule's existing override. SETTINGS ONLY — zero PROC
    // consumer: no heightfield-symmetry stage exists in this codebase yet (Correction 4/ARCH
    // territory, explicitly deferred). Reserved from the moment it is settable (Constitution §8),
    // same posture as StratumAppearance_PARAMS.h.
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Zero PROC consumer yet (STEP16 ruling #1/#2/#3 — this override IS one of
    // the homes ARCH §13 names, retrofit in scope for this ticket).
    int  radialSymmetryRepeatCount = 3;
};

} // namespace Params
} // namespace SanmapGen
