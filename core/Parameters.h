#pragma once
#include <string>
#include <vector>

namespace SanmapGen {

    // Bitmask for Symmetry Mixing
    enum SymmetryFlags {
        Symmetry_None   = 0,
        Symmetry_Point  = 1 << 0,
        Symmetry_X      = 1 << 1,
        Symmetry_Z      = 1 << 2,
        Symmetry_XY     = 1 << 3,
        Symmetry_Radial = 1 << 4
    };

    enum class BlendMode {
        Add,
        Subtract,
        Multiply,
        Overlay
    };

    enum class NoiseType {
        OpenSimplex2,
        OpenSimplex2S,
        Cellular,
        Perlin,
        ValueCubic,
        Value
    };

    enum class FractalType {
        None,
        FBm,
        Ridged,
        PingPong
    };

    struct NoiseLayer {
        std::string Name = "New Layer";
        bool Enabled = true;
        
        NoiseType Type = NoiseType::OpenSimplex2;
        FractalType Fractal = FractalType::FBm;
        int SymmetryMask = Symmetry_None;
        BlendMode Blend = BlendMode::Add;
        
        float Frequency = 0.01f;
        int Octaves = 4;
        float Gain = 0.5f;
        float PingPongStrength = 2.0f;
        float Opacity = 1.0f; // Multiplier/Weight for this layer
        float CellularJitter = 1.0f;
        
        // Terrain Density Shaping
        float LandDensity = 0.5f;
        float PlateauDensity = 0.0f;
        float MountainDensity = 0.0f;
        float RampDensity = 0.0f;
    };

    enum class SymmetryAlgorithm {
        Fold,
        Blur,
        CrossFade,
        Cylinder3D,
        Torus3D,
        NativeHash
    };

    struct GenerationParams {
        // --- General ---
        int Seed = 12345;
        int MapSize = 512;
        
        // --- Symmetry Globals ---
        SymmetryAlgorithm SymAlgorithm = SymmetryAlgorithm::Fold;
        float SymmetryBlurRadius = 10.0f;
        float CrossFadeWidth = 0.2f; // Radians to crossfade
        float CylinderZScale = 1.0f; // Stretch the cylinder length
        float TorusMajorRadius = 128.0f; // Donut ring size
        float TorusMinorRadius = 64.0f; // Donut tube size
        
        // --- The Dynamic Layer Stack ---
        std::vector<NoiseLayer> Layers;
        
        // --- Gameplay ---
        int SpawnPointCount = 2;
        float AlloyMultiplier = 1.0f;
        float HydroMultiplier = 1.0f;
        
        // --- Water (Sanmap Settings) ---
        float WaterLevelMin = 20.0f;
        float WaterLevelMax = 40.0f;
        float DeepWaterDepthMin = 10.0f;
        float DeepWaterDepthMax = 30.0f;
        
        std::string WaveGeneratorBlueprint = "";
        
        // Default constructor to push one base layer
        GenerationParams() {
            NoiseLayer baseLayer;
            baseLayer.Name = "Base Mountain (Low Freq)";
            baseLayer.Frequency = 0.005f;
            baseLayer.Octaves = 5;
            baseLayer.Gain = 0.5f;
            baseLayer.PingPongStrength = 2.0f;
            baseLayer.Opacity = 1.0f;
            Layers.push_back(baseLayer);
        }
    };

} // namespace SanmapGen
