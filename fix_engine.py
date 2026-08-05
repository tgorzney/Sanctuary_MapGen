import os
import re

def fix_parameters():
    path = "core/Parameters.h"
    with open(path, "r") as f:
        content = f.read()
    
    helper = """        // Helper to get a flat list of all layers across all GeoLayers in calculation order
        std::vector<const NoiseLayer*> GetFlatLayers() const {
            std::vector<const NoiseLayer*> flat;
            for (const auto& gl : GeoLayers) {
                if (!gl.Enabled) continue;
                for (const auto& l : gl.Layers) {
                    flat.push_back(&l);
                }
            }
            return flat;
        }
        std::vector<NoiseLayer*> GetFlatLayersMutable() {
            std::vector<NoiseLayer*> flat;
            for (auto& gl : GeoLayers) {
                if (!gl.Enabled) continue;
                for (auto& l : gl.Layers) {
                    flat.push_back(&l);
                }
            }
            return flat;
        }
"""
    if "GetFlatLayers" not in content:
        content = content.replace("std::vector<GeoLayerDef> GeoLayers; // Main heightmap generation", "std::vector<GeoLayerDef> GeoLayers; // Main heightmap generation\n" + helper)
        with open(path, "w") as f:
            f.write(content)
        print("Patched Parameters.h")

def patch_file(filepath):
    if not os.path.exists(filepath): return
    with open(filepath, "r") as f:
        content = f.read()

    print(f"Patching {filepath}...")
    
    if "ErosionSimulator.cpp" in filepath:
        content = content.replace("for (size_t currentLayerIdx = 0; currentLayerIdx < params.Layers.size(); ++currentLayerIdx) {", 
                                  "auto flatLayers = params.GetFlatLayers();\n    for (size_t currentLayerIdx = 0; currentLayerIdx < flatLayers.size(); ++currentLayerIdx) {")
        content = content.replace("const auto& layer = params.Layers[currentLayerIdx];", "const auto& layer = *flatLayers[currentLayerIdx];")
        content = content.replace("const auto& currentLayer = params.Layers[currentLayerIdx];", "const auto& currentLayer = *flatLayers[currentLayerIdx];")
        content = content.replace("params.Layers[i].Enabled", "flatLayers[i]->Enabled")
        content = content.replace("params.Layers[i].Erosion.DepositionMode", "flatLayers[i]->Erosion.DepositionMode")
        content = content.replace("params.Layers[prev].Enabled", "flatLayers[prev]->Enabled")
        content = content.replace("auto flatLayers = params.GetFlatLayers();\n    auto flatLayers = params.GetFlatLayers();", "auto flatLayers = params.GetFlatLayers();")
        
        content = content.replace("for (size_t i = 0; i < params.Layers.size(); ++i)", "auto flatLayers = params.GetFlatLayers();\nfor (size_t i = 0; i < flatLayers.size(); ++i)")
        content = content.replace("const auto& layer = params.Layers[i];", "const auto& layer = *flatLayers[i];")
        content = content.replace("params.Layers.size()", "flatLayers.size()")
        
    elif "TerrainGenerator.cpp" in filepath:
        content = content.replace("for (size_t i = 0; i < params.Layers.size(); ++i) {", 
                                  "auto flatLayers = params.GetFlatLayers();\n        for (size_t i = 0; i < flatLayers.size(); ++i) {")
        content = content.replace("const auto& layer = params.Layers[i];", "const auto& layer = *flatLayers[i];")
        content = content.replace("params.Layers[i].Enabled", "flatLayers[i]->Enabled")
        content = content.replace("params.Layers[prev].Enabled", "flatLayers[prev]->Enabled")
        content = content.replace("for (size_t currentLayerIdx = 0; currentLayerIdx < params.Layers.size(); ++currentLayerIdx) {", 
                                  "auto flatLayers = params.GetFlatLayers();\n        for (size_t currentLayerIdx = 0; currentLayerIdx < flatLayers.size(); ++currentLayerIdx) {")
        content = content.replace("const auto& layer = params.Layers[currentLayerIdx];", "const auto& layer = *flatLayers[currentLayerIdx];")
        
        content = content.replace("auto flatLayers = params.GetFlatLayers();\n        auto flatLayers = params.GetFlatLayers();", "auto flatLayers = params.GetFlatLayers();")
        
    elif "MapExporter.cpp" in filepath:
        content = content.replace("for (size_t i = 0; i < params.Layers.size(); ++i) {", 
                                  "auto flatLayers = params.GetFlatLayers();\n        for (size_t i = 0; i < flatLayers.size(); ++i) {")
        content = content.replace("const auto& layer = params.Layers[i];", "const auto& layer = *flatLayers[i];")
        content = content.replace("layer.Erosion.UseGPU", "params.UseGPUHydraulic")
        content = content.replace("params.Layers.size()", "flatLayers.size()")
        
    elif "TerrainCompute.cpp" in filepath:
        content = content.replace("for (size_t i = 0; i < params.Layers.size(); ++i) {", 
                                  "auto flatLayers = params.GetFlatLayers();\n    for (size_t i = 0; i < flatLayers.size(); ++i) {")
        content = content.replace("const auto& layer = params.Layers[i];", "const auto& layer = *flatLayers[i];")
        content = content.replace("params.Layers[prev].Enabled", "flatLayers[prev]->Enabled")
        content = content.replace("params.Layers.size()", "flatLayers.size()")
        
    elif "ErosionCompute.cpp" in filepath:
        content = content.replace("for (size_t currentLayerIdx = 0; currentLayerIdx < params.Layers.size(); ++currentLayerIdx) {", 
                                  "auto flatLayers = params.GetFlatLayers();\n    for (size_t currentLayerIdx = 0; currentLayerIdx < flatLayers.size(); ++currentLayerIdx) {")
        content = content.replace("const auto& layer = params.Layers[currentLayerIdx];", "const auto& layer = *flatLayers[currentLayerIdx];")
        content = content.replace("params.Layers[i].Enabled", "flatLayers[i]->Enabled")
        content = content.replace("params.Layers.size()", "flatLayers.size()")
        
    with open(filepath, "w") as f:
        f.write(content)


fix_parameters()
patch_file("core/TerrainGenerator.cpp")
patch_file("core/ErosionSimulator.cpp")
patch_file("core/TerrainCompute.cpp")
patch_file("core/ErosionCompute.cpp")
patch_file("core/MapExporter.cpp")

# Also fix the BooleanMask constructor in PlacementRules.cpp
with open("core/PlacementRules.cpp", "r") as f:
    content = f.read()
content = content.replace('BooleanMask exclusionMask(mapSize, params.Seed, params.GlobalSymmetryMask, "exclusion", false);', 
                          'BooleanMask exclusionMask(mapSize, mapSize, false);')
with open("core/PlacementRules.cpp", "w") as f:
    f.write(content)

# And fix main.cpp sorting
with open("gui/main.cpp", "r") as f:
    content = f.read()
# Remove the stable_sort
sort_code = """            // Re-order layers by StratumIndex for generation
            std::stable_sort(params.Layers.begin(), params.Layers.end(), [](const SanmapGen::NoiseLayer& a, const SanmapGen::NoiseLayer& b) {
                return a.StratumIndex < b.StratumIndex;
            });"""
content = content.replace(sort_code, "")
with open("gui/main.cpp", "w") as f:
    f.write(content)

print("Done patching.")
