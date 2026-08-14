// MapFields_DATA.h — the computed field set generation produces.
// Layer: DATA (computed output). Aggregates the heightfield, flow/accumulation, and
// the 9 per-stratum material-mask weight fields, all as FloatFields. Replaces the
// scattered cached maps of the old GenerationResult. DATA-pure: depends only on
// FloatField (MATH-adjacent DATA), takes plain ints for sizing (no PARAMS coupling).
#pragma once
#include "FloatField_DATA.h"

namespace SanmapGen {
namespace Data {

class MapFields {
public:
    static constexpr int stratumCount = 9;

    FloatField heightfield;
    FloatField flow;
    FloatField accumulation;
    FloatField materialMasks[stratumCount];

    // Size every field to a square grid of side `vertexSize` (= mapSize + 1).
    void Resize(int vertexSize, float fillValue = 0.0f) {
        heightfield.Resize(vertexSize, vertexSize, fillValue);
        flow.Resize(vertexSize, vertexSize, fillValue);
        accumulation.Resize(vertexSize, vertexSize, fillValue);
        for (int index = 0; index < stratumCount; ++index)
            materialMasks[index].Resize(vertexSize, vertexSize, fillValue);
    }

    int VertexSize() const { return heightfield.Width(); }
    bool IsSized() const { return !heightfield.IsEmpty(); }
};

} // namespace Data
} // namespace SanmapGen
