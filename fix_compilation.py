import os
import re

def comprehensive_fix():
    # ErosionSimulator.cpp
    with open("core/ErosionSimulator.cpp", "r") as f:
        content = f.read()
    
    # Add auto flatLayers to function
    func_start = "void ErosionSimulator::SimulateStratifiedErosionDelta(std::vector<FloatMask>& stratums, const std::vector<DropletSpawn>& spawns, const ErosionSettings& settings, const GenerationParams& params, int mapSize, int currentLayerIdx) {"
    if func_start in content:
        content = content.replace(func_start, func_start + "\n        auto flatLayers = params.GetFlatLayers();")
    
    with open("core/ErosionSimulator.cpp", "w") as f:
        f.write(content)

    # TerrainGenerator.cpp
    with open("core/TerrainGenerator.cpp", "r") as f:
        content = f.read()
    
    # Remove duplicate flatLayers around 552
    content = content.replace("auto flatLayers = params.GetFlatLayers();\n        auto flatLayers = params.GetFlatLayers();", "auto flatLayers = params.GetFlatLayers();")
    content = content.replace("auto flatLayers = params.GetFlatLayers();\nauto flatLayers = params.GetFlatLayers();", "auto flatLayers = params.GetFlatLayers();")
    
    # The [ error: 'flatLayers[currentLayerIdx]' returns a pointer, so we must dereference it properly
    # But wait, my script did const auto& layer = *flatLayers[currentLayerIdx]; which is fine.
    # The reason for the error C2088: built-in operator '[' cannot be applied is because of the redefinition of flatLayers to an error type!
    # If we remove the redefinition, it fixes all [ errors!
    content = content.replace("auto flatLayers = params.GetFlatLayers();\n        for (size_t currentLayerIdx", "for (size_t currentLayerIdx")
    content = content.replace("auto flatLayers = params.GetFlatLayers();\n    for (size_t currentLayerIdx", "for (size_t currentLayerIdx")
    
    # Wait, if we remove it, we need to make sure it IS defined at the top of GenerateMap!
    # GenerateMap already has auto flatLayers = params.GetFlatLayers(); at line 460!
    # Let's just find and replace all redundant auto flatLayers = params.GetFlatLayers(); EXCEPT the first one.
    
    parts = content.split("auto flatLayers = params.GetFlatLayers();")
    new_content = parts[0]
    for i in range(1, len(parts)):
        if i == 1:
            new_content += "auto flatLayers = params.GetFlatLayers();" + parts[i]
        else:
            new_content += parts[i]
            
    content = new_content
    
    # TerrainGenerator.cpp 709 missing argument!
    # The function signature was 6 args, but I did a string replace that somehow nuked it?
    # No, SimulateStratifiedErosionDelta DOES take 6 arguments. The issue was that layer.Erosion evaluated to int because of initialization failure. 
    # But wait, flatLayers[currentLayerIdx] -> (*flatLayers[currentLayerIdx])
    content = content.replace("layer.Erosion", "flatLayers[currentLayerIdx]->Erosion")
    content = content.replace("flatLayers[currentLayerIdx]->Erosion.UseGPU", "params.UseGPUHydraulic")
    
    with open("core/TerrainGenerator.cpp", "w") as f:
        f.write(content)

    # TerrainCompute.cpp
    with open("core/TerrainCompute.cpp", "r") as f:
        content = f.read()
    func_start = "void TerrainCompute::DispatchTerrain(std::vector<FloatMask>& stratums, const GenerationParams& params) {"
    if func_start in content:
        content = content.replace(func_start, func_start + "\n        auto flatLayers = params.GetFlatLayers();")
    with open("core/TerrainCompute.cpp", "w") as f:
        f.write(content)

    # ErosionCompute.cpp
    with open("core/ErosionCompute.cpp", "r") as f:
        content = f.read()
    func_start = "void ErosionCompute::DispatchStratified(std::vector<FloatMask>& stratums, const std::vector<DropletSpawn>& spawns, const ErosionSettings& settings, const GenerationParams& params, int mapSize, int currentLayerIdx) {"
    if func_start in content:
        content = content.replace(func_start, func_start + "\n        auto flatLayers = params.GetFlatLayers();")
    # Also in ErosionCompute.cpp, some glUniform1i calls:
    content = content.replace("PFNGLUNIFORM1IPROC", "glUniform1i")
    content = content.replace("flatLayers[currentLayerIdx]->ErodeBeneath", "(*flatLayers[currentLayerIdx]).ErodeBeneath")
    with open("core/ErosionCompute.cpp", "w") as f:
        f.write(content)

    # MapExporter.cpp
    with open("core/MapExporter.cpp", "r") as f:
        content = f.read()
    content = content.replace("layer.Name", "layer->Name")
    
    # fix outParams.GeoLayers.push_back(layer);
    # Actually we just want outParams.GeoLayers = params.GeoLayers;
    # But the old code pushed each layer. Let's just do outParams.GeoLayers = params.GeoLayers;
    content = content.replace("outParams.GeoLayers.push_back(layer);", "")
    content = content.replace("outParams.GeoLayers.clear();", "outParams.GeoLayers = params.GeoLayers;")
    
    with open("core/MapExporter.cpp", "w") as f:
        f.write(content)

comprehensive_fix()
print("Comprehensive fix applied.")
