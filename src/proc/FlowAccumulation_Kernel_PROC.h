// FlowAccumulation_Kernel_PROC.h — the one flow/accumulation kernel contract both backends
// share. Layer: PROC. Declares (a) every tweakable stage constant (Constitution §8 — nothing
// the kernels use is baked into the code or the shader), (b) the fixed neighbour table both
// backends walk in the SAME order, and (c) the stochastic tie-breaker hash. The GLSL twin
// repeats these declarations literally, so the CPU and GPU kernels cannot drift
// (DISPATCH_INTERFACE_SPEC §1/§4).
#pragma once

namespace SanmapGen {
namespace Proc {

// Stage constants — defaults only; every one is settable per project (§8).
struct FlowAccumulationConstants {
    float cellWeight               = 1.0f;        // drainage each cell contributes (uniform rainfall)
    float flowNoiseImpact          = 0.0f;        // stochastic SFD: unitNoise * this, added to the weighted drop
    float depressionFillEpsilon    = 1.0e-5f;     // priority-flood monotone rise across a filled basin
    float cardinalInverseDistance  = 1.0f;        // precomputed reciprocals — never divide inside the loop
    float diagonalInverseDistance  = 0.70710678f; // 1 / sqrt(2)
    float flowMagnitudeScale       = 1.0f;        // MapFields.flow = path slope * this
    unsigned int flowNoiseSeed     = 0u;          // hashed with (cellX, cellY, neighbourIndex)
    int  gpuFillIterationLimit         = 0;       // 0 = automatic (fillIterationsPerSide * vertexSize)
    int  gpuAccumulationIterationLimit = 0;       // 0 = automatic (accumulationIterationsPerSide * vertexSize)
    int  gpuConvergenceCheckInterval   = 16;      // iterations between the early-out convergence probes
    int  fillIterationsPerSide         = 4;
    int  accumulationIterationsPerSide = 4;
    bool bFillDepressions          = true;        // route through pits/flats out to the map border
    bool bNormalizeAccumulation    = false;       // Exact/Output keeps raw drainage counts (conservation)
};

// The std430 record uploaded to the GPU. Order and type are load-bearing (the GLSL constant
// block repeats it verbatim); integers travel as uniforms because the SYS seam exposes int
// uniforms only. 8 floats keeps the block a 16-byte multiple.
struct FlowAccumulationKernelRecord {
    float cellWeight              = 1.0f;
    float flowNoiseImpact         = 0.0f;
    float depressionFillEpsilon   = 1.0e-5f;
    float cardinalInverseDistance = 1.0f;
    float diagonalInverseDistance = 0.70710678f;
    float flowMagnitudeScale      = 1.0f;
    float unusedPaddingA          = 0.0f;
    float unusedPaddingB          = 0.0f;
};

constexpr int flowNeighbourCount = 8;
constexpr int flowSinkDirection  = -1;   // no strictly lower neighbour: drainage terminates here

// The neighbour order is part of the contract: opposite(index) == (index ^ 1). That identity
// is what lets the GPU accumulation pass GATHER from the cells draining into a cell instead
// of scattering into them — no float read-modify-write race (DISPATCH_INTERFACE_SPEC §1).
constexpr int flowNeighbourOffsetX[flowNeighbourCount] = { -1,  1,  0,  0, -1,  1,  1, -1 };
constexpr int flowNeighbourOffsetY[flowNeighbourCount] = {  0,  0, -1,  1, -1,  1, -1,  1 };
constexpr bool bFlowNeighbourIsDiagonal[flowNeighbourCount] =
    { false, false, false, false, true, true, true, true };

// Planchon-Darboux seed for interior cells: the fill relaxes DOWN to the priority-flood
// answer from here, so it must start above every reachable terrain height.
constexpr float flowUnfilledSurfaceHeight = 1.0e30f;

// Reproducible per-(cell, neighbour) unit noise. Integer-only mixing, so the CPU and the GLSL
// twin produce bit-identical values on every machine (DETERMINISM_SPEC).
inline unsigned int FlowNoiseHash(unsigned int cellX, unsigned int cellY,
                                  unsigned int neighbourIndex, unsigned int seed) {
    unsigned int mixed = cellX * 374761393u + cellY * 668265263u
                       + neighbourIndex * 2246822519u + seed * 3266489917u;
    mixed = (mixed ^ (mixed >> 13)) * 1274126177u;
    return mixed ^ (mixed >> 16);
}

// Top 24 bits -> a float in [0, 1) that is exactly representable, so no rounding can differ.
inline float FlowNoiseUnit(unsigned int cellX, unsigned int cellY,
                           unsigned int neighbourIndex, unsigned int seed) {
    return static_cast<float>(FlowNoiseHash(cellX, cellY, neighbourIndex, seed) >> 8)
         * (1.0f / 16777216.0f);
}

} // namespace Proc
} // namespace SanmapGen
