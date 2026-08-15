// MapFields_DATA.h — the computed field set generation produces.
// Layer: DATA (computed output). Aggregates the heightfield, flow/accumulation, and the TWO
// per-stratum field families the ratified field model defines (ARCH §7.2, MASKING_SPEC 1.1):
//
//   materialProportions[s]   PHYSICAL — how much of stratum s is in the column at this cell.
//                            Single writer: the sim block (NoiseBlend seeds it; Erosion and
//                            Thermal evolve and renormalize it). Read by the sims and by Mask.
//   surfaceStratumWeights[s] VISIBLE — the resolved 0..1 weight of stratum s at the surface
//                            after the slope gate, the stored-art merge and the one remap.
//                            Single writer: the Mask stage, exclusively. Read by Placement,
//                            Bake, the preview and the .sanmap export.
//
// The legacy single `materialMasks` field is retired: it named a role, not a quantity, and
// gating it in place let a renormalizing sim undo the gate (ARCH §1.1/§7.2).
// DATA-pure: depends only on FloatField, takes plain ints for sizing (no PARAMS coupling).
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
    FloatField materialProportions[stratumCount];
    FloatField surfaceStratumWeights[stratumCount];

    // Size every field to a square grid of side `vertexSize` (= mapSize + 1).
    void Resize(int vertexSize, float fillValue = 0.0f) {
        heightfield.Resize(vertexSize, vertexSize, fillValue);
        flow.Resize(vertexSize, vertexSize, fillValue);
        accumulation.Resize(vertexSize, vertexSize, fillValue);
        for (int index = 0; index < stratumCount; ++index) {
            materialProportions[index].Resize(vertexSize, vertexSize, fillValue);
            surfaceStratumWeights[index].Resize(vertexSize, vertexSize, fillValue);
        }
    }

    int VertexSize() const { return heightfield.Width(); }
    bool IsSized() const { return !heightfield.IsEmpty(); }
};

} // namespace Data
} // namespace SanmapGen
