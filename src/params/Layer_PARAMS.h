// Layer_PARAMS.h — one height layer's settings (the NoiseLayer replacement).
// Layer: PARAMS (part of the editable layer stack — the recipe). Settings ONLY:
// per ARCH §5.2 the god-object's image-bake state, per-layer erosion, placement
// fields, and physics tags are evicted elsewhere. No behavior, no computed data.
#pragma once
#include <string>
#include "GenerationEnums_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct Layer {
    // Identity / stack control
    std::string name = "Layer";  // designer-facing label only. Pure metadata: NO stage consumes
                                 // it, so it is deliberately absent from every parameter hash —
                                 // renaming a layer must not re-run generation (WO B2).
    bool bEnabled      = true;
    bool bLocked       = false;
    int  stratumIndex  = 0;      // 0..8

    // Noise source
    NoiseType   noiseType        = NoiseType::OpenSimplex2;
    FractalType fractalType      = FractalType::FractionalBrownian;
    float       frequency        = 0.005f;
    int         octaves          = 5;
    float       gain             = 0.5f;   // persistence
    float       lacunarity       = 2.0f;   // per-octave frequency multiplier (was a hardcoded
                                           // 2.0 inside the kernels — Constitution §8)
    float       weightedStrength = 0.0f;   // octave amplitude weighting
    float       pingPongStrength = 2.0f;   // PingPong fractal only
    float       cellularJitter   = 1.0f;   // Cellular noise only

    // Post-noise density shaping (NOISE_BLEND_SPEC "density" group). 0.5 land density is
    // the identity (the reshape scales by landDensity * 2); 0 disables the others.
    float landDensity     = 0.5f;
    float mountainDensity = 0.0f;
    float plateauDensity  = 0.0f;   // terracing
    float rampDensity     = 0.0f;

    // Post-noise reshaping (Photoshop "Levels")
    float levelsShadows     = 0.0f;
    float levelsMidtones    = 1.0f;
    float levelsHighlights  = 1.0f;
    float levelsOutputBlack = 0.0f;
    float levelsOutputWhite = 1.0f;

    // Combine into the stack
    HeightBlendMode blendMode           = HeightBlendMode::Add;
    float           opacity             = 1.0f;
    float           heightBlendContrast = 1.0f;
    float           heightBlendMinimum  = 0.0f;
    float           heightBlendMaximum  = 1.0f;

    // Local symmetry override (SANMAP_FORMAT_SPEC Correction 3), same field names/defaults/position
    // convention as MarkerRule/PropRule/UnitRule's existing override. SETTINGS ONLY — zero PROC
    // consumer: no heightfield-symmetry stage exists in this codebase yet (Correction 4/ARCH
    // territory, explicitly deferred). Reserved from the moment it is settable (Constitution §8),
    // same posture as StratumAppearance_PARAMS.h.
    //
    // NAMED GAP, explicitly deferred (not this ticket's to fix): the real map format's per-layer
    // MinHeight/MaxHeight/MinSlope/MaxSlope height-and-slope gates have no equivalent field here at
    // all — silently dropped in the v1->v2 port, not merely carried over. Logged for a future
    // LAYER_SYSTEM_SPEC conversation.
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Zero PROC consumer yet (STEP16 ruling #1/#2/#3 — this override IS one of
    // the homes ARCH §13 names, retrofit in scope for this ticket).
    int  radialSymmetryRepeatCount = 3;
};

} // namespace Params
} // namespace SanmapGen
