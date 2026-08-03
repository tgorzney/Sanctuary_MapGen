#include "PreviewRenderer.h"
#include <vector>
#include <algorithm>

namespace SanmapGen {

    GLuint PreviewRenderer::UpdatePreviewTexture(const FloatMask& heightmap, GLuint existingTexture) {
        int width = heightmap.GetWidth();
        int height = heightmap.GetHeight();

        // If the mask is empty, return 0
        if (width <= 0 || height <= 0) return existingTexture;

        // Convert the float mask (0.0 to 1.0) to an RGBA pixel buffer (0 to 255)
        std::vector<uint8_t> pixels(width * height * 4);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float val = heightmap.Get(x, y);
                // Clamp to [0, 1] just in case
                if (val < 0.0f) val = 0.0f;
                if (val > 1.0f) val = 1.0f;
                
                uint8_t c = static_cast<uint8_t>(val * 255.0f);
                int idx = (y * width + x) * 4;
                pixels[idx + 0] = c;     // R
                pixels[idx + 1] = c;     // G
                pixels[idx + 2] = c;     // B
                pixels[idx + 3] = 255;   // A
            }
        }

        GLuint textureID = existingTexture;
        if (textureID == 0) {
            // Generate a new OpenGL texture
            glGenTextures(1, &textureID);
        }

        // Upload the pixel buffer to the GPU
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // This is necessary if texture doesn't wrap correctly
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Upload pixels to GPU memory
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

} // namespace SanmapGen
