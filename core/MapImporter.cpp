#include "MapImporter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "stb_image.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace SanmapGen {

bool MapImporter::LoadSanmap(const std::string& pathOrFolder, GenerationParams& outParams, std::string& outDebugLog) {
    std::ofstream dbg("debug_importer.txt");
    auto log = [&](const std::string& msg) {
        dbg << msg;
        outDebugLog += msg;
    };
    log("--- Start LoadSanmap ---\n");
    log("Input path: " + pathOrFolder + "\n");

    std::string mapdefPath = pathOrFolder;
    std::string folderPath = pathOrFolder;
    
    // If the path is a file, use it directly as the mapdef, and its parent as the folder.
    if (fs::exists(mapdefPath) && fs::is_regular_file(mapdefPath)) {
        folderPath = fs::path(mapdefPath).parent_path().string();
        log("Path is a regular file. Parent folder: " + folderPath + "\n");
    } else {
        // If it's a folder, assume mapdef.sanmap is inside it.
        mapdefPath = folderPath + "/mapdef.sanmap";
        log("Path is a folder or doesn't exist. Looking for: " + mapdefPath + "\n");
        if (!fs::exists(mapdefPath)) {
            log("Error: " + mapdefPath + " does not exist!\n");
            std::cerr << "MapImporter Error: Could not find mapdef.sanmap in " << folderPath << "\n";
            return false;
        }
    }

    std::ifstream inFile(mapdefPath);
    if (!inFile) {
        log("Error: Failed to open " + mapdefPath + " for reading.\n");
        return false;
    }

    log("Successfully opened " + mapdefPath + "\n");

    json mapdef;
    try {
        inFile >> mapdef;
        log("Successfully parsed JSON.\n");
    } catch (const std::exception& e) {
        log("JSON Parse Error: " + std::string(e.what()) + "\n");
        return false;
    } catch (...) {
        log("Unknown JSON Parse Error.\n");
        return false;
    }

    if (mapdef.contains("width")) {
        outParams.MapSize = mapdef["width"];
    }

    // Load Water
    if (mapdef.contains("waterLevelMin")) outParams.Water.WaterLevelMin = mapdef["waterLevelMin"];
    else if (mapdef.contains("waterLevel")) {
        outParams.Water.WaterLevelMin = mapdef["waterLevel"];
        outParams.Water.WaterLevelMax = mapdef["waterLevel"];
    }
    if (mapdef.contains("waterLevelMax")) outParams.Water.WaterLevelMax = mapdef["waterLevelMax"];
    
    if (mapdef.contains("deepWaterDepthMin")) outParams.Water.DeepWaterDepthMin = mapdef["deepWaterDepthMin"];
    else if (mapdef.contains("waterDepth")) {
        outParams.Water.DeepWaterDepthMin = mapdef["waterDepth"];
        outParams.Water.DeepWaterDepthMax = mapdef["waterDepth"];
    }
    if (mapdef.contains("deepWaterDepthMax")) outParams.Water.DeepWaterDepthMax = mapdef["deepWaterDepthMax"];
    
    if (mapdef.contains("waterWindSpeed")) outParams.Water.WaterWindSpeed = mapdef["waterWindSpeed"];
    if (mapdef.contains("waterWindDirection")) outParams.Water.WaterWindDirection = mapdef["waterWindDirection"];
    if (mapdef.contains("waterShoreDepthOffset")) outParams.Water.WaterShoreDepthOffset = mapdef["waterShoreDepthOffset"];
    if (mapdef.contains("waterShoreDepthStrength")) outParams.Water.WaterShoreDepthStrength = mapdef["waterShoreDepthStrength"];
    if (mapdef.contains("waterShoreDistanceOffset")) outParams.Water.WaterShoreDistanceOffset = mapdef["waterShoreDistanceOffset"];
    if (mapdef.contains("waterShoreDistanceStrength")) outParams.Water.WaterShoreDistanceStrength = mapdef["waterShoreDistanceStrength"];
    if (mapdef.contains("waveGeneratorBlueprint")) outParams.Water.WaveGeneratorBlueprint = mapdef["waveGeneratorBlueprint"];

    // Load Atmosphere
    if (mapdef.contains("sunRA")) outParams.Atmosphere.SunRA = mapdef["sunRA"];
    if (mapdef.contains("sunDA")) outParams.Atmosphere.SunDA = mapdef["sunDA"];
    if (mapdef.contains("sunIntensity")) outParams.Atmosphere.SunIntensity = mapdef["sunIntensity"];
    if (mapdef.contains("sunTint")) {
        outParams.Atmosphere.SunTint[0] = mapdef["sunTint"]["r"];
        outParams.Atmosphere.SunTint[1] = mapdef["sunTint"]["g"];
        outParams.Atmosphere.SunTint[2] = mapdef["sunTint"]["b"];
        outParams.Atmosphere.SunTint[3] = mapdef["sunTint"]["a"];
    }
    if (mapdef.contains("sunTemperature")) outParams.Atmosphere.SunTemperature = mapdef["sunTemperature"];
    if (mapdef.contains("sunAngularDiameter")) outParams.Atmosphere.SunAngularDiameter = mapdef["sunAngularDiameter"];
    if (mapdef.contains("sunVolumetricsMultiplier")) outParams.Atmosphere.SunVolumetricsMultiplier = mapdef["sunVolumetricsMultiplier"];
    if (mapdef.contains("sunVolumetricsShadowDimer")) outParams.Atmosphere.SunVolumetricsShadowDimer = mapdef["sunVolumetricsShadowDimer"];
    
    if (mapdef.contains("skylightIntensity")) outParams.Atmosphere.SkylightIntensity = mapdef["skylightIntensity"];
    if (mapdef.contains("skylightTint")) {
        outParams.Atmosphere.SkylightTint[0] = mapdef["skylightTint"]["r"];
        outParams.Atmosphere.SkylightTint[1] = mapdef["skylightTint"]["g"];
        outParams.Atmosphere.SkylightTint[2] = mapdef["skylightTint"]["b"];
        outParams.Atmosphere.SkylightTint[3] = mapdef["skylightTint"]["a"];
    }
    if (mapdef.contains("skylightTemperature")) outParams.Atmosphere.SkylightTemperature = mapdef["skylightTemperature"];
    
    if (mapdef.contains("exposure")) outParams.Atmosphere.Exposure = mapdef["exposure"];
    if (mapdef.contains("exposureCompensation")) outParams.Atmosphere.ExposureCompensation = mapdef["exposureCompensation"];
    if (mapdef.contains("skyboxExposure")) outParams.Atmosphere.SkyboxExposure = mapdef["skyboxExposure"];
    
    if (mapdef.contains("fogAttenuationDistance")) outParams.Atmosphere.FogAttenuationDistance = mapdef["fogAttenuationDistance"];
    if (mapdef.contains("fogBaseHeight")) outParams.Atmosphere.FogBaseHeight = mapdef["fogBaseHeight"];
    if (mapdef.contains("fogMaximumHeight")) outParams.Atmosphere.FogMaximumHeight = mapdef["fogMaximumHeight"];
    if (mapdef.contains("fogMaximumDistance")) outParams.Atmosphere.FogMaximumDistance = mapdef["fogMaximumDistance"];
    if (mapdef.contains("fogAnisotropy")) outParams.Atmosphere.FogAnisotropy = mapdef["fogAnisotropy"];
    
    if (mapdef.contains("skyboxPath")) outParams.Atmosphere.SkyboxPath = mapdef["skyboxPath"];
    
    if (mapdef.contains("globalWindSpeed")) outParams.Atmosphere.GlobalWindSpeed = mapdef["globalWindSpeed"];
    if (mapdef.contains("globalWindDirection")) outParams.Atmosphere.GlobalWindDirection = mapdef["globalWindDirection"];

    // Load Stratums
    if (mapdef.contains("stratums") && mapdef["stratums"].is_array()) {
        auto str = mapdef["stratums"];
        for (size_t i = 0; i < str.size() && i < outParams.Stratums.size(); ++i) {
            auto s = str[i];
            auto& strat = outParams.Stratums[i];
            
            if (s.contains("albedo")) strat.AlbedoPath = s["albedo"];
            if (s.contains("normal")) strat.NormalPath = s["normal"];
            if (s.contains("mask")) strat.MaskPath = s["mask"];
            
            if (s.contains("tileSize")) { strat.TileSize[0] = s["tileSize"]["x"]; strat.TileSize[1] = s["tileSize"]["y"]; }
            if (s.contains("tileSizeFar")) { strat.TileSizeFar[0] = s["tileSizeFar"]["x"]; strat.TileSizeFar[1] = s["tileSizeFar"]["y"]; }
            if (s.contains("tileSizeTriplanar")) strat.TileSizeTriplanar = s["tileSizeTriplanar"];
            if (s.contains("tileSizeFarTriplanar")) strat.TileSizeFarTriplanar = s["tileSizeFarTriplanar"];
            
            if (s.contains("normalScale")) strat.NormalScale = s["normalScale"];
            if (s.contains("normalScaleFar")) strat.NormalScaleFar = s["normalScaleFar"];
            if (s.contains("normalFarNearBlend")) strat.NormalFarNearBlend = s["normalFarNearBlend"];
            if (s.contains("heightFarNearBlend")) strat.HeightFarNearBlend = s["heightFarNearBlend"];
            
            if (s.contains("diffuseRemap")) { strat.DiffuseRemap[0] = s["diffuseRemap"]["x"]; strat.DiffuseRemap[1] = s["diffuseRemap"]["y"]; strat.DiffuseRemap[2] = s["diffuseRemap"]["z"]; strat.DiffuseRemap[3] = s["diffuseRemap"]["w"]; }
            if (s.contains("farColorRemap")) { strat.FarColorRemap[0] = s["farColorRemap"]["x"]; strat.FarColorRemap[1] = s["farColorRemap"]["y"]; strat.FarColorRemap[2] = s["farColorRemap"]["z"]; strat.FarColorRemap[3] = s["farColorRemap"]["w"]; }
            if (s.contains("maskRemapMin")) { strat.MaskRemapMin[0] = s["maskRemapMin"]["x"]; strat.MaskRemapMin[1] = s["maskRemapMin"]["y"]; strat.MaskRemapMin[2] = s["maskRemapMin"]["z"]; strat.MaskRemapMin[3] = s["maskRemapMin"]["w"]; }
            if (s.contains("maskRemapMax")) { strat.MaskRemapMax[0] = s["maskRemapMax"]["x"]; strat.MaskRemapMax[1] = s["maskRemapMax"]["y"]; strat.MaskRemapMax[2] = s["maskRemapMax"]["z"]; strat.MaskRemapMax[3] = s["maskRemapMax"]["w"]; }
            
            if (s.contains("physics")) {
                auto p = s["physics"];
                if (p.contains("hardness")) strat.Hardness = p["hardness"];
                if (p.contains("friction")) strat.Friction = p["friction"];
                if (p.contains("cohesion")) strat.Cohesion = p["cohesion"];
                if (p.contains("capacityMult")) strat.CapacityMult = p["capacityMult"];
            }
        }
    }

    // Process Heightmap
    std::string hmPath = folderPath + "/Textures/heightmap.raw";
    if (!fs::exists(hmPath)) {
        hmPath = folderPath + "/heightmap.raw";
    }
    
    log("Looking for heightmap: " + hmPath + "\n");
    if (fs::exists(hmPath)) {
        log("Heightmap found. Attempting to load...\n");
        std::ifstream hmFile(hmPath, std::ios::binary | std::ios::ate);
        if (hmFile) {
            std::streamsize size = hmFile.tellg();
            hmFile.seekg(0, std::ios::beg);
            
            int expectedDim = outParams.MapSize + 1;
            log("Heightmap file size: " + std::to_string(size) + " bytes. Expected dim: " + std::to_string(expectedDim) + " (" + std::to_string(expectedDim * expectedDim * sizeof(uint16_t)) + " bytes)\n");
            
            if (size == expectedDim * expectedDim * sizeof(uint16_t)) {
                std::vector<uint16_t> rawData(expectedDim * expectedDim);
                if (hmFile.read(reinterpret_cast<char*>(rawData.data()), size)) {
                    log("Heightmap successfully read and injected into GeoLayers.\n");
                    outParams.GeoLayers.clear();
                    GeoLayerDef gl;
                    gl.Name = "Imported Heightmap";
                    
                    NoiseLayer baseLayer;
                    baseLayer.Name = "Baked Heightmap";
                    baseLayer.UseImage = true;
                    baseLayer.ImageWidth = expectedDim;
                    baseLayer.ImageHeight = expectedDim;
                    baseLayer.ImageData.resize(rawData.size());
                    
                    for (size_t i = 0; i < rawData.size(); ++i) {
                        baseLayer.ImageData[i] = static_cast<float>(rawData[i]) / 65535.0f;
                    }
                    
                    gl.Layers.push_back(baseLayer);
                    outParams.GeoLayers.push_back(gl);
                }
            }
        }
    }

    // Process Splat Maps (stratums_1_4.tga, stratums_5_8.tga)
    std::string s1_4Path = folderPath + "/Textures/stratums_1_4.tga";
    if (!fs::exists(s1_4Path)) s1_4Path = folderPath + "/stratums_1_4.tga";
    
    std::string s5_8Path = folderPath + "/Textures/stratums_5_8.tga";
    if (!fs::exists(s5_8Path)) s5_8Path = folderPath + "/stratums_5_8.tga";
    
    log("Looking for splat maps: " + s1_4Path + " and " + s5_8Path + "\n");
    
    if (fs::exists(s1_4Path)) {
        log("Found stratums_1_4.tga. Loading...\n");
        int w1, h1, c1;
        unsigned char* s14Data = stbi_load(s1_4Path.c_str(), &w1, &h1, &c1, 4);
        unsigned char* s58Data = nullptr;
        
        if (fs::exists(s5_8Path)) {
            log("Found stratums_5_8.tga. Loading...\n");
            int w2, h2, c2;
            s58Data = stbi_load(s5_8Path.c_str(), &w2, &h2, &c2, 4);
        }
        
        log("s14Data loaded: " + std::string(s14Data ? "YES" : "NO") + ". Dimensions: " + std::to_string(w1) + "x" + std::to_string(h1) + " (Expected MapSize: " + std::to_string(outParams.MapSize) + ")\n");
        
        if (s14Data && w1 == outParams.MapSize) {
            log("Splat maps perfectly match map size. Extracting and converting to ImportedMaskData.\n");
            int texSize = outParams.MapSize;
            int pixelCount = texSize * texSize;
            
            for (auto& s : outParams.Stratums) {
                s.ImportedMaskData.assign(pixelCount, 0.0f);
                s.UseImportedMask = true; // Auto-enable on import
            }
            
            for (int i = 0; i < pixelCount; ++i) {
                int baseIdx = i * 4;
                if (0 < outParams.Stratums.size()) outParams.Stratums[0].ImportedMaskData[i] = s14Data[baseIdx] / 255.0f;
                if (1 < outParams.Stratums.size()) outParams.Stratums[1].ImportedMaskData[i] = s14Data[baseIdx + 1] / 255.0f;
                if (2 < outParams.Stratums.size()) outParams.Stratums[2].ImportedMaskData[i] = s14Data[baseIdx + 2] / 255.0f;
                if (3 < outParams.Stratums.size()) outParams.Stratums[3].ImportedMaskData[i] = s14Data[baseIdx + 3] / 255.0f;
                
                if (s58Data) {
                    if (4 < outParams.Stratums.size()) outParams.Stratums[4].ImportedMaskData[i] = s58Data[baseIdx] / 255.0f;
                    if (5 < outParams.Stratums.size()) outParams.Stratums[5].ImportedMaskData[i] = s58Data[baseIdx + 1] / 255.0f;
                    if (6 < outParams.Stratums.size()) outParams.Stratums[6].ImportedMaskData[i] = s58Data[baseIdx + 2] / 255.0f;
                    if (7 < outParams.Stratums.size()) outParams.Stratums[7].ImportedMaskData[i] = s58Data[baseIdx + 3] / 255.0f;
                }
            }
        }
        
        if (s14Data) stbi_image_free(s14Data);
        if (s58Data) stbi_image_free(s58Data);
    } else {
        log("stratums_1_4.tga does not exist.\n");
    }

    // Load explicitly placed markers (and wipe procedural rules)
    outParams.Markers.clear(); // User explicitly asked to disable procedural markers on map load
    if (mapdef.contains("markers") && mapdef["markers"].is_object()) {
        auto markers = mapdef["markers"];
        for (auto it = markers.begin(); it != markers.end(); ++it) {
            std::string markerType = it.key();
            auto typeObj = it.value();
            if (typeObj.contains("transforms") && typeObj["transforms"].is_object()) {
                auto transforms = typeObj["transforms"];
                for (auto tIt = transforms.begin(); tIt != transforms.end(); ++tIt) {
                    std::string transformName = tIt.key();
                    auto tVal = tIt.value();
                    
                    MarkerTransform mt;
                    mt.Type = markerType;
                    
                    if (tVal.is_array() && tVal.size() >= 3) {
                        mt.Position[0] = tVal[0];
                        mt.Position[1] = tVal[1];
                        mt.Position[2] = tVal[2];
                    } else if (tVal.is_object() && tVal.contains("position") && tVal["position"].is_object()) {
                        auto pos = tVal["position"];
                        if (pos.contains("x")) mt.Position[0] = pos["x"];
                        if (pos.contains("y")) mt.Position[1] = pos["y"];
                        if (pos.contains("z")) mt.Position[2] = pos["z"];
                    }
                    
                    outParams.MarkersList[transformName] = mt;
                }
            }
        }
    }

    log("--- End LoadSanmap (SUCCESS) ---\n");
    return true;
}

} // namespace SanmapGen
