#pragma once

namespace SanmapGen {

    enum SymmetryFlags {
        Symmetry_None   = 0,
        Symmetry_Point  = 1 << 0,
        Symmetry_X      = 1 << 1,
        Symmetry_Z      = 1 << 2,
        Symmetry_XY     = 1 << 3,
        Symmetry_Radial = 1 << 4
    };

    enum class SymmetryAlgorithm {
        Fold,
        Blur,
        CrossFade,
        Cylinder3D,
        Torus3D,
        NativeHash,
        Superposition
    };

    enum MarkerPriority {
        Priority_LargestArea = 0,
        Priority_SmallestArea = 1,
        Priority_LeastVariance = 2
    };
    
    enum MarkerGradientType {
        Gradient_None = 0,
        Gradient_CenterFocus = 1,
        Gradient_EdgeFocus = 2,
        Gradient_Torus = 3
    };

    enum class BlendMode {
        Add,
        Subtract,
        Multiply,
        Overlay,
        Max,
        Min
    };

    enum class NoiseType {
        OpenSimplex2,
        OpenSimplex2S,
        Cellular,
        Perlin,
        ValueCubic,
        Value,
        None
    };

    enum class FractalType {
        None,
        FBm,
        Ridged,
        PingPong
    };

    enum class ImportedMaskMode {
        Disabled,
        ProceduralStart,
        StaticOverride
    };

    enum class LayerType {
        Manual,
        Fixed
    };

}
