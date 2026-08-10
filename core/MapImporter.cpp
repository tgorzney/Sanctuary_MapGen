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

    // Clear old state before loading to prevent leaked array dimension bugs
    outParams.MarkersList.clear();
    outParams.StaticPropsList.clear();
    outParams.GeoLayers.clear();
    for (auto& s : outParams.Stratums) {
        s.importedMaskData.clear();
        s.maskMode = ImportedMaskMode::Disabled;
    }

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

    outParams.MapFolderPath = folderPath;

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
    if (mapdef.contains("waterWindShoreWavesRemap")) outParams.Water.WaterWindShoreWavesRemap = mapdef["waterWindShoreWavesRemap"];
    if (mapdef.contains("waterShoreDepthOffset")) outParams.Water.WaterShoreDepthOffset = mapdef["waterShoreDepthOffset"];
    if (mapdef.contains("waterShoreDepthStrength")) outParams.Water.WaterShoreDepthStrength = mapdef["waterShoreDepthStrength"];
    if (mapdef.contains("waterShoreDistanceOffset")) outParams.Water.WaterShoreDistanceOffset = mapdef["waterShoreDistanceOffset"];
    if (mapdef.contains("waterShoreDistanceStrength")) outParams.Water.WaterShoreDistanceStrength = mapdef["waterShoreDistanceStrength"];
    if (mapdef.contains("waterShoreGeneratorBlueprint")) outParams.Water.WaveGeneratorBlueprint = mapdef["waterShoreGeneratorBlueprint"];

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
    if (mapdef.contains("sunCookie") && mapdef["sunCookie"].is_object() && mapdef["sunCookie"].contains("path")) outParams.Atmosphere.SunCookiePath = mapdef["sunCookie"]["path"];
    if (mapdef.contains("sunCookieSize") && mapdef["sunCookieSize"].is_object()) {
        if (mapdef["sunCookieSize"].contains("x")) outParams.Atmosphere.SunCookieSize[0] = mapdef["sunCookieSize"]["x"];
        if (mapdef["sunCookieSize"].contains("y")) outParams.Atmosphere.SunCookieSize[1] = mapdef["sunCookieSize"]["y"];
    }
    
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
    
    
    if (mapdef.contains("skybox") && mapdef["skybox"].is_object() && mapdef["skybox"].contains("path")) outParams.Atmosphere.SkyboxPath = mapdef["skybox"]["path"];
    if (mapdef.contains("skyboxRotation")) outParams.Atmosphere.SkyboxRotation = mapdef["skyboxRotation"];
    if (mapdef.contains("skyboxIntensityMode")) {
        std::string mode = mapdef["skyboxIntensityMode"];
        if (mode == "Exposure") outParams.Atmosphere.SkyboxIntensityMode = SkyIntensityMode::Exposure;
        else if (mode == "Lux") outParams.Atmosphere.SkyboxIntensityMode = SkyIntensityMode::Lux;
        else if (mode == "Multiplier") outParams.Atmosphere.SkyboxIntensityMode = SkyIntensityMode::Multiplier;
    }
    if (mapdef.contains("skyboxMultiplier")) outParams.Atmosphere.SkyboxMultiplier = mapdef["skyboxMultiplier"];
    if (mapdef.contains("skyboxLuxValue")) outParams.Atmosphere.SkyboxLuxValue = mapdef["skyboxLuxValue"];

    
    if (mapdef.contains("windSpeed")) outParams.Atmosphere.GlobalWindSpeed = mapdef["windSpeed"];
    if (mapdef.contains("windDirection")) outParams.Atmosphere.GlobalWindDirection = mapdef["windDirection"];
    
    // Background Fog
    if (mapdef.contains("backgroundFogIntensity")) outParams.Atmosphere.BackgroundFogIntensity = mapdef["backgroundFogIntensity"];
    if (mapdef.contains("backgroundFogRange")) outParams.Atmosphere.BackgroundFogRange = mapdef["backgroundFogRange"];
    if (mapdef.contains("backgroundFogMinimum")) outParams.Atmosphere.BackgroundFogMinimum = mapdef["backgroundFogMinimum"];
    if (mapdef.contains("backgroundSkyColorIntensity")) outParams.Atmosphere.BackgroundSkyColorIntensity = mapdef["backgroundSkyColorIntensity"];
    if (mapdef.contains("backgroundColorIntensity")) outParams.Atmosphere.BackgroundColorIntensity = mapdef["backgroundColorIntensity"];
    if (mapdef.contains("backgroundColor") && mapdef["backgroundColor"].is_object()) {
        auto bc = mapdef["backgroundColor"];
        if(bc.contains("r")) outParams.Atmosphere.BackgroundColor[0] = bc["r"];
        if(bc.contains("g")) outParams.Atmosphere.BackgroundColor[1] = bc["g"];
        if(bc.contains("b")) outParams.Atmosphere.BackgroundColor[2] = bc["b"];
        if(bc.contains("a")) outParams.Atmosphere.BackgroundColor[3] = bc["a"];
    }
    if (mapdef.contains("backgroundColorFadeoutRange")) outParams.Atmosphere.BackgroundColorFadeoutRange = mapdef["backgroundColorFadeoutRange"];
    if (mapdef.contains("backgroundColorFadeoutPower")) outParams.Atmosphere.BackgroundColorFadeoutPower = mapdef["backgroundColorFadeoutPower"];
    
    // Height Fog
    if (mapdef.contains("heightFogIntensity")) outParams.Atmosphere.HeightFogIntensity = mapdef["heightFogIntensity"];
    if (mapdef.contains("heightFogRange") && mapdef["heightFogRange"].is_object()) {
        auto hfr = mapdef["heightFogRange"];
        if(hfr.contains("x")) outParams.Atmosphere.HeightFogRange[0] = hfr["x"];
        if(hfr.contains("y")) outParams.Atmosphere.HeightFogRange[1] = hfr["y"];
    }
    if (mapdef.contains("heightFogStart")) outParams.Atmosphere.HeightFogStart = mapdef["heightFogStart"];
    if (mapdef.contains("heightFogEnd")) outParams.Atmosphere.HeightFogEnd = mapdef["heightFogEnd"];
    if (mapdef.contains("heightFogPower")) outParams.Atmosphere.HeightFogPower = mapdef["heightFogPower"];
    
    // Linear Fog
    if (mapdef.contains("linearFogIntensity")) outParams.Atmosphere.LinearFogIntensity = mapdef["linearFogIntensity"];
    if (mapdef.contains("linearFogStart")) outParams.Atmosphere.LinearFogStart = mapdef["linearFogStart"];
    if (mapdef.contains("linearFogEnd")) outParams.Atmosphere.LinearFogEnd = mapdef["linearFogEnd"];
    if (mapdef.contains("linearFogPower")) outParams.Atmosphere.LinearFogPower = mapdef["linearFogPower"];
    if (mapdef.contains("linearFogCameraIntensity")) outParams.Atmosphere.LinearFogCameraIntensity = mapdef["linearFogCameraIntensity"];
    if (mapdef.contains("linearFogCameraStart")) outParams.Atmosphere.LinearFogCameraStart = mapdef["linearFogCameraStart"];
    if (mapdef.contains("linearFogCameraEnd")) outParams.Atmosphere.LinearFogCameraEnd = mapdef["linearFogCameraEnd"];


    // Load Stratums
    if (mapdef.contains("stratumLayers") && mapdef["stratumLayers"].is_array()) {
        auto str = mapdef["stratumLayers"];
        for (size_t i = 0; i < str.size() && i < outParams.Stratums.size(); ++i) {
            auto s = str[i];
            auto& strat = outParams.Stratums[i];
            
            if (s.contains("albedo") && s["albedo"].is_object() && s["albedo"].contains("path")) {
                strat.albedo.path = s["albedo"]["path"];
                std::string pathStr = strat.albedo.path;
                
                // Attempt to reverse engineer EnvironmentTheme and MaterialName
                // Format: Environment/ThemeName/Stratum/MaterialName_albedo.dds
                size_t envPos = pathStr.find("Environment/");
                if (envPos != std::string::npos) {
                    size_t themeStart = envPos + 12;
                    size_t themeEnd = pathStr.find('/', themeStart);
                    if (themeEnd != std::string::npos) {
                        strat.EnvironmentTheme = pathStr.substr(themeStart, themeEnd - themeStart);
                        
                        size_t fileStart = pathStr.find_last_of('/');
                        if (fileStart != std::string::npos) {
                            std::string filename = pathStr.substr(fileStart + 1);
                            size_t albedoPos = filename.find("_albedo");
                            if (albedoPos != std::string::npos) {
                                strat.MaterialName = filename.substr(0, albedoPos);
                            }
                        }
                    }
                }
            }
            if (s.contains("normal") && s["normal"].is_object() && s["normal"].contains("path")) strat.normal.path = s["normal"]["path"];
            if (s.contains("mask") && s["mask"].is_object() && s["mask"].contains("path")) strat.mask.path = s["mask"]["path"];
            
            auto parseVec2 = [](const json& j, SanVector2& out) {
                if (j.is_array() && j.size() >= 2) { out.x = j[0]; out.y = j[1]; }
                else if (j.is_object()) {
                    if (j.contains("x")) out.x = j["x"];
                    if (j.contains("y")) out.y = j["y"];
                }
            };
            auto parseColor = [](const json& j, SanColor& out) {
                if (j.is_array() && j.size() >= 4) { out.r = j[0]; out.g = j[1]; out.b = j[2]; out.a = j[3]; }
                else if (j.is_object()) {
                    if (j.contains("r")) out.r = j["r"];
                    if (j.contains("g")) out.g = j["g"];
                    if (j.contains("b")) out.b = j["b"];
                    if (j.contains("a")) out.a = j["a"];
                }
            };
            auto parseVec4 = [](const json& j, SanVector4& out) {
                if (j.is_array() && j.size() >= 4) { out.x = j[0]; out.y = j[1]; out.z = j[2]; out.w = j[3]; }
                else if (j.is_object()) {
                    if (j.contains("x")) out.x = j["x"];
                    if (j.contains("y")) out.y = j["y"];
                    if (j.contains("z")) out.z = j["z"];
                    if (j.contains("w")) out.w = j["w"];
                }
            };

            if (s.contains("tileSize")) parseVec2(s["tileSize"], strat.tileSize);
            if (s.contains("tileSizeFar")) parseVec2(s["tileSizeFar"], strat.tileSizeFar);
            if (s.contains("tileSizeTriplanar")) strat.tileSizeTriplanar = s["tileSizeTriplanar"];
            if (s.contains("tileSizeFarTriplanar")) strat.tileSizeFarTriplanar = s["tileSizeFarTriplanar"];
            
            if (s.contains("normalScale")) strat.normalScale = s["normalScale"];
            if (s.contains("normalScaleFar")) strat.normalScaleFar = s["normalScaleFar"];
            if (s.contains("normalFarNearBlend")) strat.normalFarNearBlend = s["normalFarNearBlend"];
            if (s.contains("heightFarNearBlend")) strat.heightFarNearBlend = s["heightFarNearBlend"];
            
            if (s.contains("diffuseRemap")) parseColor(s["diffuseRemap"], strat.diffuseRemap);
            if (s.contains("farColorRemap")) parseColor(s["farColorRemap"], strat.farColorRemap);
            if (s.contains("maskRemapMin")) parseVec4(s["maskRemapMin"], strat.maskRemapMin);
            if (s.contains("maskRemapMax")) parseVec4(s["maskRemapMax"], strat.maskRemapMax);
            
            if (s.contains("hardness")) strat.hardness = s["hardness"];
            if (s.contains("friction")) strat.friction = s["friction"];
            if (s.contains("cohesion")) strat.cohesion = s["cohesion"];
            if (s.contains("capacityMult")) strat.capacityMult = s["capacityMult"];
        }
    }

    // Load Armies
    if (mapdef.contains("armies") && mapdef["armies"].is_object()) {
        outParams.Armies.clear();
        std::function<UnitGroup(const json&)> loadGroup;
        loadGroup = [&](const json& gj) -> UnitGroup {
            UnitGroup g;
            if (gj.contains("units") && gj["units"].is_object()) {
                for (auto it = gj["units"].begin(); it != gj["units"].end(); ++it) {
                    const auto& uj = it.value();
                    UnitTransform u;
                    if (uj.contains("type")) u.Type = uj["type"];
                    if (uj.contains("tpid")) u.Tpid = uj["tpid"];
                    if (uj.contains("position")) { u.Position[0] = uj["position"]["x"]; u.Position[1] = uj["position"]["y"]; u.Position[2] = uj["position"]["z"]; }
                    if (uj.contains("rotation")) { u.Rotation[0] = uj["rotation"]["x"]; u.Rotation[1] = uj["rotation"]["y"]; u.Rotation[2] = uj["rotation"]["z"]; u.Rotation[3] = uj["rotation"]["w"]; }
                    if (uj.contains("scale")) { u.Scale[0] = uj["scale"]["x"]; u.Scale[1] = uj["scale"]["y"]; u.Scale[2] = uj["scale"]["z"]; }
                    g.Units[it.key()] = u;
                }
            }
            if (gj.contains("groups") && gj["groups"].is_object()) {
                for (auto it = gj["groups"].begin(); it != gj["groups"].end(); ++it) {
                    g.Groups[it.key()] = loadGroup(it.value());
                }
            }
            return g;
        };

        const float defaultColors[8][4] = {
            {1.0f, 0.0f, 0.0f, 1.0f}, // Red
            {1.0f, 0.4f, 0.7f, 1.0f}, // Pink
            {1.0f, 0.5f, 0.0f, 1.0f}, // Orange
            {0.5f, 0.0f, 0.5f, 1.0f}, // Purple
            {0.0f, 0.0f, 1.0f, 1.0f}, // Blue
            {0.0f, 0.5f, 0.5f, 1.0f}, // Teal
            {0.0f, 0.5f, 0.0f, 1.0f}, // Green
            {0.2f, 0.8f, 0.2f, 1.0f}  // Lime Green
        };
        int armyIdx = 0;
        for (auto it = mapdef["armies"].begin(); it != mapdef["armies"].end(); ++it) {
            Army a;
            int cIdx = armyIdx % 8;
            a.Color[0] = defaultColors[cIdx][0];
            a.Color[1] = defaultColors[cIdx][1];
            a.Color[2] = defaultColors[cIdx][2];
            a.Color[3] = defaultColors[cIdx][3];
            armyIdx++;
            const auto& aj = it.value();
            if (aj.contains("faction")) a.Faction = aj["faction"];
            if (aj.contains("alloys")) a.Alloys = aj["alloys"];
            if (aj.contains("energy")) a.Energy = aj["energy"];
            
            if (aj.contains("groups") && aj["groups"].is_object()) {
                for (auto git = aj["groups"].begin(); git != aj["groups"].end(); ++git) {
                    a.Groups[git.key()] = loadGroup(git.value());
                }
            }
            outParams.Armies[it.key()] = a;
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
            
            int deducedDim = std::round(std::sqrt(size / sizeof(uint16_t)));
            if (deducedDim * deducedDim * sizeof(uint16_t) == size) {
                std::vector<uint16_t> rawData(deducedDim * deducedDim);
                if (hmFile.read(reinterpret_cast<char*>(rawData.data()), size)) {
                    log("Heightmap successfully read and injected into GeoLayers.\n");
                    GeoLayerDef gl;
                    gl.Name = "Imported Heightmap";
                    
                    NoiseLayer baseLayer;
                    baseLayer.Name = "Baked Heightmap";
                    baseLayer.UseImage = true;
                    baseLayer.ImageWidth = deducedDim;
                    baseLayer.ImageHeight = deducedDim;
                    baseLayer.ImageData.resize(rawData.size());
                    
                    for (size_t i = 0; i < rawData.size(); ++i) {
                        baseLayer.ImageData[i] = static_cast<float>(rawData[i]) / 65535.0f;
                    }
                    
                    gl.Layers.push_back(baseLayer);
                    outParams.GeoLayers.push_back(gl);
                }
            } else {
                log("Heightmap size is not a perfect square for 16-bit depth.\n");
            }
        }
    }

    // Look for Splat Maps and Heightmaps universally
    auto findFile = [](const std::string& dir, const std::string& prefix) -> std::string {
        if (!fs::exists(dir)) return "";
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().filename().string();
                if (fname.find(prefix) == 0) return entry.path().string();
            }
        }
        return "";
    };
    
    std::string texFolder = folderPath + "/Textures";
    outParams.PendingSplat14Path = findFile(texFolder, "stratums_1_4.");
    if (outParams.PendingSplat14Path.empty()) outParams.PendingSplat14Path = findFile(folderPath, "stratums_1_4.");
    
    outParams.PendingSplat58Path = findFile(texFolder, "stratums_5_8.");
    if (outParams.PendingSplat58Path.empty()) outParams.PendingSplat58Path = findFile(folderPath, "stratums_5_8.");
    
    outParams.PendingHeightmapPath = findFile(texFolder, "heightmap.");
    if (outParams.PendingHeightmapPath.empty()) outParams.PendingHeightmapPath = findFile(folderPath, "heightmap.");

    std::filesystem::path p(pathOrFolder);
    std::string mapName = p.stem().string();

    // Load explicitly placed markers (and wipe procedural rules)
    outParams.ProceduralMarkerLayers.clear();
    PlacedMarkerLayer importedSanmapLayer;
    importedSanmapLayer.Name = mapName + " Markers";
    importedSanmapLayer.Type = LayerType::Fixed;
    
    // Parse Areas and determine auto-scale
    float markerScaleFactor = 1.0f;
    if (mapdef.contains("areas") && mapdef["areas"].is_object()) {
        outParams.Areas.clear();
        for (auto it = mapdef["areas"].begin(); it != mapdef["areas"].end(); ++it) {
            MapArea a;
            a.Name = it.key();
            auto areaJson = it.value();
            if (areaJson.is_object()) {
                if (areaJson.contains("x")) a.X = areaJson["x"];
                if (areaJson.contains("y")) a.Y = areaJson["y"];
                if (areaJson.contains("width")) a.Width = areaJson["width"];
                if (areaJson.contains("length")) a.Length = areaJson["length"];
            }
            outParams.Areas.push_back(a);
            
            if ((a.Name == "PlayableArea" || a.Name == "Playable") && a.Width > 0 && a.Width != (float)outParams.MapSize) {
                markerScaleFactor = static_cast<float>(outParams.MapSize) / a.Width;
                log("Detected Playable Area mismatch (" + a.Name + ": " + std::to_string(a.Width) + 
                    ", MapSize: " + std::to_string(outParams.MapSize) + "). Applying scale factor: " + 
                    std::to_string(markerScaleFactor) + "\n");
            }
        }
    }
    
    if (mapdef.contains("markers") && mapdef["markers"].is_object()) {
        auto markers = mapdef["markers"];
        for (auto it = markers.begin(); it != markers.end(); ++it) {
            std::string markerType = it.key();
            auto typeObj = it.value();
            if (typeObj.contains("transforms") && typeObj["transforms"].is_object()) {
                auto transforms = typeObj["transforms"];
                for (auto tIt = transforms.begin(); tIt != transforms.end(); ++tIt) {
                    std::string originalKey = tIt.key();
                    std::string transformName = originalKey;
                    
                    // Keep the exact original key from the map file, as the Sanctuary Map Editor uses these string names to lookup prefabs.
                    // Modifying them causes the editor to throw NullReferenceExceptions during the 100% loading phase.
                    auto tVal = tIt.value();
                    
                    MarkerTransform mt;
                    mt.Type = markerType;
                    mt.CustomName = transformName; // The unique internal key
                    mt.IsManual = true; // Essential: prevents Generator from deleting it
                    
                    if (tVal.is_array() && tVal.size() >= 3) {
                        mt.Position[0] = static_cast<float>(tVal[0]) * markerScaleFactor;
                        mt.Position[1] = tVal[1];
                        mt.Position[2] = static_cast<float>(tVal[2]) * markerScaleFactor;
                    } else if (tVal.is_object() && tVal.contains("position") && tVal["position"].is_object()) {
                        auto pos = tVal["position"];
                        if (pos.contains("x")) mt.Position[0] = static_cast<float>(pos["x"]) * markerScaleFactor;
                        if (pos.contains("y")) mt.Position[1] = pos["y"];
                        if (pos.contains("z")) mt.Position[2] = static_cast<float>(pos["z"]) * markerScaleFactor;
                    }
                    
                    bool isGameplay = false;
                    for (const auto& kt : outParams.KnownMarkerTypes) {
                        if (markerType == kt) {
                            isGameplay = true;
                            break;
                        }
                    }
                    
                    if (isGameplay) {
                        outParams.MarkersList[transformName] = mt;
                        importedSanmapLayer.MarkerKeys.push_back(transformName);
                        // No army inference logic; rely entirely on the armies defined in the map file.
                    } else {
                        GenerationParams::PropInstance pi;
                        pi.X = mt.Position[0];
                        pi.Y = mt.Position[1];
                        pi.Z = mt.Position[2];
                        pi.TintColor = 0xFF00FF00; // Default green for props
                        outParams.StaticPropsList.push_back(pi);
                    }
                }
            }
        }
        
        // Load and scale props if they exist
        if (mapdef.contains("props") && mapdef["props"].is_array()) {
            auto propsArray = mapdef["props"];
            if (markerScaleFactor != 1.0f) {
                for (auto& propGroup : propsArray) {
                    if (propGroup.contains("transforms") && propGroup["transforms"].is_array()) {
                        for (auto& t : propGroup["transforms"]) {
                            if (t.contains("position") && t["position"].is_object()) {
                                if (t["position"].contains("x")) t["position"]["x"] = static_cast<float>(t["position"]["x"]) * markerScaleFactor;
                                if (t["position"].contains("z")) t["position"]["z"] = static_cast<float>(t["position"]["z"]) * markerScaleFactor;
                            }
                        }
                    }
                }
            }
            outParams.ImportedPropsJSON = propsArray.dump();
        } else {
            outParams.ImportedPropsJSON = "";
        }

        // Load and scale decals if they exist
        if (mapdef.contains("decals") && mapdef["decals"].is_array()) {
            auto decalsArray = mapdef["decals"];
            if (markerScaleFactor != 1.0f) {
                for (auto& decalGroup : decalsArray) {
                    if (decalGroup.contains("transforms") && decalGroup["transforms"].is_array()) {
                        for (auto& t : decalGroup["transforms"]) {
                            if (t.contains("position") && t["position"].is_object()) {
                                if (t["position"].contains("x")) t["position"]["x"] = static_cast<float>(t["position"]["x"]) * markerScaleFactor;
                                if (t["position"].contains("z")) t["position"]["z"] = static_cast<float>(t["position"]["z"]) * markerScaleFactor;
                            }
                        }
                    }
                }
            }
            outParams.ImportedDecalsJSON = decalsArray.dump();
        } else {
            outParams.ImportedDecalsJSON = "";
        }
        // Ensure every loaded Army has a Spawn marker
        for (const auto& [armyId, army] : outParams.Armies) {
            bool foundSpawn = false;
            for (const auto& [key, marker] : outParams.MarkersList) {
                if ((marker.Type == "Spawn" || marker.Type == "Spawns") && marker.CustomName.find(armyId) != std::string::npos) {
                    foundSpawn = true;
                    break;
                }
            }
            if (!foundSpawn) {
                MarkerTransform mt;
                mt.Type = "Spawn";
                mt.CustomName = "Spawn_" + armyId;
                mt.IsManual = true;
                mt.Position[0] = outParams.MapSize / 2.0f;
                mt.Position[1] = 0.0f;
                mt.Position[2] = outParams.MapSize / 2.0f;
                outParams.MarkersList[mt.CustomName] = mt;
                importedSanmapLayer.MarkerKeys.push_back(mt.CustomName);
            }
        }
        
        // Build Spatial Chunk Grid for O(1) click detection
        int chunks = outParams.SpatialGridResolution;
        outParams.MarkerSpatialGrid.assign(chunks * chunks, GenerationParams::MarkerChunk());
        
        for (const auto& [key, marker] : outParams.MarkersList) {
            float normX = marker.Position[0] / outParams.MapSize;
            float normY = marker.Position[2] / outParams.MapSize;
            int cx = std::clamp(static_cast<int>(normX * chunks), 0, chunks - 1);
            int cy = std::clamp(static_cast<int>(normY * chunks), 0, chunks - 1);
            outParams.MarkerSpatialGrid[cy * chunks + cx].MarkerKeys.push_back(key);
        }
        
        outParams.PlacedMarkerLayers.push_back(importedSanmapLayer);
    }

    log("--- End LoadSanmap (SUCCESS) ---\n");
    return true;
}

