#include "Export_Metadata.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "export/Export_Textures.h"
#include <algorithm>
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace SanmapGen {

    void to_json(nlohmann::json& j, const SanTextureLoader& t) { j = nlohmann::json{{"path", t.path}}; }
    void from_json(const nlohmann::json& j, SanTextureLoader& t) { if (j.contains("path")) j.at("path").get_to(t.path); else if (j.is_string()) t.path = j.get<std::string>(); }
    
    void to_json(nlohmann::json& j, const SanNormalTextureLoader& t) { j = nlohmann::json{{"path", t.path}}; }
    void from_json(const nlohmann::json& j, SanNormalTextureLoader& t) { if (j.contains("path")) j.at("path").get_to(t.path); else if (j.is_string()) t.path = j.get<std::string>(); }
    
    void to_json(nlohmann::json& j, const SanMaskTextureLoader& t) { j = nlohmann::json{{"path", t.path}}; }
    void from_json(const nlohmann::json& j, SanMaskTextureLoader& t) { if (j.contains("path")) j.at("path").get_to(t.path); else if (j.is_string()) t.path = j.get<std::string>(); }
    
    void to_json(nlohmann::json& j, const SanVector2& v) { j = nlohmann::json{{"x", v.x}, {"y", v.y}}; }
    void from_json(const nlohmann::json& j, SanVector2& v) {
        if (j.is_array() && j.size() >= 2) { v.x = j[0]; v.y = j[1]; }
        else { 
            if (j.contains("x")) j.at("x").get_to(v.x); 
            if (j.contains("y")) j.at("y").get_to(v.y); 
        }
    }
    
    void to_json(nlohmann::json& j, const SanVector4& v) { j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w}}; }
    void from_json(const nlohmann::json& j, SanVector4& v) {
        if (j.is_array() && j.size() >= 4) { v.x = j[0]; v.y = j[1]; v.z = j[2]; v.w = j[3]; }
        else {
            if (j.contains("x")) j.at("x").get_to(v.x);
            if (j.contains("y")) j.at("y").get_to(v.y);
            if (j.contains("z")) j.at("z").get_to(v.z);
            if (j.contains("w")) j.at("w").get_to(v.w);
        }
    }
    
    void to_json(nlohmann::json& j, const SanColor& v) { j = nlohmann::json{{"r", v.r}, {"g", v.g}, {"b", v.b}, {"a", v.a}}; }
    void from_json(const nlohmann::json& j, SanColor& v) {
        if (j.is_array() && j.size() >= 4) { v.r = j[0]; v.g = j[1]; v.b = j[2]; v.a = j[3]; }
        else {
            if (j.contains("r")) j.at("r").get_to(v.r);
            if (j.contains("g")) j.at("g").get_to(v.g);
            if (j.contains("b")) j.at("b").get_to(v.b);
            if (j.contains("a")) j.at("a").get_to(v.a);
        }
    }
    
    void to_json(nlohmann::json& j, const StratumSettings& s) {
        j = nlohmann::json{
            {"name", s.name},
            {"albedo", s.albedo},
            {"normal", s.normal},
            {"mask", s.mask},
            {"tileSize", s.tileSize},
            {"tileSizeFar", s.tileSizeFar},
            {"tileSizeTriplanar", s.tileSizeTriplanar},
            {"tileSizeFarTriplanar", s.tileSizeFarTriplanar},
            {"normalScale", s.normalScale},
            {"normalScaleFar", s.normalScaleFar},
            {"normalFarNearBlend", s.normalFarNearBlend},
            {"heightFarNearBlend", s.heightFarNearBlend},
            {"diffuseRemap", s.diffuseRemap},
            {"farColorRemap", s.farColorRemap},
            {"previewColor", s.previewColor},
            {"maskRemapMin", s.maskRemapMin},
            {"maskRemapMax", s.maskRemapMax},
            {"maskMode", (int)s.maskMode},
            {"hardness", s.hardness},
            {"friction", s.friction},
            {"cohesion", s.cohesion},
            {"capacityMult", s.capacityMult},
            {"absorptionRate", s.absorptionRate}
        };
    }
    
    void from_json(const nlohmann::json& j, StratumSettings& s) {
        if (j.contains("name")) j.at("name").get_to(s.name); else if (j.contains("Name")) j.at("Name").get_to(s.name);
        
        if (j.contains("albedo")) j.at("albedo").get_to(s.albedo); else if (j.contains("AlbedoPath")) s.albedo.path = j.at("AlbedoPath").get<std::string>();
        if (j.contains("normal")) j.at("normal").get_to(s.normal); else if (j.contains("NormalPath")) s.normal.path = j.at("NormalPath").get<std::string>();
        if (j.contains("mask")) j.at("mask").get_to(s.mask); else if (j.contains("MaskPath")) s.mask.path = j.at("MaskPath").get<std::string>();
        
        if (j.contains("tileSize")) j.at("tileSize").get_to(s.tileSize); else if (j.contains("TileSize")) j.at("TileSize").get_to(s.tileSize);
        if (j.contains("tileSizeFar")) j.at("tileSizeFar").get_to(s.tileSizeFar); else if (j.contains("TileSizeFar")) j.at("TileSizeFar").get_to(s.tileSizeFar);
        if (j.contains("tileSizeTriplanar")) j.at("tileSizeTriplanar").get_to(s.tileSizeTriplanar); else if (j.contains("TileSizeTriplanar")) j.at("TileSizeTriplanar").get_to(s.tileSizeTriplanar);
        if (j.contains("tileSizeFarTriplanar")) j.at("tileSizeFarTriplanar").get_to(s.tileSizeFarTriplanar); else if (j.contains("TileSizeFarTriplanar")) j.at("TileSizeFarTriplanar").get_to(s.tileSizeFarTriplanar);
        
        if (j.contains("normalScale")) j.at("normalScale").get_to(s.normalScale); else if (j.contains("NormalScale")) j.at("NormalScale").get_to(s.normalScale);
        if (j.contains("normalScaleFar")) j.at("normalScaleFar").get_to(s.normalScaleFar); else if (j.contains("NormalScaleFar")) j.at("NormalScaleFar").get_to(s.normalScaleFar);
        if (j.contains("normalFarNearBlend")) j.at("normalFarNearBlend").get_to(s.normalFarNearBlend); else if (j.contains("NormalFarNearBlend")) j.at("NormalFarNearBlend").get_to(s.normalFarNearBlend);
        if (j.contains("heightFarNearBlend")) j.at("heightFarNearBlend").get_to(s.heightFarNearBlend); else if (j.contains("HeightFarNearBlend")) j.at("HeightFarNearBlend").get_to(s.heightFarNearBlend);
        
        if (j.contains("diffuseRemap")) j.at("diffuseRemap").get_to(s.diffuseRemap); else if (j.contains("DiffuseRemap")) j.at("DiffuseRemap").get_to(s.diffuseRemap);
        if (j.contains("farColorRemap")) j.at("farColorRemap").get_to(s.farColorRemap); else if (j.contains("FarColorRemap")) j.at("FarColorRemap").get_to(s.farColorRemap);
        if (j.contains("previewColor")) j.at("previewColor").get_to(s.previewColor);
        
        if (j.contains("maskRemapMin")) j.at("maskRemapMin").get_to(s.maskRemapMin); else if (j.contains("MaskRemapMin")) j.at("MaskRemapMin").get_to(s.maskRemapMin);
        if (j.contains("maskRemapMax")) j.at("maskRemapMax").get_to(s.maskRemapMax); else if (j.contains("MaskRemapMax")) j.at("MaskRemapMax").get_to(s.maskRemapMax);
        
        if (j.contains("maskMode")) s.maskMode = (ImportedMaskMode)j.at("maskMode").get<int>();
        
        if (j.contains("hardness")) j.at("hardness").get_to(s.hardness); else if (j.contains("Hardness")) j.at("Hardness").get_to(s.hardness);
        if (j.contains("friction")) j.at("friction").get_to(s.friction); else if (j.contains("Friction")) j.at("Friction").get_to(s.friction);
        if (j.contains("cohesion")) j.at("cohesion").get_to(s.cohesion); else if (j.contains("Cohesion")) j.at("Cohesion").get_to(s.cohesion);
        if (j.contains("capacityMult")) j.at("capacityMult").get_to(s.capacityMult); else if (j.contains("CapacityMult")) j.at("CapacityMult").get_to(s.capacityMult);
        if (j.contains("absorptionRate")) j.at("absorptionRate").get_to(s.absorptionRate); else if (j.contains("AbsorptionRate")) j.at("AbsorptionRate").get_to(s.absorptionRate);
    }
