#include "PreviewRenderer.h"
#include <vector>
#include <algorithm>

namespace SanmapGen {

    GLuint PreviewRenderer::UpdatePreviewTexture(const FloatMask& heightmap, const GenerationResult& genResult, const GenerationParams& params, GLuint existingTexture) {
        int width = heightmap.GetWidth();
        int height = heightmap.GetHeight();

        // If the mask is empty, return 0
        if (width <= 1 || height <= 1) return existingTexture;

        int quadWidth = width - 1;
        int quadHeight = height - 1;

        std::vector<uint8_t> pixels(quadWidth * quadHeight * 4);
        
        for (int y = 0; y < quadHeight; ++y) {
            for (int x = 0; x < quadWidth; ++x) {
                // Get the 4 vertices of the quad
                float v00 = heightmap.Get(x, y);
                float v10 = heightmap.Get(x + 1, y);
                float v01 = heightmap.Get(x, y + 1);
                float v11 = heightmap.Get(x + 1, y + 1);

                // Quad Mean Height
                float val = (v00 + v10 + v01 + v11) * 0.25f;
                
                if (val < 0.0f) val = 0.0f;
                if (val > 1.0f) val = 1.0f;
                
                // Base grayscale heightmap
                uint8_t r = static_cast<uint8_t>(val * 255.0f);
                uint8_t g = r;
                uint8_t b = r;
                
                // Real height calculation
                float realHeight = val * 128.0f; // Assuming 128 max height for now
                
                // ActivePreviewMode: 0 = Heightmap, 1 = Flow Map, 2 = Water, 3 = Composite
                
                // Compute slope if needed for Flow/Composite (using 4 vertices)
                float slopeDegrees = 0.0f;
                if (params.ActivePreviewMode == 1 || params.ActivePreviewMode == 3 || params.ShowMarkers) {
                    float dx = ((v10 + v11) - (v00 + v01)) * 0.5f * 128.0f;
                    float dy = ((v01 + v11) - (v00 + v10)) * 0.5f * 128.0f;
                    slopeDegrees = atan(sqrt(dx*dx + dy*dy)) * (180.0f / 3.14159265f);
                }
                
                float finalR = r / 255.0f;
                float finalG = g / 255.0f;
                float finalB = b / 255.0f;
                
                auto EvalGradientColor = [&](float val, const GradientSettings& settings, float& outR, float& outG, float& outB, float& outA) {
                    const auto& stops = settings.Stops;
                    if (stops.empty()) {
                        outR = params.FlowMapColor[0]; outG = params.FlowMapColor[1]; outB = params.FlowMapColor[2]; outA = params.FlowMapColor[3];
                        return;
                    }
                    if (val <= stops.front().Location) {
                        outR = stops.front().Color[0]; outG = stops.front().Color[1]; outB = stops.front().Color[2]; outA = stops.front().Color[3];
                        return;
                    }
                    if (val >= stops.back().Location) {
                        outR = stops.back().Color[0]; outG = stops.back().Color[1]; outB = stops.back().Color[2]; outA = stops.back().Color[3];
                        return;
                    }
                    
                    for (size_t i = 0; i < stops.size() - 1; ++i) {
                        if (val >= stops[i].Location && val <= stops[i+1].Location) {
                            if (settings.SmoothInterpolation) {
                                float range = stops[i+1].Location - stops[i].Location;
                                float t = (val - stops[i].Location) / std::max(0.001f, range);
                                outR = stops[i].Color[0] * (1.0f - t) + stops[i+1].Color[0] * t;
                                outG = stops[i].Color[1] * (1.0f - t) + stops[i+1].Color[1] * t;
                                outB = stops[i].Color[2] * (1.0f - t) + stops[i+1].Color[2] * t;
                                outA = stops[i].Color[3] * (1.0f - t) + stops[i+1].Color[3] * t;
                            } else {
                                outR = stops[i].Color[0]; outG = stops[i].Color[1]; outB = stops[i].Color[2]; outA = stops[i].Color[3];
                            }
                            return;
                        }
                    }
                    outR = stops.back().Color[0]; outG = stops.back().Color[1]; outB = stops.back().Color[2]; outA = stops.back().Color[3];
                };

                if (params.ActivePreviewMode == 1) { // Slope
                    float sR, sG, sB, sA;
                    EvalGradientColor(slopeDegrees, params.SlopeSettingsParams.Gradient, sR, sG, sB, sA);
                    finalR = finalR * (1.0f - sA) + sR * sA;
                    finalG = finalG * (1.0f - sA) + sG * sA;
                    finalB = finalB * (1.0f - sA) + sB * sA;
                } else if (params.ActivePreviewMode == 2) { // Flow (Velocity)
                    float flowVal = genResult.FlowMap.Get(x, y) * 100.0f; // Scale 0-1 to 0-100 for gradient
                    float sR, sG, sB, sA;
                    EvalGradientColor(flowVal, params.FlowSettingsParams.Gradient, sR, sG, sB, sA);
                    finalR = finalR * (1.0f - sA) + sR * sA;
                    finalG = finalG * (1.0f - sA) + sG * sA;
                    finalB = finalB * (1.0f - sA) + sB * sA;
                } else if (params.ActivePreviewMode == 3) { // Accumulation
                    float accVal = genResult.AccumulationMap.Get(x, y) * 100.0f; 
                    float sR, sG, sB, sA;
                    EvalGradientColor(accVal, params.FlowSettingsParams.Gradient, sR, sG, sB, sA);
                    finalR = finalR * (1.0f - sA) + sR * sA;
                    finalG = finalG * (1.0f - sA) + sG * sA;
                    finalB = finalB * (1.0f - sA) + sB * sA;
                } else if (params.ActivePreviewMode == 4) { // Composite
                    if (params.ShowStratums) {
                        float compR = 0.0f, compG = 0.0f, compB = 0.0f;
                        for (size_t i = 0; i < genResult.Stratums.size(); ++i) {
                            float m00 = genResult.Stratums[i].Get(x, y);
                            float m10 = genResult.Stratums[i].Get(x + 1, y);
                            float m01 = genResult.Stratums[i].Get(x, y + 1);
                            float m11 = genResult.Stratums[i].Get(x + 1, y + 1);
                            float maskVal = (m00 + m10 + m01 + m11) * 0.25f;
                            
                            float strength = val * maskVal;
                            compR += params.Stratums[i].PreviewColor[0] * strength;
                            compG += params.Stratums[i].PreviewColor[1] * strength;
                            compB += params.Stratums[i].PreviewColor[2] * strength;
                        }
                        if (compR > 0 || compG > 0 || compB > 0) {
                            finalR = std::min(compR, 1.0f);
                            finalG = std::min(compG, 1.0f);
                            finalB = std::min(compB, 1.0f);
                        }
                    }
                    
                    if (params.ShowSlopeMap) {
                        float sR, sG, sB, sA;
                        EvalGradientColor(slopeDegrees, params.SlopeSettingsParams.Gradient, sR, sG, sB, sA);
                        finalR = finalR * (1.0f - sA) + sR * sA;
                        finalG = finalG * (1.0f - sA) + sG * sA;
                        finalB = finalB * (1.0f - sA) + sB * sA;
                    }
                    
                    if (params.ShowFlowMap) {
                        float flowVal = genResult.FlowMap.Get(x, y) * 100.0f;
                        float sR, sG, sB, sA;
                        EvalGradientColor(flowVal, params.FlowSettingsParams.Gradient, sR, sG, sB, sA);
                        finalR = finalR * (1.0f - sA) + sR * sA;
                        finalG = finalG * (1.0f - sA) + sG * sA;
                        finalB = finalB * (1.0f - sA) + sB * sA;
                    }
                    
                    if (params.ShowAccumulationMap) {
                        float accVal = genResult.AccumulationMap.Get(x, y) * 100.0f;
                        float sR, sG, sB, sA;
                        EvalGradientColor(accVal, params.FlowSettingsParams.Gradient, sR, sG, sB, sA);
                        finalR = finalR * (1.0f - sA) + sR * sA;
                        finalG = finalG * (1.0f - sA) + sG * sA;
                        finalB = finalB * (1.0f - sA) + sB * sA;
                    }
                }
                
                // Water Overlay (Modes 2, 3, 4)
                if ((params.ActivePreviewMode == 2 || params.ActivePreviewMode == 3 || params.ActivePreviewMode == 4) && params.ShowWater) {
                    if (realHeight <= params.Water.WaterLevelMax) {
                        float depth = params.Water.WaterLevelMax - realHeight;
                        // Shallow vs Deep water mix
                        float deepRatio = std::clamp((depth - params.Water.DeepWaterDepthMin) / 
                                          std::max(0.1f, (params.Water.DeepWaterDepthMax - params.Water.DeepWaterDepthMin)), 0.0f, 1.0f);
                        
                        // Default water colors (could be moved to params)
                        float shallowR = 0.2f, shallowG = 0.6f, shallowB = 0.8f;
                        float deepR = 0.05f, deepG = 0.1f, deepB = 0.3f;
                        
                        float wR = shallowR * (1.0f - deepRatio) + deepR * deepRatio;
                        float wG = shallowG * (1.0f - deepRatio) + deepG * deepRatio;
                        float wB = shallowB * (1.0f - deepRatio) + deepB * deepRatio;
                        
                        // Blend water over terrain (alpha based on depth)
                        float waterAlpha = std::min(depth * 0.1f, 0.85f);
                        finalR = finalR * (1.0f - waterAlpha) + wR * waterAlpha;
                        finalG = finalG * (1.0f - waterAlpha) + wG * waterAlpha;
                        finalB = finalB * (1.0f - waterAlpha) + wB * waterAlpha;
                    }
                }
                
                r = static_cast<uint8_t>(std::clamp(finalR * 255.0f, 0.0f, 255.0f));
                g = static_cast<uint8_t>(std::clamp(finalG * 255.0f, 0.0f, 255.0f));
                b = static_cast<uint8_t>(std::clamp(finalB * 255.0f, 0.0f, 255.0f));
                
                // Optional Markers Overlay (Composite only)
                if (params.ActivePreviewMode == 3 && params.ShowMarkers) {
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
                
                // Optional Props Overlay (Composite only)
                if (params.ActivePreviewMode == 3 && params.ShowProps) {
                    bool hasProp = false;
                    for (const auto& rule : params.Props) {
                        if (!rule.Enabled) continue;
                        if (rule.AvoidWater && realHeight <= params.Water.WaterLevelMax) continue;
                        
                        if (slopeDegrees >= rule.MinSlope && slopeDegrees <= rule.MaxSlope &&
                            realHeight >= rule.MinHeight && realHeight <= rule.MaxHeight) {
                            
                            float hash = fmod(sin(x * 9.123f + y * 83.456f) * 43758.5453f, 1.0f);
                            if (hash < 0.0f) hash += 1.0f;
                            if (hash < rule.Density * 0.01f) {
                                hasProp = true;
                                break;
                            }
                        }
                    }
                    if (hasProp) {
                        r = 50; g = 255; b = 50; // Green dot for Prop
                    }
                }
                
                int idx = (y * quadWidth + x) * 4;
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, quadWidth, quadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

} // namespace SanmapGen
