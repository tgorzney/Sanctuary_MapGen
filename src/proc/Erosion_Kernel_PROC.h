// Erosion_Kernel_PROC.h — the one erosion kernel contract both backends consume.
// Layer: PROC. Declares (a) the flattened per-pass configuration whose field order/type is
// the std430 layout Erosion_PROC.glsl mirrors EXACTLY (DISPATCH_INTERFACE_SPEC §4), (b) the
// fixed-point erosion state conversion, and (c) the integer hash random stream. The hash and
// the fixed-point conversion are pure integer/float ops with no libm call, so CPU and GPU
// agree bit-for-bit and a deterministic re-run reproduces itself (DETERMINISM_SPEC).
#pragma once

namespace SanmapGen {
namespace Proc {

// Layout of one stratum's physics record inside the flat material buffer both backends read.
constexpr int materialPhysicsStride            = 8;   // floats per stratum (16-byte multiple)
constexpr int materialPhysicsHardnessOffset    = 0;
constexpr int materialPhysicsFrictionOffset    = 1;
constexpr int materialPhysicsCohesionOffset    = 2;
constexpr int materialPhysicsCapacityOffset    = 3;
constexpr int materialPhysicsAbsorptionOffset  = 4;
constexpr int materialPhysicsErodableOffset    = 5;   // 1.0 = erodable, 0.0 = not

// One erosion pass, ready for either backend. 10 ints + 22 floats = 128 bytes, so the
// std430 array stride stays a 16-byte multiple. Order is load-bearing: the GLSL block
// repeats it verbatim.
struct ErosionKernelConfiguration {
    int dropletCount            = 0;
    int maximumLifetime         = 0;
    int vertexSize              = 0;
    int stratumCount            = 0;
    int depositStratum          = 0;   // sediment lands here
    int highestErodableStratum  = 0;   // carve top-down from here to stratum 0
    int bDepositionMode         = 0;
    int randomSeed              = 0;
    int bConserveSedimentAtExit = 1;   // dump the remaining load at the last valid cell
    int integerPadding          = 0;

    float heightFixedPointScale   = 1048576.0f;
    float heightFixedPointInverse = 1.0f / 1048576.0f;
    float baseErosionRate         = 0.3f;
    float baseDepositionRate      = 0.3f;
    float evaporationRate         = 0.02f;
    float gravity                 = 4.0f;
    float capacityBaseMultiplier  = 4.0f;
    float carryingCapacityScale   = 1.0f;
    float capacityMinimum         = 0.01f;
    float inertiaBase             = 0.05f;
    float inertiaFrictionScale    = 0.1f;
    float viscosityReciprocal     = 1.0f;   // 1/max(minimumViscosity, viscosity) — never divide
    float meanderStrength         = 0.0f;
    float divergenceFactor        = 0.0f;   // 1 - slopeAdherence
    float divergenceThreshold     = 1.0f;
    float waterMinimum            = 0.001f;
    float sedimentMinimum         = 0.0001f;
    float thicknessEpsilon        = 0.0001f;
    float initialSedimentLoad     = 0.0f;
    float boundaryMargin          = 1.0f;
    float depositionModeCapacityGain = 100.0f;
    float floatPadding            = 0.0f;
};

// --- fixed-point erosion state -------------------------------------------------------
// Round-half-away-from-zero; `int(x)` truncates toward zero in both C++ and GLSL, so the
// two implementations produce the identical tick count for the identical float.
inline int HeightToFixedPoint(float height, float fixedPointScale) {
    float scaled = height * fixedPointScale;
    return static_cast<int>(scaled + (scaled >= 0.0f ? 0.5f : -0.5f));
}
inline float FixedPointToHeight(int fixedTicks, float fixedPointInverse) {
    return static_cast<float>(fixedTicks) * fixedPointInverse;
}

// --- shared random stream ------------------------------------------------------------
// lowbias32 finaliser: pure uint32 shifts/multiplies, wrapping identically in C++ and GLSL.
inline unsigned int HashRandomUnsigned(unsigned int state) {
    state ^= state >> 16;
    state *= 0x7feb352du;
    state ^= state >> 15;
    state *= 0x846ca68bu;
    state ^= state >> 16;
    return state;
}
inline unsigned int HashRandomCombine(unsigned int first, unsigned int second) {
    return HashRandomUnsigned(first * 0x9e3779b9u + second);
}
// Uniform in [0,1): the top 24 bits are exactly representable as a float, so the divide is
// exact and the same on every machine.
inline float HashRandomUnitFloat(unsigned int state) {
    return static_cast<float>(HashRandomUnsigned(state) >> 8) * (1.0f / 16777216.0f);
}

} // namespace Proc
} // namespace SanmapGen
