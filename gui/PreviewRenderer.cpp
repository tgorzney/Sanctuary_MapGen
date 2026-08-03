#include "PreviewRenderer.h"
#include <vector>
#include <algorithm>

namespace SanmapGen {

    GLuint PreviewRenderer::UpdatePreviewTexture(const FloatMask& heightmap, const GenerationParams& params, GLuint existingTexture) {
        int width = heightmap.GetWidth();
        int height = heightmap.GetHeight();

        // If the mask is empty, return 0
        if (width <= 0 || height <= 0) return existingTexture;

        std::vector<uint8_t> pixels(width * height * 4);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float val = heightmap.Get(x, y);
                if (val < 0.0f) val = 0.0f;
                if (val > 1.0f) val = 1.0f;
                
                // Base grayscale heightmap
                uint8_t r = static_cast<uint8_t>(val * 255.0f);
                uint8_t g = r;
                uint8_t b = r;
                
                // Real height calculation for markers
                float realHeight = val * 128.0f; // Assuming 128 max height for now
                
                // Optional Water Overlay
                if (params.ShowWater && realHeight <= params.Water.WaterLevelMax) {
                    r = static_cast<uint8_t>(r * 0.3f);
                    g = static_cast<uint8_t>(g * 0.5f);
                    b = static_cast<uint8_t>(b * 0.8f + 50);
                }
                
                // Optional Markers Overlay
                if (params.ShowMarkers) {
                    // Approximate slope calculation using 3x3 difference
                    float dx = 0.0f, dy = 0.0f;
                    if (x > 0 && x < width - 1) {
                        dx = (heightmap.Get(x + 1, y) - heightmap.Get(x - 1, y)) * 128.0f * 0.5f;
                    }
                    if (y > 0 && y < height - 1) {
                        dy = (heightmap.Get(x, y + 1) - heightmap.Get(x, y - 1)) * 128.0f * 0.5f;
                    }
                    float slopeDegrees = atan(sqrt(dx*dx + dy*dy)) * (180.0f / 3.14159265f);
                    
                    bool hasMarker = false;
                    for (const auto& rule : params.Markers) {
                        if (!rule.Enabled) continue;
                        if (slopeDegrees >= rule.MinSlope && slopeDegrees <= rule.MaxSlope &&
                            realHeight >= rule.MinHeight && realHeight <= rule.MaxHeight) {
                            
                            // Cheap deterministic "density" check using a hash
                            float hash = fmod(sin(x * 12.9898f + y * 78.233f) * 43758.5453f, 1.0f);
                            if (hash < 0.0f) hash += 1.0f;
                            
                            if (hash < rule.Density * 0.01f) {
                                hasMarker = true;
                                break;
                            }
                        }
                    }
                    
                    if (hasMarker) {
                        r = 255; g = 50; b = 50; // Red dot for marker
                    }
                }
                
                int idx = (y * width + x) * 4;
                pixels[idx + 0] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
                pixels[idx + 3] = 255;
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
