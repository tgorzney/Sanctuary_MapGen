// Thermal_Kernel_PROC.h — the ONE talus-relaxation kernel contract both backends share.
// Layer: PROC. Declares (a) every tweakable thermal constant (Constitution §8 — this is where
// the legacy GPU "/2.0" divisor and the CPU-only ThermalRate/ThermalIterations split die) and
// (b) the cell math Thermal_PROC.glsl mirrors expression for expression, so the Cpu accuracy
// path and the Gpu speed path cannot drift (DISPATCH_INTERFACE_SPEC §4).
// The talus angle is resolved to a height threshold HERE, once, on the Cpu, and the resolved
// thresholds are uploaded — so both backends relax against bit-identical numbers.
#pragma once
#include "../data/MapFields_DATA.h"
#include "../math/Reciprocal_MATH.h"
#include "../math/Trigonometry_MATH.h"

namespace SanmapGen {
namespace Proc {

// Von Neumann stencil — the 4 axis neighbours material may slide to. Structural (it defines
// the kernel's shape), not a physical constant, so it is not a tweakable.
constexpr int   thermalNeighbourCount        = 4;
constexpr float thermalInverseNeighbourCount = 0.25f;
// Visit order is load-bearing: float accumulation order must match Thermal_PROC.glsl exactly.
constexpr int   thermalNeighbourOffsetX[thermalNeighbourCount] = { -1,  1,  0,  0 };
constexpr int   thermalNeighbourOffsetY[thermalNeighbourCount] = {  0,  0, -1,  1 };
constexpr float thermalDegreesToRadians      = 0.01745329252f;
constexpr float thermalTalusAngleCeiling     = 89.0f;   // tan() guard (Constitution §6)

// Stage constants — sane defaults, every one settable per project (Constitution §8).
struct ThermalConstants {
    int   iterationCount     = 8;      // relaxation sweeps; ONE value for both backends now
                                       // (was CPU ThermalIterations vs GPU GPUPreviewIterations)
    float relaxationRate     = 0.5f;   // 0..1 share of the over-talus excess moved per sweep.
                                       // THIS is the tweakable replacing the hardcoded "/2.0";
                                       // values above ~0.8 can overshoot and ring.
    float cellWorldSize      = 1.0f;   // horizontal spacing between heightfield vertices
    float minimumColumnDepth = 0.05f;  // remix depth floor for material transport
    float movementEpsilon    = 1.0e-7f;// below this a cell counts as stable / receives nothing
    float maskWeightEpsilon  = 1.0e-6f;// below this the material masks are treated as empty
    bool  bTransportMaterialMasks = true;  // carry the donor's material where material moves
    float talusAngleDegrees[Data::MapFields::stratumCount] = {
        35.0f, 35.0f, 35.0f, 35.0f, 35.0f, 35.0f, 35.0f, 35.0f, 35.0f };
};

// The flat float block handed to the Gpu: GpuResource_SYS exposes int uniforms only, so every
// float the kernel needs travels in one std430 buffer. The shader addresses it through
// #defines built from these slot names — one layout, declared once.
namespace ThermalConstantSlot {
    constexpr int spreadFactorActive = 0;   // relaxationRate * thermalInverseNeighbourCount
    constexpr int movementEpsilon    = 1;
    constexpr int minimumColumnDepth = 2;
    constexpr int maskWeightEpsilon  = 3;
    constexpr int talusThresholdBase = 4;   // + stratumIndex
    constexpr int totalCount         = talusThresholdBase + Data::MapFields::stratumCount;
}

// Talus angle -> the largest height difference (normalized heightfield units) one neighbour
// step may hold. Portable Sine/Cosine (no libm), so the threshold is reproducible.
inline float TalusThresholdFromAngle(float angleDegrees, float cellWorldSize, float terrainMaxHeight) {
    if (angleDegrees < 0.0f) angleDegrees = 0.0f;
    if (angleDegrees > thermalTalusAngleCeiling) angleDegrees = thermalTalusAngleCeiling;
    if (terrainMaxHeight <= 0.0f) terrainMaxHeight = 1.0f;
    const float radians = angleDegrees * thermalDegreesToRadians;
    const float tangent = Math::Sine(radians) * Math::Reciprocal(Math::Cosine(radians));
    return tangent * cellWorldSize * Math::Reciprocal(terrainMaxHeight);
}

// How much of a drop sits past the talus threshold — the only quantity that ever moves.
inline float ExcessDrop(float higherHeight, float lowerHeight, float talusThreshold) {
    const float excess = higherHeight - lowerHeight - talusThreshold;
    return excess > 0.0f ? excess : 0.0f;
}

} // namespace Proc
} // namespace SanmapGen