void MetadataExporter::ExportSanmap(const std::string& folderPath, const GenerationParams& params, const FloatMask& heightmap, const GenerationResult& genData, bool exportTextures) {
    if (!fs::exists(folderPath)) {
        fs::create_directories(folderPath);
    }
    
    // Construct the sanmap JSON matching SanMap.cs
    json mapdef;
    mapdef["fileVersion"] = 3;
    mapdef["mapVersion"] = 1;
    mapdef["name"] = "Generated Map";
    mapdef["credits"] = "Sanctuary Map Generator";
    mapdef["width"] = params.MapSize;
    mapdef["length"] = params.MapSize;
    mapdef["height"] = 128; // Standard height
    mapdef["heightmapResolution"] = params.MapSize + 1;
    
    mapdef["hasWater"] = true;
    
    mapdef["waterLevel"] = params.Water.WaterLevelMin;
    mapdef["waterDepth"] = params.Water.DeepWaterDepthMin;
    
    mapdef["waterWindSpeed"] = params.Water.WaterWindSpeed;
    mapdef["waterWindDirection"] = params.Water.WaterWindDirection;
    mapdef["waterWindShoreWavesRemap"] = params.Water.WaterWindShoreWavesRemap;
    mapdef["waterShoreDepthOffset"] = params.Water.WaterShoreDepthOffset;
    mapdef["waterShoreDepthStrength"] = params.Water.WaterShoreDepthStrength;
    mapdef["waterShoreDistanceOffset"] = params.Water.WaterShoreDistanceOffset;
    mapdef["waterShoreDistanceStrength"] = params.Water.WaterShoreDistanceStrength;
    mapdef["waterShoreGeneratorBlueprint"] = params.Water.WaveGeneratorBlueprint;

    mapdef["shader"] = "RTS/TerrainLit";
    mapdef["heightTransition"] = 0.5;
    mapdef["fadeDistance"] = 128.0;
    mapdef["fadeStartDistance"] = 1.0;

    // Atmosphere
    mapdef["sunRA"] = params.Atmosphere.SunRA;
    mapdef["sunDA"] = params.Atmosphere.SunDA;
    mapdef["sunIntensity"] = params.Atmosphere.SunIntensity;
    mapdef["sunTint"] = {
        {"r", params.Atmosphere.SunTint[0]},
        {"g", params.Atmosphere.SunTint[1]},
        {"b", params.Atmosphere.SunTint[2]},
        {"a", params.Atmosphere.SunTint[3]}
    };
    mapdef["sunTemperature"] = params.Atmosphere.SunTemperature;
    mapdef["sunAngularDiameter"] = params.Atmosphere.SunAngularDiameter;
    mapdef["sunVolumetricsMultiplier"] = params.Atmosphere.SunVolumetricsMultiplier;
    mapdef["sunVolumetricsShadowDimer"] = params.Atmosphere.SunVolumetricsShadowDimer;
    mapdef["sunPosition"] = {{"x", params.Atmosphere.SunPosition[0]}, {"y", params.Atmosphere.SunPosition[1]}, {"z", params.Atmosphere.SunPosition[2]}};
    mapdef["sunCookie"] = {{"path", params.Atmosphere.SunCookiePath}};
    mapdef["sunCookieSize"] = {{"x", params.Atmosphere.SunCookieSize[0]}, {"y", params.Atmosphere.SunCookieSize[1]}};
    
    mapdef["skylightIntensity"] = params.Atmosphere.SkylightIntensity;
    mapdef["skylightTint"] = {
        {"r", params.Atmosphere.SkylightTint[0]},
        {"g", params.Atmosphere.SkylightTint[1]},
        {"b", params.Atmosphere.SkylightTint[2]},
        {"a", params.Atmosphere.SkylightTint[3]}
    };
    mapdef["skylightTemperature"] = params.Atmosphere.SkylightTemperature;
    
    mapdef["exposure"] = params.Atmosphere.Exposure;
    mapdef["exposureCompensation"] = params.Atmosphere.ExposureCompensation;
    
    mapdef["skyboxRotation"] = params.Atmosphere.SkyboxRotation;
    switch (params.Atmosphere.SkyboxIntensityMode) {
        case SkyIntensityMode::Exposure: mapdef["skyboxIntensityMode"] = "Exposure"; break;
        case SkyIntensityMode::Lux: mapdef["skyboxIntensityMode"] = "Lux"; break;
        case SkyIntensityMode::Multiplier: mapdef["skyboxIntensityMode"] = "Multiplier"; break;
    }
    
    mapdef["skyboxExposure"] = params.Atmosphere.SkyboxExposure;
    mapdef["skyboxMultiplier"] = params.Atmosphere.SkyboxMultiplier;
    mapdef["skyboxLuxValue"] = params.Atmosphere.SkyboxLuxValue;
    
    mapdef["fogAttenuationDistance"] = params.Atmosphere.FogAttenuationDistance;
    mapdef["fogBaseHeight"] = params.Atmosphere.FogBaseHeight;
    mapdef["fogMaximumHeight"] = params.Atmosphere.FogMaximumHeight;
    mapdef["fogMaximumDistance"] = params.Atmosphere.FogMaximumDistance;
    mapdef["fogAnisotropy"] = params.Atmosphere.FogAnisotropy;
    
    mapdef["skybox"] = {{"path", params.Atmosphere.SkyboxPath}};
    
    mapdef["backgroundFogIntensity"] = params.Atmosphere.BackgroundFogIntensity;
    mapdef["backgroundFogRange"] = params.Atmosphere.BackgroundFogRange;
    mapdef["backgroundFogMinimum"] = params.Atmosphere.BackgroundFogMinimum;
    mapdef["backgroundSkyColorIntensity"] = params.Atmosphere.BackgroundSkyColorIntensity;
    mapdef["backgroundColorIntensity"] = params.Atmosphere.BackgroundColorIntensity;
    mapdef["backgroundColor"] = {{"r", params.Atmosphere.BackgroundColor[0]}, {"g", params.Atmosphere.BackgroundColor[1]}, {"b", params.Atmosphere.BackgroundColor[2]}, {"a", params.Atmosphere.BackgroundColor[3]}};
    mapdef["backgroundColorFadeoutRange"] = params.Atmosphere.BackgroundColorFadeoutRange;
    mapdef["backgroundColorFadeoutPower"] = params.Atmosphere.BackgroundColorFadeoutPower;
    
    mapdef["heightFogIntensity"] = params.Atmosphere.HeightFogIntensity;
    mapdef["heightFogRange"] = {{"x", params.Atmosphere.HeightFogRange[0]}, {"y", params.Atmosphere.HeightFogRange[1]}};
    mapdef["heightFogStart"] = params.Atmosphere.HeightFogStart;
    mapdef["heightFogEnd"] = params.Atmosphere.HeightFogEnd;
    mapdef["heightFogPower"] = params.Atmosphere.HeightFogPower;
    
    mapdef["linearFogIntensity"] = params.Atmosphere.LinearFogIntensity;
    mapdef["linearFogStart"] = params.Atmosphere.LinearFogStart;
    mapdef["linearFogEnd"] = params.Atmosphere.LinearFogEnd;
    mapdef["linearFogPower"] = params.Atmosphere.LinearFogPower;
    mapdef["linearFogCameraIntensity"] = params.Atmosphere.LinearFogCameraIntensity;
    mapdef["linearFogCameraStart"] = params.Atmosphere.LinearFogCameraStart;
    mapdef["linearFogCameraEnd"] = params.Atmosphere.LinearFogCameraEnd;
    
    mapdef["windSpeed"] = params.Atmosphere.GlobalWindSpeed;
    mapdef["windDirection"] = params.Atmosphere.GlobalWindDirection;

    // Stratum Layers
    mapdef["stratumLayers"] = params.Stratums;
    
    // Areas, armies, chains
    mapdef["areas"] = json::object();
    
    // Serialize Armies
    json armiesObj = json::object();
    for (const auto& [armyName, army] : params.Armies) {
        json armyJson = json::object();
        armyJson["faction"] = army.Faction;
        armyJson["alloys"] = army.Alloys;
        armyJson["energy"] = army.Energy;
        
        // Recursive lambda for groups
        std::function<json(const UnitGroup&)> serializeGroup;
        serializeGroup = [&](const UnitGroup& group) -> json {
            json gJson = json::object();
            
            json unitsObj = json::object();
            for (const auto& [unitName, unit] : group.Units) {
                json uJson = json::object();
                uJson["type"] = unit.Type;
                if (!unit.Tpid.empty()) uJson["tpid"] = unit.Tpid;
                uJson["position"] = {{"x", unit.Position[0]}, {"y", unit.Position[1]}, {"z", unit.Position[2]}};
                uJson["rotation"] = {{"x", unit.Rotation[0]}, {"y", unit.Rotation[1]}, {"z", unit.Rotation[2]}, {"w", unit.Rotation[3]}};
                uJson["scale"] = {{"x", unit.Scale[0]}, {"y", unit.Scale[1]}, {"z", unit.Scale[2]}};
                unitsObj[unitName] = uJson;
            }
            gJson["units"] = unitsObj;
            
            json subGroupsObj = json::object();
            for (const auto& [subGroupName, subGroup] : group.Groups) {
                subGroupsObj[subGroupName] = serializeGroup(subGroup);
            }
            gJson["groups"] = subGroupsObj;
            
            return gJson;
        };
        
        json rootGroupsObj = json::object();
        for (const auto& [groupName, group] : army.Groups) {
            rootGroupsObj[groupName] = serializeGroup(group);
        }
        armyJson["groups"] = rootGroupsObj;
        
        armiesObj[armyName] = armyJson;
    }
    
    // If no armies are defined, provide default empty armies based on spawn points
    if (armiesObj.empty() && params.SpawnPointCount > 0) {
        for (int i = 0; i < params.SpawnPointCount; i++) {
            json fallbackArmy = json::object();
            fallbackArmy["faction"] = 0;
            fallbackArmy["alloys"] = 100.0;
            fallbackArmy["energy"] = 1000.0;
            fallbackArmy["groups"] = json::object();
            armiesObj["Army_" + std::to_string(i + 1)] = fallbackArmy;
        }
    }
    
    mapdef["armies"] = armiesObj;
    
    json markersObj = json::object();
    
    // We want to group markers by Type (e.g. "Spawn", "Alloy", "Plasma")
    std::map<std::string, json> groupedTransforms;
    std::map<std::string, int> typeCounters;
    
    for (const auto& [key, marker] : params.MarkersList) {
        if (marker.IsHidden) continue;
        
        json tfObj;
        tfObj["position"] = {{"x", marker.Position[0]}, {"y", marker.Position[1]}, {"z", marker.Position[2]}};
        tfObj["rotation"] = {{"x", marker.Rotation[0]}, {"y", marker.Rotation[1]}, {"z", marker.Rotation[2]}, {"w", marker.Rotation[3]}};
        tfObj["scale"] = {{"x", marker.Scale[0]}, {"y", marker.Scale[1]}, {"z", marker.Scale[2]}};
        
        std::string transformKey = marker.CustomName;
        if (transformKey.empty()) {
            if (marker.Type == "Spawn") {
                transformKey = "ARMY_" + std::to_string(++typeCounters["Spawn"]);
            } else if (marker.Type == "Alloy") {
                transformKey = "Mex " + std::to_string(typeCounters["Alloy"]++);
            } else {
                transformKey = marker.Type + "_" + std::to_string(++typeCounters[marker.Type]);
            }
        }
        
        groupedTransforms[marker.Type][transformKey] = tfObj;
    }
    
    // Create the structured markers object
    for (const auto& [type, transforms] : groupedTransforms) {
        json typeObj = json::object();
        typeObj["resource"] = (type == "Alloy" || type == "Plasma" || type == "Hydro");
        typeObj["transforms"] = transforms;
        
        // Native expects "Alloys", "Plasmas", "Spawn"
        std::string outType = type;
        if (outType == "Alloy") outType = "Alloys";
        else if (outType == "Plasma") outType = "Plasmas";
        
        markersObj[outType] = typeObj;
    }
    
    mapdef["markers"] = markersObj;

    json propsArr = json::array();
    for (const auto& rule : params.Props) {
        if (!rule.Enabled) continue;
        json propType;
        propType["blueprintPath"] = rule.BlueprintPath;
        propType["transforms"] = json::array(); // Placeholder for actual transforms generated
        propsArr.push_back(propType);
    }
    mapdef["props"] = propsArr;

    json decalsArr = json::array();
    for (const auto& rule : params.Decals) {
        if (!rule.Enabled) continue;
        json decalType;
        decalType["blueprintPath"] = rule.BlueprintPath;
        decalType["transforms"] = json::array(); // Placeholder for actual transforms generated
        decalsArr.push_back(decalType);
    }
    mapdef["decals"] = decalsArr;
    mapdef["chains"] = json::object();

    // Determine the map name from the folder path
    std::string mapName = "mapdef";
    size_t lastSlash = folderPath.find_last_of("/\\");
    if (lastSlash != std::string::npos && lastSlash + 1 < folderPath.length()) {
        mapName = folderPath.substr(lastSlash + 1);
    }
    
    // Set map name in JSON
    mapdef["name"] = mapName;

    // Export JSON matching the folder name
    std::string filePath = folderPath + "/" + mapName + ".sanmap";
    std::ofstream out(filePath);
    out << mapdef.dump(4);
    out.close();

    if (exportTextures) {
        // Create Textures subfolder required by Native Editor
        std::string texFolder = folderPath + "/Textures";
        if (!fs::exists(texFolder)) {
            fs::create_directories(texFolder);
        }

        // Export Heightmap
        std::string hmPath = texFolder + "/heightmap.raw";
        TextureExporter::ExportHeightmap(hmPath, params, heightmap);
        
        // Export Stratums
        TextureExporter::ExportStratums(texFolder, params, genData);

        // Export Tints
        TextureExporter::ExportTints(texFolder, params);
    }
}




