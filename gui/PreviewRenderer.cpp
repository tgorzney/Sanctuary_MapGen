#include "PreviewRenderer.h"
#include <vector>
#include <algorithm>

// GL_CLAMP_TO_EDGE is OpenGL 1.2+; define as fallback in case the bundled GL header only covers 1.1
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace SanmapGen {

    GLuint PreviewRenderer::UpdatePreviewTexture(const FloatMask& heightmap, const GenerationResult& genResult, const GenerationParams& params, GLuint existingTexture, bool bGeometryChanged) {
        int width = heightmap.GetWidth();
        int height = heightmap.GetHeight();

        // If the mask is empty, return 0
        if (width <= 1 || height <= 1) return existingTexture;

        int quadWidth = width - 1;
        int quadHeight = height - 1;
        
        static std::vector<float> cachedSlopes;
        static std::vector<float> cachedHeights;
        if (bGeometryChanged || cachedSlopes.size() != static_cast<size_t>(width * height)) {
            cachedSlopes.resize(width * height);
            cachedHeights.resize(width * height);
            
            float cellSize = static_cast<float>(params.MapSize) / quadWidth;
            if (cellSize < 1.0f) cellSize = 1.0f;
            
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    float v00 = heightmap.Get(x, y);
                    float v10 = heightmap.Get(std::min(x + 1, width - 1), y);
                    float v01 = heightmap.Get(x, std::min(y + 1, height - 1));
                    float v11 = heightmap.Get(std::min(x + 1, width - 1), std::min(y + 1, height - 1));
                    
                    float dx = (((v10 + v11) - (v00 + v01)) * 0.5f * 128.0f) / cellSize;
                    float dy = (((v01 + v11) - (v00 + v10)) * 0.5f * 128.0f) / cellSize;
                    
                    int idx = y * width + x;
                    
                    if (params.SlopeSettingsParams.bUseEngineParityMath) {
                        float lenSq = dx * dx + dy * dy + 1.0f;
                        float len = std::sqrt(lenSq);
                        float dotProduct = 1.0f / len; 
                        cachedSlopes[idx] = std::acos(dotProduct) * (180.0f / 3.14159265f);
                    } else {
                        cachedSlopes[idx] = atan(sqrt(dx*dx + dy*dy)) * (180.0f / 3.14159265f);
                    }
                    
                    cachedHeights[idx] = v00 * 128.0f;
                }
            }
        }
        
        float minHeight = 0.0f;
        float maxHeight = 1.0f;
        
        if (params.AutoLevelPreview) {
            minHeight = 1e10f;
            maxHeight = -1e10f;
            for (int i = 0; i < width * height; ++i) {
                float h = heightmap.GetDataPtr()[i];
                if (h < minHeight) minHeight = h;
                if (h > maxHeight) maxHeight = h;
            }
            if (maxHeight - minHeight < 0.0001f) {
                minHeight = 0.0f;
                maxHeight = 1.0f;
            }
        }

        std::vector<uint8_t> pixels(quadWidth * quadHeight * 4);
        
        #pragma omp parallel for
        for (int y = 0; y < quadHeight; ++y) {
            for (int x = 0; x < quadWidth; ++x) {
                // Get the 4 vertices of the quad
                float v00 = heightmap.Get(x, y);
                float v10 = heightmap.Get(x + 1, y);
                float v01 = heightmap.Get(x, y + 1);
                float v11 = heightmap.Get(x + 1, y + 1);

                // Center-of-quad height: average all 4 vertices so preview pixels map
                // correctly to the quad grid (matches the two-heightmap architecture).
                float val = (v00 + v10 + v01 + v11) * 0.25f;
                
                if (params.AutoLevelPreview) {
                    val = (val - minHeight) / (maxHeight - minHeight);
                }
                
                if (val < 0.0f) val = 0.0f;
                if (val > 1.0f) val = 1.0f;
                
                // Base grayscale heightmap
                uint8_t r = static_cast<uint8_t>(val * 255.0f);
                uint8_t g = r;
                uint8_t b = r;
                
                // Fetch cached geometry values
                int idx = y * width + x;
                float realHeight = cachedHeights[idx];
                float slopeDegrees = cachedSlopes[idx];
                
                // Initialize background canvas
                float finalR = 0.0f, finalG = 0.0f, finalB = 0.0f;
                
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

                // Unified Compositor: Bottom to Top
                for (const auto& layer : params.PreviewLayers) {
                    if (layer.Blend == GenerationParams::LayerBlendMode::None) continue;
                    
                    float sR = 0.0f, sG = 0.0f, sB = 0.0f, sA = 0.0f;
                    bool hasColor = false;
                    
                    if (layer.Type == GenerationParams::PreviewLayerType::Heightmap) {
                        sR = val; sG = val; sB = val; sA = 1.0f;
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::DetailNormal) {
                        sR = 0.5f; sG = 0.5f; sB = 1.0f; sA = 1.0f; // Placeholder flat normal
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Holes) {
                        sA = 0.0f; // Placeholder, assuming no holes in generator yet
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Tint) {
                        sR = 1.0f; sG = 1.0f; sB = 1.0f; sA = 1.0f; // Placeholder white tint
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Smoothness) {
                        sR = 0.5f; sG = 0.5f; sB = 0.5f; sA = 1.0f; // Placeholder 0.5 smoothness
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Stratums) {
                        float totalMask = 0.0f;
                        for (size_t i = 0; i < genResult.MaterialMasks.size(); ++i) {
                            float m00 = genResult.MaterialMasks[i].Get(x, y);
                            float m10 = genResult.MaterialMasks[i].Get(x + 1, y);
                            float m01 = genResult.MaterialMasks[i].Get(x, y + 1);
                            float m11 = genResult.MaterialMasks[i].Get(x + 1, y + 1);
                            float maskVal = (m00 + m10 + m01 + m11) * 0.25f;
                            
                            float remapMin = params.Stratums[i].maskRemapMin[0];
                            float remapMax = params.Stratums[i].maskRemapMax[0];
                            if (remapMax - remapMin > 0.0001f) {
                                maskVal = (maskVal - remapMin) / (remapMax - remapMin);
                            }
                            maskVal = std::clamp(maskVal, 0.0f, 1.0f);
                            
                            sR += params.Stratums[i].previewColor[0] * maskVal;
                            sG += params.Stratums[i].previewColor[1] * maskVal;
                            sB += params.Stratums[i].previewColor[2] * maskVal;
                            totalMask += maskVal;
                        }

                        if (totalMask > 0.0001f) {
                            sR /= totalMask;
                            sG /= totalMask;
                            sB /= totalMask;
                            
                            if (layer.Blend == GenerationParams::LayerBlendMode::Normal) {
                                sA = 1.0f; // Solid opaque in normal mode
                            } else {
                                sA = std::min(totalMask, 1.0f);
                            }
                        } else {
                            sA = 0.0f;
                        }
                        if (sA > 0) hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Slope) {
                        EvalGradientColor(slopeDegrees, params.SlopeSettingsParams.Gradient, sR, sG, sB, sA);
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Flow) {
                        float flowVal = genResult.FlowMap.Get(x, y) * 100.0f;
                        EvalGradientColor(flowVal, params.FlowSettingsParams.Gradient, sR, sG, sB, sA);
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Accumulation) {
                        float accVal = genResult.AccumulationMap.Get(x, y) * 100.0f;
                        EvalGradientColor(accVal, params.FlowSettingsParams.Gradient, sR, sG, sB, sA);
                        hasColor = true;
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Water) {
                        if (realHeight <= params.Water.WaterLevelMax) {
                            float minH = genResult.TerrainMinHeight;
                            float maxH = params.Water.WaterLevelMax;
                            float t = 0.0f;
                            if (maxH > minH) {
                                t = (realHeight - minH) / (maxH - minH);
                                t = std::clamp(t, 0.0f, 1.0f);
                            } else {
                                t = 1.0f;
                            }
                            EvalGradientColor(t, params.Water.Gradient, sR, sG, sB, sA);
                            
                            if (layer.Blend == GenerationParams::LayerBlendMode::Normal) {
                                sA = 1.0f; // 100% opaque in normal mode to easily see the water line
                            }
                            
                            hasColor = true;
                        }
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Markers) {
                        if (params.ProceduralMarkerLayers.empty() || !params.ProceduralMarkerLayers[0].Enabled) continue;
                        for (const auto& rule : params.ProceduralMarkerLayers[0].Rules) {
                            if (!rule.Enabled) continue;
                            if (slopeDegrees >= rule.MinSlope && slopeDegrees <= rule.MaxSlope &&
                                realHeight >= rule.MinHeight && realHeight <= rule.MaxHeight) {
                                float hash = fmod(sin(x * 12.9898f + y * 78.233f) * 43758.5453f, 1.0f);
                                if (hash < 0.0f) hash += 1.0f;
                                if (hash < rule.Density * 0.01f) { 
                                    sR = 1.0f; sG = 0.2f; sB = 0.2f; sA = 1.0f; 
                                    hasColor = true; break; 
                                }
                            }
                        }
                    }
                    else if (layer.Type == GenerationParams::PreviewLayerType::Props) {
                        for (const auto& gl : params.GeoLayers) {
                            if (gl.Type != LayerType::Prop || !gl.Enabled) continue;
                            for (const auto& rule : gl.Layers) {
                                if (!rule.Enabled) continue;
                                if (rule.AvoidWater && realHeight <= params.Water.WaterLevelMax) continue;
                                if (slopeDegrees >= rule.MinSlope && slopeDegrees <= rule.MaxSlope &&
                                    realHeight >= rule.MinHeight && realHeight <= rule.MaxHeight) {
                                    float hash = fmod(sin(x * 9.123f + y * 83.456f) * 43758.5453f, 1.0f);
                                    if (hash < 0.0f) hash += 1.0f;
                                    
                                    // Use LandDensity to simulate the density for props
                                    if (hash < rule.LandDensity) { 
                                        sR = 0.2f; sG = 1.0f; sB = 0.2f; sA = 1.0f; 
                                        hasColor = true; break; 
                                    }
                                }
                            }
                            if (hasColor) break;
                        }
                    }
                    
                    if (!hasColor || sA <= 0.0f) continue;
                    
                    // TG_UE Branchless Blend Math Integration
                    float B_r = finalR, B_g = finalG, B_b = finalB;
                    float S_r = sR, S_g = sG, S_b = sB;
                    float A = sA;
                    float invA = 1.0f - A;
                    
                    if (layer.Blend == GenerationParams::LayerBlendMode::Normal) {
                        finalR = B_r * invA + S_r * A;
                        finalG = B_g * invA + S_g * A;
                        finalB = B_b * invA + S_b * A;
                    } 
                    else if (layer.Blend == GenerationParams::LayerBlendMode::Add) {
                        finalR = std::min(B_r + S_r * A, 1.0f);
                        finalG = std::min(B_g + S_g * A, 1.0f);
                        finalB = std::min(B_b + S_b * A, 1.0f);
                    }
                    else if (layer.Blend == GenerationParams::LayerBlendMode::Subtract) {
                        finalR = std::max(B_r - S_r * A, 0.0f);
                        finalG = std::max(B_g - S_g * A, 0.0f);
                        finalB = std::max(B_b - S_b * A, 0.0f);
                    }
                    else if (layer.Blend == GenerationParams::LayerBlendMode::Multiply) {
                        finalR = B_r * (S_r * A + invA);
                        finalG = B_g * (S_g * A + invA);
                        finalB = B_b * (S_b * A + invA);
                    }
                    else if (layer.Blend == GenerationParams::LayerBlendMode::Divide) {
                        finalR = std::min(B_r / std::max(S_r * A + invA, 0.001f), 1.0f);
                        finalG = std::min(B_g / std::max(S_g * A + invA, 0.001f), 1.0f);
                        finalB = std::min(B_b / std::max(S_b * A + invA, 0.001f), 1.0f);
                    }
                    else if (layer.Blend == GenerationParams::LayerBlendMode::Screen) {
                        finalR = 1.0f - (1.0f - B_r) * (1.0f - S_r * A);
                        finalG = 1.0f - (1.0f - B_g) * (1.0f - S_g * A);
                        finalB = 1.0f - (1.0f - B_b) * (1.0f - S_b * A);
                    }
                    else if (layer.Blend == GenerationParams::LayerBlendMode::Overlay) {
                        // Branchless overlay
                        float stepR = (B_r < 0.5f) ? 1.0f : 0.0f;
                        float stepG = (B_g < 0.5f) ? 1.0f : 0.0f;
                        float stepB = (B_b < 0.5f) ? 1.0f : 0.0f;
                        
                        float overR = stepR * (2.0f * B_r * S_r) + (1.0f - stepR) * (1.0f - 2.0f * (1.0f - B_r) * (1.0f - S_r));
                        float overG = stepG * (2.0f * B_g * S_g) + (1.0f - stepG) * (1.0f - 2.0f * (1.0f - B_g) * (1.0f - S_g));
                        float overB = stepB * (2.0f * B_b * S_b) + (1.0f - stepB) * (1.0f - 2.0f * (1.0f - B_b) * (1.0f - S_b));
                        
                        finalR = B_r * invA + overR * A;
                        finalG = B_g * invA + overG * A;
                        finalB = B_b * invA + overB * A;
                    }
                    else if (layer.Blend == GenerationParams::LayerBlendMode::HardLight) {
                        // Branchless hard light (same as overlay but inputs swapped)
                        float stepR = (S_r < 0.5f) ? 1.0f : 0.0f;
                        float stepG = (S_g < 0.5f) ? 1.0f : 0.0f;
                        float stepB = (S_b < 0.5f) ? 1.0f : 0.0f;
                        
                        float hardR = stepR * (2.0f * B_r * S_r) + (1.0f - stepR) * (1.0f - 2.0f * (1.0f - B_r) * (1.0f - S_r));
                        float hardG = stepG * (2.0f * B_g * S_g) + (1.0f - stepG) * (1.0f - 2.0f * (1.0f - B_g) * (1.0f - S_g));
                        float hardB = stepB * (2.0f * B_b * S_b) + (1.0f - stepB) * (1.0f - 2.0f * (1.0f - B_b) * (1.0f - S_b));
                        
                        finalR = B_r * invA + hardR * A;
                        finalG = B_g * invA + hardG * A;
                        finalB = B_b * invA + hardB * A;
                    }
                    else if (layer.Blend == GenerationParams::LayerBlendMode::SoftLight) {
                        // Branchless soft light
                        auto softBlend = [](float b, float s) {
                            float step = (s < 0.5f) ? 1.0f : 0.0f;
                            float lower = b - (1.0f - 2.0f * s) * b * (1.0f - b);
                            float upper = b + (2.0f * s - 1.0f) * (sqrt(b) - b);
                            return step * lower + (1.0f - step) * upper;
                        };
                        finalR = B_r * invA + softBlend(B_r, S_r) * A;
                        finalG = B_g * invA + softBlend(B_g, S_g) * A;
                        finalB = B_b * invA + softBlend(B_b, S_b) * A;
                    }
                }
                
                // --- FOCUS GRADIENT DEBUG OVERLAY ---
                if (params.ShowFocusGradientDebugRuleIndex >= 0 && params.ShowFocusGradientDebugRuleIndex < (int)params.ProceduralMarkerLayers[0].Rules.size()) {
                    const auto& rule = params.ProceduralMarkerLayers[0].Rules[params.ShowFocusGradientDebugRuleIndex];
                    if (rule.FocusGradient != Gradient_None) {
                        float dx = (float)(x - (width / 2));
                        float dy = (float)(y - (height / 2));
                        float dist = std::sqrt(dx*dx + dy*dy);
                        
                        float prob = 1.0f;
                        if (rule.FocusGradient == Gradient_CenterFocus) {
                            float norm = dist / rule.FocusGradientRadius;
                            if (norm > 1.0f) norm = 1.0f;
                            norm = std::pow(norm, rule.FocusGradientContrast);
                            prob = 1.0f - (norm * rule.FocusGradientStrength);
                        } else if (rule.FocusGradient == Gradient_EdgeFocus) {
                            float norm = dist / rule.FocusGradientRadius;
                            if (norm > 1.0f) norm = 1.0f;
                            norm = std::pow(norm, rule.FocusGradientContrast);
                            float baseProb = 1.0f - norm;
                            prob = 1.0f - (baseProb * rule.FocusGradientStrength);
                        } else if (rule.FocusGradient == Gradient_Torus) {
                            float norm = std::abs(dist - rule.FocusGradientRadius) / rule.FocusGradientRadius;
                            if (norm > 1.0f) norm = 1.0f;
                            norm = std::pow(norm, rule.FocusGradientContrast);
                            prob = 1.0f - (norm * rule.FocusGradientStrength);
                        }
                        
                        prob = std::clamp(prob, 0.0f, 1.0f);
                        
                        float debugAlpha = (1.0f - prob) * 0.6f;
                        if (debugAlpha > 0.0f) {
                            finalR = finalR * (1.0f - debugAlpha) + 1.0f * debugAlpha;
                            finalG = finalG * (1.0f - debugAlpha) + 0.0f * debugAlpha;
                            finalB = finalB * (1.0f - debugAlpha) + 0.0f * debugAlpha;
                        }
                    }
                }
                
                r = static_cast<uint8_t>(std::clamp(finalR * 255.0f, 0.0f, 255.0f));
                g = static_cast<uint8_t>(std::clamp(finalG * 255.0f, 0.0f, 255.0f));
                b = static_cast<uint8_t>(std::clamp(finalB * 255.0f, 0.0f, 255.0f));

                int pxIdx = (y * quadWidth + x) * 4;
                pixels[pxIdx + 0] = r;
                pixels[pxIdx + 1] = g;
                pixels[pxIdx + 2] = b;
                pixels[pxIdx + 3] = 255;
            }
        }
        
        // --- BAKE STATIC PROPS (Flattened Image Layer) ---
        // Instead of processing 100,000+ items every frame in ImGui, we bake them into the texture once!
        if (!params.StaticPropsList.empty()) {
            for (const auto& prop : params.StaticPropsList) {
                float normX = prop.X / static_cast<float>(params.MapSize);
                float normY = prop.Z / static_cast<float>(params.MapSize);
                
                int px = static_cast<int>(normX * quadWidth);
                int py = static_cast<int>(normY * quadHeight);
                
                // Draw a 3x3 pixel dot for the prop
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int cx = px + dx;
                        int cy = py + dy;
                        if (cx >= 0 && cx < quadWidth && cy >= 0 && cy < quadHeight) {
                            int idx = (cy * quadWidth + cx) * 4;
                            pixels[idx + 0] = (prop.TintColor >> 16) & 0xFF; // R
                            pixels[idx + 1] = (prop.TintColor >> 8) & 0xFF;  // G
                            pixels[idx + 2] = (prop.TintColor) & 0xFF;       // B
                        }
                    }
                }
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
        
        // Clamp to edge so the texture doesn't tile/wrap at boundaries when UVs sit near 0 or 1
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Upload pixels to GPU memory
        // Ensure no row-padding misalignment regardless of future format changes
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, quadWidth, quadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

} // namespace SanmapGen
