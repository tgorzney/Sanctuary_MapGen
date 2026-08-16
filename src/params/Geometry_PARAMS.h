// Geometry_PARAMS.h — core map dimensions and seed (adjustable settings).
// Layer: PARAMS (the recipe; serializes to mapGeneratorData). Plain data + trivial
// derived helpers, no behavior. The heightfield is (mapSize+1) vertices per side.
#pragma once
#include <cstddef>

namespace SanmapGen {
namespace Params {

struct Geometry {
    int          mapSize          = 256;     // cells per side
    unsigned int seed             = 0u;      // generation seed
    float        terrainMaxHeight = 128.0f;  // vertical extent in game units (read from the
                                             // map, not hardcoded; entity Y is absolute)
    float        terrainMinHeight = 0.0f;    // vertical FLOOR in game units — the height a
                                             // normalized 0 maps to. Promoted to a settable
                                             // parameter by the tab rebuild (Constitution §8);
                                             // v1 carried it and never exposed it. NOTE: no
                                             // generation stage consumes it yet — every stage
                                             // still scales by terrainMaxHeight alone — so
                                             // moving it is presentation-only until a stage
                                             // work-order binds it. It is settings, not a
                                             // second height scale: nothing may derive an
                                             // extent from it without that work-order.
    bool bScaleFeaturesToMapSize = true;     // v1 parity: when set, a layer samples noise at a
                                             // frequency scaled by the map size, so a feature
                                             // keeps the same RELATIVE size when the map is
                                             // resized. Consumed by exactly one stage —
                                             // NoiseBlend (EffectiveLayerFrequency) — which is
                                             // why it lives here beside mapSize rather than on
                                             // the layer it modifies.
    float        worldUnitsPerCell = 1.0f;   // one heightfield cell -> game units (X/Z). Map
                                             // geometry, not a placement constant: Placement
                                             // emits positions with it, the preview maps an
                                             // instance onto a pixel with it. Kept explicit
                                             // rather than derived (worldExtent / mapSize)
                                             // until a worldExtent setting exists.

    // Heightfield stores one extra vertex per side (cell corners).
    int VertexSize() const { return mapSize + 1; }
    std::size_t VertexCount() const {
        std::size_t side = static_cast<std::size_t>(VertexSize());
        return side * side;
    }
    // The band a normalized 0..1 height occupies, in game units.
    float TerrainHeightSpan() const { return terrainMaxHeight - terrainMinHeight; }
    bool IsValid() const {
        return mapSize > 0 && terrainMaxHeight > 0.0f && terrainMaxHeight > terrainMinHeight;
    }
};

} // namespace Params
} // namespace SanmapGen
