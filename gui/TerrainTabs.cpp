#include "UITabs.h"
#include "widgets/Widget_LayerManager.h"
#include "imgui.h"
#include "FileDialog.h"
#include <string>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <execution>
#include <vector>

namespace SanmapGen {
namespace UI {

    // Internal helper to render the UI for a single NoiseLayer

    // Reusable Global Layer Framework

    void RenderHeightmapTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Checkbox("##showHeightmap", &params.ShowHeightmap); ImGui::SameLine();
        ImGui::Text("Global Settings");
        ImGui::Separator();

        if (ImGui::InputInt("Map Seed", &params.Seed)) bNeedsMapUpdate = true;
        
        const int sizes[] = { 256, 512, 1024, 2048, 4096 };
        const char* size_labels[] = { "256", "512", "1024", "2048", "4096" };
        
        int size_idx = 1;
        for (int i = 0; i < IM_ARRAYSIZE(sizes); ++i) {
            if (sizes[i] == params.MapSize) size_idx = i;
        }
        
        ImGui::Checkbox("Scale features to Map Size", &params.ScaleFeaturesToMapSize);
        if (ImGui::BeginCombo("Map Size", size_labels[size_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(sizes); n++) {
                if (ImGui::Selectable(size_labels[n], size_idx == n)) {
                    if (size_idx != n) {
                        int oldS = sizes[size_idx];
                        size_idx = n;
                        params.MapSize = sizes[size_idx];
                        bNeedsMapUpdate = true;
                        if (params.ScaleFeaturesToMapSize) {
                            float scale = static_cast<float>(sizes[n]) / oldS; // Corrected scaling math: new / old
                            auto scaleNoiseLayer = [&](NoiseLayer& l) {
                                l.Frequency /= scale; // Frequency gets smaller to stretch
                                
                                // Physically upscale imported heightmaps so erosion can add detail at the new resolution
                                if (l.UseImage && !l.ImageData.empty()) {
                                    int oldW = l.ImageWidth;
                                    int oldH = l.ImageHeight;
                                    int newW = static_cast<int>(oldW * scale);
                                    int newH = static_cast<int>(oldH * scale);
                                    
                                    if (newW > 0 && newH > 0 && (newW != oldW || newH != oldH)) {
                                        std::vector<float> resizedData(newW * newH);
                                        float ratioW = static_cast<float>(oldW) / newW;
                                        float ratioH = static_cast<float>(oldH) / newH;
                                        
                                        #pragma omp parallel for
                                        for (int y = 0; y < newH; ++y) {
                                            float fy = (y + 0.5f) * ratioH - 0.5f;
                                            int y0 = std::clamp(static_cast<int>(fy), 0, oldH - 1);
                                            int y1 = std::clamp(y0 + 1, 0, oldH - 1);
                                            float wy = fy - static_cast<float>(y0);
                                            if (wy < 0.0f) wy = 0.0f;
                                            
                                            for (int x = 0; x < newW; ++x) {
                                                float fx = (x + 0.5f) * ratioW - 0.5f;
                                                int x0 = std::clamp(static_cast<int>(fx), 0, oldW - 1);
                                                int x1 = std::clamp(x0 + 1, 0, oldW - 1);
                                                float wx = fx - static_cast<float>(x0);
                                                if (wx < 0.0f) wx = 0.0f;
                                                
                                                float v00 = l.ImageData[y0 * oldW + x0];
                                                float v10 = l.ImageData[y0 * oldW + x1];
                                                float v01 = l.ImageData[y1 * oldW + x0];
                                                float v11 = l.ImageData[y1 * oldW + x1];
                                                
                                                float v0 = v00 * (1.0f - wx) + v10 * wx;
                                                float v1 = v01 * (1.0f - wx) + v11 * wx;
                                                resizedData[y * newW + x] = v0 * (1.0f - wy) + v1 * wy;
                                            }
                                        }
                                        l.ImageData = std::move(resizedData);
                                        l.ImageWidth = newW;
                                        l.ImageHeight = newH;
                                    }
                                }
                            };
                            
                            for (auto& gl : params.GeoLayers) {
                                for (auto& l : gl.Layers) scaleNoiseLayer(l);
                            }
                            for (auto& l : params.DetailNormalLayers) scaleNoiseLayer(l);
                            for (auto& l : params.SmoothnessLayers) scaleNoiseLayer(l);
                            for (auto& l : params.TintLayers) scaleNoiseLayer(l);
                            for (auto& l : params.HoleLayers) scaleNoiseLayer(l);
                            
                            // Scale imported masks to maintain texel density relative to map size
                            for (auto& stratum : params.Stratums) {
                                if (!stratum.importedMaskData.empty()) {
                                    int oldMaskSize = static_cast<int>(std::sqrt(stratum.importedMaskData.size()));
                                    int newMaskSize = static_cast<int>(oldMaskSize * scale);
                                    if (newMaskSize > 0 && newMaskSize != oldMaskSize) {
                                        std::vector<float> resizedData(newMaskSize * newMaskSize);
                                        float ratio = static_cast<float>(oldMaskSize) / newMaskSize;
                                        
                                        #pragma omp parallel for
                                        for (int y = 0; y < newMaskSize; ++y) {
                                            float fy = (y + 0.5f) * ratio - 0.5f;
                                            int y0 = std::clamp(static_cast<int>(fy), 0, oldMaskSize - 1);
                                            int y1 = std::clamp(y0 + 1, 0, oldMaskSize - 1);
                                            float wy = fy - static_cast<float>(y0);
                                            if (wy < 0.0f) wy = 0.0f;
                                            
                                            for (int x = 0; x < newMaskSize; ++x) {
                                                float fx = (x + 0.5f) * ratio - 0.5f;
                                                int x0 = std::clamp(static_cast<int>(fx), 0, oldMaskSize - 1);
                                                int x1 = std::clamp(x0 + 1, 0, oldMaskSize - 1);
                                                float wx = fx - static_cast<float>(x0);
                                                if (wx < 0.0f) wx = 0.0f;
                                                
                                                float v00 = stratum.importedMaskData[y0 * oldMaskSize + x0];
                                                float v10 = stratum.importedMaskData[y0 * oldMaskSize + x1];
                                                float v01 = stratum.importedMaskData[y1 * oldMaskSize + x0];
                                                float v11 = stratum.importedMaskData[y1 * oldMaskSize + x1];
                                                
                                                float v0 = v00 * (1.0f - wx) + v10 * wx;
                                                float v1 = v01 * (1.0f - wx) + v11 * wx;
                                                resizedData[y * newMaskSize + x] = v0 * (1.0f - wy) + v1 * wy;
                                            }
                                        }
                                        stratum.importedMaskData = std::move(resizedData);
                                        stratum.previewActualMaskTex = 0; // Force texture regen
                                    }
                                }
                            }
                            
                            // Hardware-optimized SIMD scaling for manual markers
                            std::vector<MarkerTransform*> markerPtrs;
                            markerPtrs.reserve(params.MarkersList.size());
                            for (auto& [k, m] : params.MarkersList) markerPtrs.push_back(&m);
                            
                            std::for_each(std::execution::par_unseq, markerPtrs.begin(), markerPtrs.end(), [scale](MarkerTransform* m) {
                                m->Position[0] *= scale;
                                m->Position[2] *= scale;
                            });
                            
                            // Hardware-optimized SIMD scaling for static props
                            std::for_each(std::execution::par_unseq, params.StaticPropsList.begin(), params.StaticPropsList.end(), [scale](GenerationParams::PropInstance& p) {
                                p.X *= scale;
                                p.Z *= scale;
                            });
                            
                            // Rebuild Spatial Grid
                            int chunks = params.SpatialGridResolution;
                            params.MarkerSpatialGrid.assign(chunks * chunks, GenerationParams::MarkerChunk());
                            for (const auto& [key, marker] : params.MarkersList) {
                                float normX = marker.Position[0] / params.MapSize;
                                float normY = marker.Position[2] / params.MapSize;
                                int cx = std::clamp(static_cast<int>(normX * chunks), 0, chunks - 1);
                                int cy = std::clamp(static_cast<int>(normY * chunks), 0, chunks - 1);
                                params.MarkerSpatialGrid[cy * chunks + cx].MarkerKeys.push_back(key);
                            }
                        }
                    }
                }
            }
            ImGui::EndCombo();
        }
        
        if (ImGui::SliderFloat("Global Gravity", &params.GlobalGravity, 1.0f, 20.0f)) bNeedsMapUpdate = true;

        ImGui::Spacing();
        ImGui::Text("GeoLayers Hierarchy");
        ImGui::Separator();
        
        std::vector<NoiseLayer> dummy;
        Widget_LayerManager::Widget_LayerManager::RenderLayerStack(params, dummy, &params.GeoLayers, true, bNeedsMapUpdate);
    }
    
