#pragma once

#include "Mask2D.h"
#include "Parameters.h"
#include <GLFW/glfw3.h> // We need GLuint from OpenGL headers
#include <vector>
#include "TerrainGenerator.h"

namespace SanmapGen {

    class PreviewRenderer {
    public:
        // Converts a FloatMask into an OpenGL Texture and returns the Texture ID
        static GLuint UpdatePreviewTexture(const FloatMask& heightmap, const GenerationResult& genResult, const GenerationParams& params, GLuint existingTexture = 0);
    };

} // namespace SanmapGen
