#pragma once

#include "Mask2D.h"
#include "Parameters.h"
#include <GLFW/glfw3.h> // We need GLuint from OpenGL headers

namespace SanmapGen {

    class PreviewRenderer {
    public:
        // Converts a FloatMask into an OpenGL Texture and returns the Texture ID
        // If an existing texture ID is provided, it updates it instead of creating a new one.
        static GLuint UpdatePreviewTexture(const FloatMask& heightmap, const GenerationParams& params, GLuint existingTexture = 0);
    };

} // namespace SanmapGen