void MetadataExporter::SaveSettings(const std::string& filePath, const GenerationParams& params) {
    json j;
    j["PresetVersion"] = params.PresetVersion;
    j["GlobalEnvironmentPath"] = params.GlobalEnvironmentPath;
    
    j["UseGPUTerrain"] = params.UseGPUTerrain;
    j["Seed"] = params.Seed;
    j["MapSize"] = params.MapSize;
    
    j["SymAlgorithm"] = static_cast<int>(params.SymAlgorithm);
    j["SpawnPointCount"] = params.SpawnPointCount;
    
    // Save Stratums
    j["Stratums"] = params.Stratums;
    
    // Save Props & Decals
    json props = json::array();
    for (const auto& p : params.Props) {
        json pj; pj["Name"] = p.Name; pj["Enabled"] = p.Enabled; pj["BlueprintPath"] = p.BlueprintPath;
        pj["Density"] = p.Density; pj["MinSlope"] = p.MinSlope; pj["MaxSlope"] = p.MaxSlope;
        pj["MinHeight"] = p.MinHeight; pj["MaxHeight"] = p.MaxHeight;
        pj["AvoidWater"] = p.AvoidWater; pj["NearCliffs"] = p.NearCliffs;
        props.push_back(pj);
    }
    j["Props"] = props;
    
    json decals = json::array();
    for (const auto& d : params.Decals) {
        json dj; dj["Name"] = d.Name; dj["Enabled"] = d.Enabled; dj["BlueprintPath"] = d.BlueprintPath;
        dj["Density"] = d.Density; dj["MinSlope"] = d.MinSlope; dj["MaxSlope"] = d.MaxSlope;
        dj["MinHeight"] = d.MinHeight; dj["MaxHeight"] = d.MaxHeight;
        decals.push_back(dj);
    }
    j["Decals"] = decals;

    // Save Armies
    json armiesMap = json::object();
    for (const auto& [armyName, army] : params.Armies) {
        json aj = json::object();
        aj["Faction"] = army.Faction;
        aj["Alloys"] = army.Alloys;
        aj["Energy"] = army.Energy;
        
        std::function<json(const UnitGroup&)> saveGroup;
        saveGroup = [&](const UnitGroup& group) -> json {
            json g = json::object();
            json unitsObj = json::object();
            for (const auto& [uName, u] : group.Units) {
                json uj;
                uj["Type"] = u.Type;
                uj["Tpid"] = u.Tpid;
                uj["Position"] = {u.Position[0], u.Position[1], u.Position[2]};
                uj["Rotation"] = {u.Rotation[0], u.Rotation[1], u.Rotation[2], u.Rotation[3]};
                uj["Scale"] = {u.Scale[0], u.Scale[1], u.Scale[2]};
                unitsObj[uName] = uj;
            }
            g["Units"] = unitsObj;
            
            json subgroupsObj = json::object();
            for (const auto& [gName, sg] : group.Groups) {
                subgroupsObj[gName] = saveGroup(sg);
            }
            g["Groups"] = subgroupsObj;
            return g;
        };
        
        json groupsObj = json::object();
        for (const auto& [groupName, group] : army.Groups) {
            groupsObj[groupName] = saveGroup(group);
        }
        aj["Groups"] = groupsObj;
        armiesMap[armyName] = aj;
    }
    j["Armies"] = armiesMap;

    // Save Layers
    json layers = json::array();
    for (const auto* layerPtr : params.GetFlatLayers()) {
        const auto& layer = *layerPtr;
        if (!layer.Enabled) continue;
        json l;
        l["Name"] = layer.Name;
        l["Enabled"] = layer.Enabled;
        l["UseImage"] = layer.UseImage;
        l["ImagePath"] = layer.ImagePath;
        l["OriginPresetPath"] = layer.OriginPresetPath;
        l["Erodable"] = layer.Erodable;
        l["StratumIndex"] = layer.StratumIndex;
        l["Blend"] = static_cast<int>(layer.Blend);
        l["Type"] = static_cast<int>(layer.Type);
        l["Fractal"] = static_cast<int>(layer.Fractal);
        l["SymmetryMask"] = layer.SymmetryMask;
        
        l["Frequency"] = layer.Frequency;
        l["Octaves"] = layer.Octaves;
        l["Gain"] = layer.Gain;
        l["PingPongStrength"] = layer.PingPongStrength;
        l["Opacity"] = layer.Opacity;
        l["CellularJitter"] = layer.CellularJitter;
        
        l["LandDensity"] = layer.LandDensity;
        l["PlateauDensity"] = layer.PlateauDensity;
        l["MountainDensity"] = layer.MountainDensity;
        l["RampDensity"] = layer.RampDensity;

        json e;
        e["Enabled"] = layer.Erosion.Enabled;
        e["DropletCount"] = layer.Erosion.DropletCount;
        e["MaxLifetime"] = layer.Erosion.MaxLifetime;
        e["Gravity"] = layer.Erosion.Gravity;
        e["EvaporationRate"] = layer.Erosion.EvaporationRate;
        e["UseRainNoise"] = layer.Erosion.UseRainNoise;
        e["RainNoiseFreq"] = layer.Erosion.RainNoiseFreq;
        e["RainNoiseOctaves"] = layer.Erosion.RainNoiseOctaves;
        e["RainNoiseThreshold"] = layer.Erosion.RainNoiseThreshold;
        e["UseOrographicRain"] = layer.Erosion.UseOrographicRain;
        e["WindAngle"] = layer.Erosion.WindAngle;
        e["DepositionMode"] = layer.Erosion.DepositionMode;
        e["SpawnMinHeight"] = layer.Erosion.SpawnMinHeight;
        e["SpawnMaxHeight"] = layer.Erosion.SpawnMaxHeight;
        e["InitialSedimentLoad"] = layer.Erosion.InitialSedimentLoad;
        l["Erosion"] = e;
        l["ErodeBeneath"] = layer.ErodeBeneath;

        layers.push_back(l);
    }
    j["Layers"] = layers;

    std::ofstream out(filePath);
    out << j.dump(4);
    out.close();
}