    void RenderSymmetryTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Text("Global Symmetry");
        ImGui::Separator();
        
        int current_alg = static_cast<int>(params.SymAlgorithm);
        const char* alg_names[] = { "Fold", "Blur", "Cross Fade", "Cylinder3D", "Torus3D", "Native Hash", "Superposition" };
        if (ImGui::Combo("Symmetry Algorithm", &current_alg, alg_names, IM_ARRAYSIZE(alg_names))) {
            params.SymAlgorithm = static_cast<SanmapGen::SymmetryAlgorithm>(current_alg);
            bNeedsMapUpdate = true;
        }
        
        if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Blur) {
            if (ImGui::SliderFloat("Blur Radius", &params.SymmetryBlurRadius, 1.0f, 50.0f)) bNeedsMapUpdate = true;
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::CrossFade) {
            if (ImGui::SliderFloat("Cross-Fade Width", &params.CrossFadeWidth, 0.0f, 0.5f)) bNeedsMapUpdate = true;
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Superposition) {
            int current_blend = static_cast<int>(params.SymSuperpositionBlend);
            const char* blend_names[] = { "Add", "Subtract", "Multiply", "Overlay", "Max", "Min" };
            if (ImGui::Combo("Blend Mode", &current_blend, blend_names, IM_ARRAYSIZE(blend_names))) {
                params.SymSuperpositionBlend = static_cast<SanmapGen::BlendMode>(current_blend);
                bNeedsMapUpdate = true;
            }
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Cylinder3D) {
            if (ImGui::SliderFloat("Z Scale", &params.CylinderZScale, 0.1f, 10.0f)) bNeedsMapUpdate = true;
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Torus3D) {
            if (ImGui::SliderFloat("Major Radius", &params.TorusMajorRadius, 1.0f, 20.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Minor Radius", &params.TorusMinorRadius, 0.1f, 5.0f)) bNeedsMapUpdate = true;
        }
        
        bool symPoint  = (params.GlobalSymmetryMask & Symmetry_Point);
        bool symX      = (params.GlobalSymmetryMask & Symmetry_X);
        bool symZ      = (params.GlobalSymmetryMask & Symmetry_Z);
        bool symXY     = (params.GlobalSymmetryMask & Symmetry_XY);
        bool symRadial = (params.GlobalSymmetryMask & Symmetry_Radial);
        if (ImGui::Checkbox("Point",  &symPoint))  { params.GlobalSymmetryMask ^= Symmetry_Point;  bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("X",      &symX))      { params.GlobalSymmetryMask ^= Symmetry_X;      bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("Z",      &symZ))      { params.GlobalSymmetryMask ^= Symmetry_Z;      bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("XY",     &symXY))     { params.GlobalSymmetryMask ^= Symmetry_XY;     bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("Radial", &symRadial)) { params.GlobalSymmetryMask ^= Symmetry_Radial; bNeedsMapUpdate = true; }
    }

    bool GradientEditor(const char* label, GradientSettings& gradient, float maxLocation) {
        bool changed = false;
        ImGui::PushID(label);
        
        if (ImGui::Checkbox("Smooth Interpolation", &gradient.SmoothInterpolation)) changed = true;
        
        ImGui::Spacing();
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        float height = 30.0f;
        ImVec2 p1 = ImVec2(p0.x + width, p0.y + height);
        
        // Draw the gradient bar
        if (gradient.Stops.empty()) {
            draw_list->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255));
        } else if (gradient.Stops.size() == 1) {
            auto& c = gradient.Stops[0].Color;
            draw_list->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3])));
        } else {
            // Sort a copy for drawing
            std::vector<GradientStop> drawStops = gradient.Stops;
            std::sort(drawStops.begin(), drawStops.end());
            
            for (size_t i = 0; i < drawStops.size() - 1; ++i) {
                float loc0 = std::clamp(drawStops[i].Location / maxLocation, 0.0f, 1.0f);
                float loc1 = std::clamp(drawStops[i+1].Location / maxLocation, 0.0f, 1.0f);
                
                ImVec2 rectMin(p0.x + loc0 * width, p0.y);
                ImVec2 rectMax(p0.x + loc1 * width, p1.y);
                
                auto& c0 = drawStops[i].Color;
                auto& c1 = drawStops[i+1].Color;
                ImU32 col0 = ImGui::ColorConvertFloat4ToU32(ImVec4(c0[0], c0[1], c0[2], c0[3]));
                ImU32 col1 = ImGui::ColorConvertFloat4ToU32(ImVec4(c1[0], c1[1], c1[2], c1[3]));
                
                if (gradient.SmoothInterpolation) {
                    draw_list->AddRectFilledMultiColor(rectMin, rectMax, col0, col1, col1, col0);
                } else {
                    draw_list->AddRectFilled(rectMin, rectMax, col0);
                }
            }
        }
        
        draw_list->AddRect(p0, p1, IM_COL32(255, 255, 255, 100)); // Border
        
        // Handle clicking on bar to add stop
        ImGui::InvisibleButton("GradientBar", ImVec2(width, height));
        if (ImGui::IsItemClicked()) {
            float t = (ImGui::GetIO().MousePos.x - p0.x) / width;
            float newLoc = t * maxLocation;
            GradientStop ns;
            ns.Location = newLoc;
            gradient.Stops.push_back(ns);
            changed = true;
        }
        
        static int selectedStop = -1;
        
        // Draw Stops
        for (int i = 0; i < (int)gradient.Stops.size(); ++i) {
            float loc = std::clamp(gradient.Stops[i].Location / maxLocation, 0.0f, 1.0f);
            ImVec2 center(p0.x + loc * width, p1.y + 5.0f);
            
            // Triangle bounds
            ImVec2 tp0(center.x - 5.0f, center.y + 10.0f);
            ImVec2 tp1(center.x + 5.0f, center.y + 10.0f);
            ImVec2 tp2(center.x, center.y);
            
            ImU32 outlineColor = (selectedStop == i) ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 255);
            auto& c = gradient.Stops[i].Color;
            ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3]));
            
            draw_list->AddTriangleFilled(tp0, tp1, tp2, fillColor);
            draw_list->AddTriangle(tp0, tp1, tp2, outlineColor);
            
            // Interaction
            ImGui::SetCursorScreenPos(ImVec2(tp0.x, tp2.y));
            ImGui::PushID(i);
            ImGui::InvisibleButton("Stop", ImVec2(10.0f, 10.0f));
            if (ImGui::IsItemClicked()) {
                selectedStop = i;
            }
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                selectedStop = i;
                float delta = ImGui::GetIO().MouseDelta.x;
                gradient.Stops[i].Location += (delta / width) * maxLocation;
                gradient.Stops[i].Location = std::clamp(gradient.Stops[i].Location, 0.0f, maxLocation);
                changed = true;
            }
            ImGui::PopID();
        }
        
        ImGui::SetCursorScreenPos(ImVec2(p0.x, p1.y + 20.0f));
        ImGui::Spacing();
        
        // Editor for selected stop
        if (selectedStop >= 0 && selectedStop < (int)gradient.Stops.size()) {
            ImGui::Text("Selected Stop:");
            if (ImGui::DragFloat("Location", &gradient.Stops[selectedStop].Location, 0.1f, 0.0f, maxLocation)) changed = true;
            if (ImGui::ColorEdit4("Color", gradient.Stops[selectedStop].Color)) changed = true;
            if (ImGui::Button("Delete Stop")) {
                gradient.Stops.erase(gradient.Stops.begin() + selectedStop);
                selectedStop = -1;
                changed = true;
            }
            
            // Keep array sorted internally if location changed
            if (changed) {
                std::sort(gradient.Stops.begin(), gradient.Stops.end());
            }
        }
        
        ImGui::PopID();
        return changed;
    }

    void RenderSlopeMapTab(GenerationParams& params, bool& bNeedsPreviewRender) {
        ImGui::Text("Slope Settings");
        if (ImGui::Checkbox("Show Slope Map Overlay", &params.ShowSlopeMap)) bNeedsPreviewRender = true;
        ImGui::Separator();
        
        ImGui::TextDisabled("Degrees (0 - 90)");
        if (GradientEditor("SlopeGradient", params.SlopeSettingsParams.Gradient, 90.0f)) {
            bNeedsPreviewRender = true;
        }
    }

    void RenderFlowMapTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Flow (Velocity) Settings");
        if (ImGui::Checkbox("Show Flow Map Overlay", &params.ShowFlowMap)) bNeedsPreviewRender = true;
        ImGui::Separator();
        
        ImGui::TextDisabled("Flow Simulation affects both Flow & Accumulation Maps.");
        ImGui::Spacing();
        if (ImGui::DragFloat("Precipitation Rate", &params.FlowSettingsParams.Precipitation, 0.01f, 0.0f, 10.0f)) bNeedsMapUpdate = true;
        if (ImGui::SliderInt("Iterations (Time)", &params.FlowSettingsParams.Iterations, 1, 100)) bNeedsMapUpdate = true;
        
        ImGui::Spacing();
        ImGui::Text("Advanced Flow Physics");
        if (ImGui::SliderFloat("Flow Volume Multiplier", &params.FlowSettingsParams.FlowVolumeMultiplier, 0.1f, 10.0f)) bNeedsMapUpdate = true;
        if (ImGui::SliderFloat("Stochastic Variance", &params.FlowSettingsParams.StochasticVariance, 0.0f, 1.0f)) bNeedsMapUpdate = true;
        if (ImGui::SliderFloat("Slope Adherence (Divergence)", &params.FlowSettingsParams.SlopeAdherence, 0.0f, 1.0f)) bNeedsMapUpdate = true;
        if (ImGui::SliderFloat("Flow Momentum", &params.FlowSettingsParams.FlowMomentum, 0.0f, 1.0f)) bNeedsMapUpdate = true;
        
        ImGui::Spacing();
        ImGui::Text("Execution Mode");
        if (ImGui::Checkbox("Use GPU Compute (Fast Mode)", &params.FlowSettingsParams.UseGPU)) bNeedsMapUpdate = true;
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Flow Gradient");
        if (GradientEditor("FlowGradient", params.FlowSettingsParams.Gradient, 100.0f)) {
            bNeedsPreviewRender = true;
        }
    }

    void RenderAccumulationMapTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Accumulation Map Settings");
        if (ImGui::Checkbox("Show Accumulation Overlay", &params.ShowAccumulationMap)) bNeedsPreviewRender = true;
        ImGui::Separator();
        
        ImGui::Spacing();
        ImGui::Text("Accumulation Physics");
        if (ImGui::Checkbox("Accurate Simultaneous Accumulation", &params.FlowSettingsParams.AccurateSimultaneousAccumulation)) bNeedsMapUpdate = true;
        ImGui::SameLine(); ImGui::TextDisabled("(Expensive CPU calculation)");
        
        if (ImGui::SliderFloat("Spillover Threshold", &params.FlowSettingsParams.SpilloverThreshold, 0.0f, 1.0f)) bNeedsMapUpdate = true;
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Accumulation Gradient");
        if (GradientEditor("AccGradient", params.FlowSettingsParams.Gradient, 100.0f)) {
            bNeedsPreviewRender = true;
        }
    }

} // namespace UI
} // namespace SanmapGen