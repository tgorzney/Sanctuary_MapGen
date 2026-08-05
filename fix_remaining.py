import os
import re

def fix_remaining():
    # ErosionCompute.cpp
    with open("core/ErosionCompute.cpp", "r") as f:
        content = f.read()
    content = content.replace("params.Layers[srcIdx]", "(*flatLayers[srcIdx])")
    content = content.replace("params.Layers[currentLayerIdx]", "(*flatLayers[currentLayerIdx])")
    with open("core/ErosionCompute.cpp", "w") as f:
        f.write(content)

    # ErosionSimulator.cpp
    with open("core/ErosionSimulator.cpp", "r") as f:
        content = f.read()
    content = content.replace("params.Layers[topLayerIdx]", "(*flatLayers[topLayerIdx])")
    content = content.replace("params.Layers[idx]", "(*flatLayers[idx])")
    with open("core/ErosionSimulator.cpp", "w") as f:
        f.write(content)

    # TerrainCompute.cpp
    with open("core/TerrainCompute.cpp", "r") as f:
        content = f.read()
    content = content.replace("params.Layers.empty()", "flatLayers.empty()")
    with open("core/TerrainCompute.cpp", "w") as f:
        f.write(content)

    # MapExporter.cpp
    with open("core/MapExporter.cpp", "r") as f:
        content = f.read()
    content = content.replace("params.Layers", "params.GetFlatLayers()")
    content = content.replace("outParams.Layers", "outParams.GeoLayers")
    with open("core/MapExporter.cpp", "w") as f:
        f.write(content)

    # TerrainGenerator.cpp
    with open("core/TerrainGenerator.cpp", "r") as f:
        content = f.read()
    content = content.replace("params.Layers[i]", "(*flatLayers[i])")
    content = content.replace("for (const auto& l : params.Layers)", "for (const auto& l : params.GetFlatLayers())")
    content = content.replace("l.SymmetryMask", "l->SymmetryMask")
    with open("core/TerrainGenerator.cpp", "w") as f:
        f.write(content)

fix_remaining()
print("Final patches applied.")
