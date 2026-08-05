import os

def final_fix():
    # 1. MapExporter.cpp
    with open("core/MapExporter.cpp", "r") as f:
        content = f.read()

    # Revert layer->Name to layer.Name in the LOAD function
    # It occurs at `if (l.contains("Name")) layer->Name = l["Name"];`
    content = content.replace("layer->Name = l[\"Name\"];", "layer.Name = l[\"Name\"];")
    
    # Fix params.GeoLayers to outParams.GeoLayers.clear()
    content = content.replace("outParams.GeoLayers = params.GeoLayers;", "outParams.GeoLayers.clear();\n        outParams.GeoLayers.push_back(GeoLayerDef());\n        outParams.GeoLayers[0].Name = \"Migrated GeoLayer\";")
    
    # Add back the push_back that was removed. It should go to GeoLayers[0].Layers
    # The removal was around line 335. Let's just find the end of the loop and insert it.
    content = content.replace("if (l.contains(\"ErodeBeneath\")) layer.ErodeBeneath = l[\"ErodeBeneath\"];\n\n            \n        }", "if (l.contains(\"ErodeBeneath\")) layer.ErodeBeneath = l[\"ErodeBeneath\"];\n\n            outParams.GeoLayers[0].Layers.push_back(layer);\n        }")
    
    # Fix params.UseGPUHydraulic to outParams.UseGPUHydraulic
    content = content.replace("params.UseGPUHydraulic = e[\"UseGPU\"];", "outParams.UseGPUHydraulic = e[\"UseGPU\"];")
    
    # Also MapExporter.cpp(136): error C2228: left of '.Enabled' must have class/struct/union
    # wait, my python script did replace layer.Enabled with layer->Enabled, but maybe there were other places?
    # No, it was:
    # for (const auto& layer : params.GetFlatLayers()) {
    #     if (!layer.Enabled) continue;
    content = content.replace("if (!layer.Enabled) continue;", "if (!layer->Enabled) continue;")
    
    # And there's also layer.Type, layer.Opacity, etc. all over MapExporter.cpp in the Export function!
    # Let me just revert the loop to:
    # auto flatLayers = params.GetFlatLayers();
    # for (const auto* layerPtr : flatLayers) {
    #     const auto& layer = *layerPtr;
    # That way all the layer.Name, layer.Enabled, layer.Type will WORK WITHOUT CHANGING THEM!
    
    # Wait! If I just do this:
    # for (const auto& layer : params.GetFlatLayers())
    # I can replace it with:
    # for (const auto* layerPtr : params.GetFlatLayers()) { const auto& layer = *layerPtr;
    content = content.replace("for (const auto& layer : params.GetFlatLayers()) {", "for (const auto* layerPtr : params.GetFlatLayers()) {\n        const auto& layer = *layerPtr;")
    content = content.replace("layer->Enabled", "layer.Enabled")
    
    with open("core/MapExporter.cpp", "w") as f:
        f.write(content)
        
    print("Fixed MapExporter.cpp")


final_fix()
