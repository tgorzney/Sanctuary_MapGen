#pragma once
#include "Params_Enums.h"
#include "Params_Gradients.h"
#include <string>
#include <vector>
#include <map>


namespace SanmapGen {
    struct ErosionSettings {
        bool Enabled = false;
        
        int DropletCount = 1000000;
        int MaxLifetime = 15;
        bool GravityUseGlobal = true; // If true, use GenerationParams::GlobalGravity
        float Gravity = 4.0f;         // Per-erosion gravity override
        float EvaporationRate = 0.02f;

        // Precipitation
        bool UseRainNoise = true;
        float RainNoiseFreq = 0.01f;
        int RainNoiseOctaves = 4;
        float RainNoiseThreshold = 0.5f;

        // Orographic
        bool UseOrographicRain = true;
        float WindAngle = 45.0f; // degrees

        // Deposition (Soil Dropping) Pass
        bool DepositionMode = false;
        float SpawnMinHeight = 0.0f;
        float SpawnMaxHeight = 1.0f;
        float InitialSedimentLoad = 1.0f;

        // --- Scientific Flow Variables (TG_UE Architecture) ---
        float FluidViscosity = 1.0f;
        float BaseAbsorptionRate = 0.05f;
        float CarryingCapacityScale = 1.0f;
    };

    struct FlowSettings {
        float Precipitation = 1.0f;
        int Iterations = 50;
        bool UseGPU = false; // Toggle CPU vs GPU
        
        // --- God-Tier Stochastic Flow Variables ---
        float FlowVolumeMultiplier = 1.0f;
        float StochasticVariance = 0.5f;
        float SlopeAdherence = 0.8f;
        float FlowMomentum = 0.2f;
        
        // --- Accumulation Variables ---
        bool AccurateSimultaneousAccumulation = false;
        float SpilloverThreshold = 0.01f;
        
        GradientSettings Gradient = {
            "Default", 
            {
                {0.0f, {0.0f, 0.0f, 0.2f, 1.0f}}, 
                {50.0f, {0.0f, 0.4f, 0.8f, 1.0f}}, 
                {100.0f, {0.0f, 1.0f, 1.0f, 1.0f}}
            },
            true
        };
    };


}