bool MetadataExporter::LoadSettings(const std::string& filePath, GenerationParams& outParams) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    json j;
    in >> j;
    in.close();
    
    int version = j.value("PresetVersion", 0); // Default to V0 if not found

    if (j.contains("GlobalEnvironmentPath")) outParams.GlobalEnvironmentPath = j["GlobalEnvironmentPath"];
    if (j.contains("UseGPUTerrain")) outParams.UseGPUTerrain = j["UseGPUTerrain"];
    if (j.contains("Seed")) outParams.Seed = j["Seed"];
    if (j.contains("MapSize")) outParams.MapSize = j["MapSize"];
    
    if (j.contains("SymAlgorithm")) outParams.SymAlgorithm = static_cast<SymmetryAlgorithm>(j["SymAlgorithm"].get<int>());
    if (j.contains("SpawnPointCount")) outParams.SpawnPointCount = j["SpawnPointCount"];
    
    if (j.contains("Stratums")) {
        outParams.Stratums.clear();
        for (const auto& sj : j["Stratums"]) {
            outParams.Stratums.push_back(sj.get<StratumSettings>());
        }
    } else if (version == 0) {
        // Migration: If no stratums were saved, outParams.Stratums already has the 9 default stratums.
    }
    
    if (j.contains("Props")) {
        outParams.Props.clear();
        for (const auto& pj : j["Props"]) {
            PropRule p;
            if (pj.contains("Name")) p.Name = pj["Name"];
            if (pj.contains("Enabled")) p.Enabled = pj["Enabled"];
            if (pj.contains("BlueprintPath")) p.BlueprintPath = pj["BlueprintPath"];
            if (pj.contains("Density")) p.Density = pj["Density"];
            if (pj.contains("MinSlope")) p.MinSlope = pj["MinSlope"];
            if (pj.contains("MaxSlope")) p.MaxSlope = pj["MaxSlope"];
            if (pj.contains("MinHeight")) p.MinHeight = pj["MinHeight"];
            if (pj.contains("MaxHeight")) p.MaxHeight = pj["MaxHeight"];
            if (pj.contains("AvoidWater")) p.AvoidWater = pj["AvoidWater"];
            if (pj.contains("NearCliffs")) p.NearCliffs = pj["NearCliffs"];
            outParams.Props.push_back(p);
        }
    }
    
    if (j.contains("Decals")) {
        outParams.Decals.clear();
        for (const auto& dj : j["Decals"]) {
            DecalRule d;
            if (dj.contains("Name")) d.Name = dj["Name"];
            if (dj.contains("Enabled")) d.Enabled = dj["Enabled"];
            if (dj.contains("BlueprintPath")) d.BlueprintPath = dj["BlueprintPath"];
            if (dj.contains("Density")) d.Density = dj["Density"];
            if (dj.contains("MinSlope")) d.MinSlope = dj["MinSlope"];
            if (dj.contains("MaxSlope")) d.MaxSlope = dj["MaxSlope"];
            if (dj.contains("MinHeight")) d.MinHeight = dj["MinHeight"];
            if (dj.contains("MaxHeight")) d.MaxHeight = dj["MaxHeight"];
            outParams.Decals.push_back(d);
        }
    }
    
    if (j.contains("Armies")) {
        outParams.Armies.clear();
        std::function<UnitGroup(const json&)> loadGroup;
        loadGroup = [&](const json& gj) -> UnitGroup {
            UnitGroup g;
            if (gj.contains("Units") && gj["Units"].is_object()) {
                for (auto it = gj["Units"].begin(); it != gj["Units"].end(); ++it) {
                    const auto& uj = it.value();
                    UnitTransform u;
                    if (uj.contains("Type")) u.Type = uj["Type"];
                    if (uj.contains("Tpid")) u.Tpid = uj["Tpid"];
                    if (uj.contains("Position")) { u.Position[0] = uj["Position"][0]; u.Position[1] = uj["Position"][1]; u.Position[2] = uj["Position"][2]; }
                    if (uj.contains("Rotation")) { u.Rotation[0] = uj["Rotation"][0]; u.Rotation[1] = uj["Rotation"][1]; u.Rotation[2] = uj["Rotation"][2]; u.Rotation[3] = uj["Rotation"][3]; }
                    if (uj.contains("Scale")) { u.Scale[0] = uj["Scale"][0]; u.Scale[1] = uj["Scale"][1]; u.Scale[2] = uj["Scale"][2]; }
                    g.Units[it.key()] = u;
                }
            }
            if (gj.contains("Groups") && gj["Groups"].is_object()) {
                for (auto it = gj["Groups"].begin(); it != gj["Groups"].end(); ++it) {
                    g.Groups[it.key()] = loadGroup(it.value());
                }
            }
            return g;
        };

        for (auto it = j["Armies"].begin(); it != j["Armies"].end(); ++it) {
            Army a;
            const auto& aj = it.value();
            if (aj.contains("Faction")) a.Faction = aj["Faction"];
            if (aj.contains("Alloys")) a.Alloys = aj["Alloys"];
            if (aj.contains("Energy")) a.Energy = aj["Energy"];
            if (aj.contains("Color") && aj["Color"].is_array() && aj["Color"].size() == 4) {
                a.Color[0] = aj["Color"][0];
                a.Color[1] = aj["Color"][1];
                a.Color[2] = aj["Color"][2];
                a.Color[3] = aj["Color"][3];
            } else {
                // Assign a default based on some order if no color is found
                static const float defaultColors[8][4] = {
                    {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.4f, 0.7f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f}, {0.5f, 0.0f, 0.5f, 1.0f},
                    {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.5f, 0.0f, 1.0f}, {0.2f, 0.8f, 0.2f, 1.0f}
                };
                static int loadedArmyIdx = 0;
                int cIdx = loadedArmyIdx % 8;
                a.Color[0] = defaultColors[cIdx][0];
                a.Color[1] = defaultColors[cIdx][1];
                a.Color[2] = defaultColors[cIdx][2];
                a.Color[3] = defaultColors[cIdx][3];
                loadedArmyIdx++;
            }
            
            if (aj.contains("Groups") && aj["Groups"].is_object()) {
                for (auto git = aj["Groups"].begin(); git != aj["Groups"].end(); ++git) {
                    a.Groups[git.key()] = loadGroup(git.value());
                }
            }
            outParams.Armies[it.key()] = a;
        }
    }

    if (j.contains("Layers")) {
        outParams.GeoLayers.clear();
        outParams.GeoLayers.push_back(GeoLayerDef());
        outParams.GeoLayers[0].Name = "Migrated GeoLayer";
        for (const auto& l : j["Layers"]) {
            NoiseLayer layer;
            if (l.contains("Name")) layer.Name = l["Name"];
            if (l.contains("Enabled")) layer.Enabled = l["Enabled"];
            if (l.contains("UseImage")) layer.UseImage = l["UseImage"];
            if (l.contains("ImagePath")) layer.ImagePath = l["ImagePath"];
            if (l.contains("OriginPresetPath")) layer.OriginPresetPath = l["OriginPresetPath"];
            if (l.contains("Erodable")) layer.Erodable = l["Erodable"];
            
            // VERSION MIGRATION for Stratum Type -> Stratum Index
            if (version == 0 && l.contains("Stratum")) {
                int oldEnum = l["Stratum"].get<int>();
                // old mapping: 0=Bedrock, 1=Sand, 2=Silt, 3=Clay, 4=Loam, 5=Snow
                switch(oldEnum) {
                    case 0: layer.StratumIndex = 0; break;
                    case 1: layer.StratumIndex = 2; break; // Map sand to stratum 2
                    case 2: layer.StratumIndex = 3; break;
                    case 3: layer.StratumIndex = 4; break;
                    case 4: layer.StratumIndex = 5; break;
                    case 5: layer.StratumIndex = 8; break; // Snow to top
                    default: layer.StratumIndex = 1; break;
                }
                
                // In V4, physics moved to Layer, so no need to map legacy V0 V1 physics to Stratums anymore.
                // We will attempt to load V1 physics from Stratum to Layer below if it's missing in Layer.
                
            } else if (l.contains("StratumIndex")) {
                layer.StratumIndex = l["StratumIndex"];
            }
            
            if (l.contains("Blend")) layer.Blend = static_cast<BlendMode>(l["Blend"].get<int>());
            if (l.contains("Type")) layer.Type = static_cast<NoiseType>(l["Type"].get<int>());
            if (l.contains("Fractal")) layer.Fractal = static_cast<FractalType>(l["Fractal"].get<int>());
            if (l.contains("SymmetryMask")) layer.SymmetryMask = l["SymmetryMask"];
            
            if (l.contains("Frequency")) layer.Frequency = l["Frequency"];
            if (l.contains("Octaves")) layer.Octaves = l["Octaves"];
            if (l.contains("Gain")) layer.Gain = l["Gain"];
            if (l.contains("PingPongStrength")) layer.PingPongStrength = l["PingPongStrength"];
            if (l.contains("Opacity")) layer.Opacity = l["Opacity"];
            if (l.contains("CellularJitter")) layer.CellularJitter = l["CellularJitter"];
            
            if (l.contains("LandDensity")) layer.LandDensity = l["LandDensity"];
            if (l.contains("PlateauDensity")) layer.PlateauDensity = l["PlateauDensity"];
            if (l.contains("MountainDensity")) layer.MountainDensity = l["MountainDensity"];
            if (l.contains("RampDensity")) layer.RampDensity = l["RampDensity"];

            if (l.contains("Erosion")) {
                const auto& e = l["Erosion"];
                if (e.contains("Enabled")) layer.Erosion.Enabled = e["Enabled"];
                        if (e.contains("DropletCount")) layer.Erosion.DropletCount = e["DropletCount"];
                if (e.contains("MaxLifetime")) layer.Erosion.MaxLifetime = e["MaxLifetime"];
                if (e.contains("Gravity")) layer.Erosion.Gravity = e["Gravity"];
                if (e.contains("EvaporationRate")) layer.Erosion.EvaporationRate = e["EvaporationRate"];
                if (e.contains("UseRainNoise")) layer.Erosion.UseRainNoise = e["UseRainNoise"];
                if (e.contains("RainNoiseFreq")) layer.Erosion.RainNoiseFreq = e["RainNoiseFreq"];
                if (e.contains("RainNoiseOctaves")) layer.Erosion.RainNoiseOctaves = e["RainNoiseOctaves"];
                if (e.contains("RainNoiseThreshold")) layer.Erosion.RainNoiseThreshold = e["RainNoiseThreshold"];
                if (e.contains("UseOrographicRain")) layer.Erosion.UseOrographicRain = e["UseOrographicRain"];
                if (e.contains("WindAngle")) layer.Erosion.WindAngle = e["WindAngle"];
                if (e.contains("DepositionMode")) layer.Erosion.DepositionMode = e["DepositionMode"];
                if (e.contains("SpawnMinHeight")) layer.Erosion.SpawnMinHeight = e["SpawnMinHeight"];
                if (e.contains("SpawnMaxHeight")) layer.Erosion.SpawnMaxHeight = e["SpawnMaxHeight"];
                if (e.contains("InitialSedimentLoad")) layer.Erosion.InitialSedimentLoad = e["InitialSedimentLoad"];
            }
            if (l.contains("ErodeBeneath")) layer.ErodeBeneath = l["ErodeBeneath"];

            outParams.GeoLayers[0].Layers.push_back(layer);
        }
    }
    
    // Make sure we have exactly 9 stratums
    while (outParams.Stratums.size() < 9) {
        StratumSettings s;
        s.name = "Stratum " + std::to_string(outParams.Stratums.size());
        outParams.Stratums.push_back(s);
    }
    
    return true;
}

} // namespace SanmapGen

