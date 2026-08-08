#pragma once
#include <string>
#include <vector>

namespace SanmapGen {

    struct WaterSettings {
            float WaterLevelMin = 0.0f;
            float WaterLevelMax = 0.0f;
            float DeepWaterDepthMin = 8.0f;
            float DeepWaterDepthMax = 8.0f;
            float WaterWindSpeed = 0.25f;
            float WaterWindDirection = 160.0f;
            float WaterWindShoreWavesRemap = 0.5f;
            float WaterShoreDepthOffset = 8.0f;
            float WaterShoreDepthStrength = 0.7f;
            float WaterShoreDistanceOffset = 0.0f;
            float WaterShoreDistanceStrength = 2.0f;
            std::string WaveGeneratorBlueprint = "";
        };

}
