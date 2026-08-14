// Mask_Kernel_PROC.h — the one mask kernel contract shared by both backends.
// Layer: PROC. Declares (a) every tweakable stage constant (Constitution §8 — nothing the
// kernels use is baked into code or shader) and (b) the per-stratum configuration record,
// whose field order/type is the std430 layout the GLSL twin mirrors EXACTLY. Declared once
// here so the CPU struct and the shader block can never drift (DISPATCH_INTERFACE_SPEC §4).
#pragma once

namespace SanmapGen {
namespace Proc {

// Stage constants — defaults only; every one is settable per project (§8).
struct MaskConstants {
    float cellSize                 = 1.0f;   // world units between two heightfield vertices
    float degreesToRadians         = 0.017453292519943295f;
    float maximumSlopeDegreesLimit = 89.9f;  // tan() guard: 90 degrees is an infinite gradient
    float smoothstepShoulder       = 3.0f;   // smoothstep = t*t*(shoulder - scale*t)
    float smoothstepScale          = 2.0f;
    float maskMinimum              = 0.0f;   // output clamp window for every mask value
    float maskMaximum              = 1.0f;
    float centralDifferenceSpan    = 2.0f;   // interior gradient spans two cells
};

// One stratum's flattened mask configuration, ready for either backend. 24 scalars = 96
// bytes, a multiple of 16 so the std430 array stride needs no extra padding. The float
// copies of the stage-wide values exist because the SYS seam exposes int uniforms only —
// the shader therefore reads every float from this block. Order is load-bearing.
struct MaskStratumConfiguration {
    int   mergeMode           = 0;   // Params::ImportedMaskMode as int
    int   storedMaskOffset    = 0;   // first element of this stratum's art in the packed buffer
    int   storedMaskWidth     = 0;   // 0 = no stored art (merge falls back to procedural)
    int   storedMaskHeight    = 0;
    int   bSmoothstepEnabled  = 0;   // 0 = hard clamp: binary in/out of the slope window
    int   bInvertEnabled      = 0;
    int   paddingFirst        = 0;
    int   paddingSecond       = 0;
    float slopeGradientLow    = 0.0f;   // tan(minimumSlopeDegrees) — the pinned gradient unit
    float slopeGradientHigh   = 0.0f;   // tan(maximumSlopeDegrees)
    float inverseFeatherLow   = 0.0f;   // 1/feather; 0 marks a hard (unfeathered) edge
    float inverseFeatherHigh  = 0.0f;
    float gateStrength        = 0.0f;   // 0 = gate disabled (weight stays 1)
    float remapMinimum        = 0.0f;
    float inverseRemapRange   = 1.0f;   // 1/(max-min); 0 marks a degenerate remap window
    float heightScale         = 1.0f;   // Geometry.terrainMaxHeight — read from the map, not 128
    float inverseSingleSpan   = 1.0f;   // 1/(1*cellSize)  — edge cells, one-sided difference
    float inverseDoubleSpan   = 0.5f;   // 1/(2*cellSize)  — interior cells, central difference
    float smoothstepShoulder  = 3.0f;
    float smoothstepScale     = 2.0f;
    float maskMinimum         = 0.0f;
    float maskMaximum         = 1.0f;
    float storedSampleScaleX  = 0.0f;   // vertex x -> stored column: (storedWidth-1)/(vertexSize-1)
    float storedSampleScaleY  = 0.0f;   // the ONE resampler is bilinear, never nearest
};

} // namespace Proc
} // namespace SanmapGen
