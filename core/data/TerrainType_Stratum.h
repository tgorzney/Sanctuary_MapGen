#pragma once
#include <string>
#include <vector>

namespace SanmapGen {

    enum class ImportedMaskMode {
        Disabled,
        ProceduralStart,
        StaticOverride
    };

    struct StratumSettings {
            std::string Name = "Stratum";
            
            std::string AlbedoPath = "";
            std::string NormalPath = "";
            std::string MaskPath = "";
            
            float TileSize[2] = { 10.0f, 10.0f };
            float TileSizeFar[2] = { 64.0f, 64.0f };
            float TileSizeTriplanar = 12.0f;
            float TileSizeFarTriplanar = 36.0f;
            
            float NormalScale = 1.0f;
            float NormalScaleFar = 1.0f;
            float NormalFarNearBlend = 0.5f;
            float HeightFarNearBlend = 0.5f;
            
            float DiffuseRemap[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            float FarColorRemap[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            
            float PreviewColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Debug tint for Map Preview only
            
            float MaskRemapMin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float MaskRemapMax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            
            unsigned int PreviewAlbedoTex = 0;
            unsigned int PreviewNormalTex = 0;
            unsigned int PreviewMaskTex = 0;
            unsigned int PreviewActualMaskTex = 0; // UI Thumbnail for the extracted uncompressed mask
            
            ImportedMaskMode MaskMode = ImportedMaskMode::Disabled;
            std::vector<float> ImportedMaskData;
            
            // Default Soil Physics for this Stratum
            float Hardness = 0.2f;
            float Friction = 0.8f;
            float Cohesion = 0.5f;
            float CapacityMult = 2.0f;
        };

}
