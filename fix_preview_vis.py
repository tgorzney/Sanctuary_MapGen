import os

with open("gui/PreviewRenderer.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Make procedural markers respect layer visibility
old_loop = """                    else if (layer.Type == GenerationParams::PreviewLayerType::Markers) {
                        for (const auto& rule : params.ProceduralMarkerLayers[0].Rules) {"""

new_loop = """                    else if (layer.Type == GenerationParams::PreviewLayerType::Markers) {
                        if (params.ProceduralMarkerLayers.empty() || !params.ProceduralMarkerLayers[0].Enabled) continue;
                        for (const auto& rule : params.ProceduralMarkerLayers[0].Rules) {"""

content = content.replace(old_loop, new_loop)

with open("gui/PreviewRenderer.cpp", "w", encoding="utf-8") as f:
    f.write(content)