void MapImporter::LoadPendingTextures(GenerationParams& outParams, std::string& outDebugLog) {
    std::ofstream dbg("debug_importer_textures.txt", std::ios::app);
    auto log = [&](const std::string& msg) {
        dbg << msg;
        outDebugLog += msg;
    };
    log("--- Start LoadPendingTextures ---\n");

    std::string s1_4Path = outParams.PendingSplat14Path;
    std::string s5_8Path = outParams.PendingSplat58Path;
    std::string hmapPath = outParams.PendingHeightmapPath;

    // Load Splat maps
    if (!s1_4Path.empty() && fs::exists(s1_4Path)) {
        log("Found splat map 1-4: " + s1_4Path + ". Loading...\n");
        int w1, h1, c1;
        unsigned char* s14Data = stbi_load(s1_4Path.c_str(), &w1, &h1, &c1, 4);
        unsigned char* s58Data = nullptr;
        int w2 = 0, h2 = 0;

        if (!s5_8Path.empty() && fs::exists(s5_8Path)) {
            log("Found splat map 5-8: " + s5_8Path + ". Loading...\n");
            int c2;
            s58Data = stbi_load(s5_8Path.c_str(), &w2, &h2, &c2, 4);
        }

        if (s14Data) {
            int texSize = outParams.MapSize;
            int pixelCount = texSize * texSize;
            log("Resampling splat maps (" + std::to_string(w1) + "x" + std::to_string(h1) +
                ") -> ImportedMaskData (" + std::to_string(texSize) + "x" + std::to_string(texSize) + ").\n");

            for (auto& s : outParams.Stratums) {
                s.importedMaskData.assign(pixelCount, 0.0f);
                s.maskMode = ImportedMaskMode::StaticOverride; // Auto-enable on import
            }

            for (int sy = 0; sy < texSize; ++sy) {
                for (int sx = 0; sx < texSize; ++sx) {
                    int tx1 = std::clamp((sx * w1) / texSize, 0, w1 - 1);
                    int ty1 = std::clamp((sy * h1) / texSize, 0, h1 - 1);
                    int sIdx1 = (ty1 * w1 + tx1) * 4;
                    int maskIdx = sy * texSize + sx;

                    if (0 < outParams.Stratums.size()) outParams.Stratums[0].importedMaskData[maskIdx] = s14Data[sIdx1 + 0] / 255.0f;
                    if (1 < outParams.Stratums.size()) outParams.Stratums[1].importedMaskData[maskIdx] = s14Data[sIdx1 + 1] / 255.0f;
                    if (2 < outParams.Stratums.size()) outParams.Stratums[2].importedMaskData[maskIdx] = s14Data[sIdx1 + 2] / 255.0f;
                    if (3 < outParams.Stratums.size()) outParams.Stratums[3].importedMaskData[maskIdx] = s14Data[sIdx1 + 3] / 255.0f;

                    if (s58Data && w2 > 0 && h2 > 0) {
                        int tx2 = std::clamp((sx * w2) / texSize, 0, w2 - 1);
                        int ty2 = std::clamp((sy * h2) / texSize, 0, h2 - 1);
                        int sIdx2 = (ty2 * w2 + tx2) * 4;
                        if (4 < outParams.Stratums.size()) outParams.Stratums[4].importedMaskData[maskIdx] = s58Data[sIdx2 + 0] / 255.0f;
                        if (5 < outParams.Stratums.size()) outParams.Stratums[5].importedMaskData[maskIdx] = s58Data[sIdx2 + 1] / 255.0f;
                        if (6 < outParams.Stratums.size()) outParams.Stratums[6].importedMaskData[maskIdx] = s58Data[sIdx2 + 2] / 255.0f;
                        if (7 < outParams.Stratums.size()) outParams.Stratums[7].importedMaskData[maskIdx] = s58Data[sIdx2 + 3] / 255.0f;
                    }
                }
            }
            log("Stratum masks imported successfully.\n");
        }

        if (s14Data) stbi_image_free(s14Data);
        if (s58Data) stbi_image_free(s58Data);
    }
    
    // Load heightmap if provided
    if (!hmapPath.empty() && fs::exists(hmapPath)) {
        log("Loading pending heightmap: " + hmapPath + "\n");
        // We will do a generic load. If it's .raw, we load 16-bit. If it's .png, we load 16-bit.
        std::ifstream file(hmapPath, std::ios::binary | std::ios::ate);
        if (file) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            
            // Assume 16-bit single channel for raw
            int expectedDim = static_cast<int>(std::sqrt(size / 2));
            if (expectedDim > 0 && size % 2 == 0) {
                std::vector<uint16_t> buffer(size / 2);
                if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                    if (outParams.GeoLayers.empty()) {
                        outParams.GeoLayers.push_back({});
                        outParams.GeoLayers.back().Name = "Imported Heightmap";
                    }
                    
                    auto& geo = outParams.GeoLayers.back();
                    if (geo.Layers.empty()) {
                        geo.Layers.push_back({});
                    }
                    auto& layer = geo.Layers.back();
                    layer.Name = "Imported Heightmap";
                    layer.Blend = BlendMode::Add;
                    layer.UseImage = true;
                    layer.ImageWidth = outParams.MapSize;
                    layer.ImageHeight = outParams.MapSize;
                    layer.ImagePath = hmapPath;

                    int targetDim = outParams.MapSize;
                    layer.ImageData.assign(targetDim * targetDim, 0.0f);
                    
                    for (int y = 0; y < targetDim; ++y) {
                        for (int x = 0; x < targetDim; ++x) {
                            int sx = std::clamp((x * expectedDim) / targetDim, 0, expectedDim - 1);
                            int sy = std::clamp((y * expectedDim) / targetDim, 0, expectedDim - 1);
                            float val = static_cast<float>(buffer[sy * expectedDim + sx]) / 65535.0f;
                            layer.ImageData[y * targetDim + x] = val;
                        }
                    }
                    log("Heightmap imported successfully.\n");
                }
            }
        }
    }

    outParams.PendingSplat14Path = "";
    outParams.PendingSplat58Path = "";
    outParams.PendingHeightmapPath = "";
}

} // namespace SanmapGen
