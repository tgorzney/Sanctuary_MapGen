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

    // Heightfield stores one extra vertex per side (cell corners).
    int VertexSize() const { return mapSize + 1; }
    std::size_t VertexCount() const {
        std::size_t side = static_cast<std::size_t>(VertexSize());
        return side * side;
    }
    bool IsValid() const { return mapSize > 0 && terrainMaxHeight > 0.0f; }
};

} // namespace Params
} // namespace SanmapGen
