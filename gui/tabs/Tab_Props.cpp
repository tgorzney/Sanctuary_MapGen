#include "../UITabs.h"
#include "../UIHelpers.h"
#include "../widgets/Widget_LayerManager.h"
#include "../widgets/VirtualListRenderer.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

    void RenderPropsTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Manually Placed Props");
        ImGui::Separator();
        ImGui::TextDisabled("These props were loaded from the map file and are fully preserved.");
        ImGui::Spacing();
        
        if (params.ManualPropLayers.empty()) {
            ImGui::TextDisabled("No manual props found in this map.");
        } else {
            ImGui::BeginChild("ManualPropsList", ImVec2(0, 300), true);
            for (int layerIdx = 0; layerIdx < (int)params.ManualPropLayers.size(); layerIdx++) {
                auto& layer = params.ManualPropLayers[layerIdx];
                
                ImGui::PushID(layerIdx);
                bool layerOpen = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                
                if (layerOpen) {
                    ImGui::Indent();
                    
                    if (ImGui::Checkbox("Use Group Color", &layer.UseGroupColor)) {
                        params.UpdateStaticPropsColors();
                        bNeedsPreviewRender = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::ColorEdit4("Group Color", layer.GroupColor, ImGuiColorEditFlags_NoInputs)) {
                        if (layer.UseGroupColor) {
                            params.UpdateStaticPropsColors();
                            bNeedsPreviewRender = true;
                        }
                    }
                    if (ImGui::SliderFloat("Layer Icon Scale", &layer.IconScale, 0.1f, 10.0f)) {
                        if (layer.UseGroupColor) {
                            params.UpdateStaticPropsColors();
                            bNeedsPreviewRender = true;
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    for (int groupIdx = 0; groupIdx < (int)layer.Groups.size(); groupIdx++) {
                        auto& group = layer.Groups[groupIdx];
                        ImGui::PushID(groupIdx);
                        
                        char headerName[256];
                        snprintf(headerName, sizeof(headerName), "%s (%zu)###group_%d", group.BlueprintPath.c_str(), group.Transforms.size(), groupIdx);
                        
                        bool groupOpen = ImGui::TreeNode(headerName);
                        
                        ImGui::SameLine();
                        if (ImGui::ColorEdit4("##TypeColor", group.Color, ImGuiColorEditFlags_NoInputs)) {
                            if (!layer.UseGroupColor) {
                                params.UpdateStaticPropsColors();
                                bNeedsPreviewRender = true;
                            }
                        }
                        
                        if (groupOpen) {
                            if (ImGui::SliderFloat("Icon Scale", &group.IconScale, 0.1f, 10.0f)) {
                                if (!layer.UseGroupColor) {
                                    params.UpdateStaticPropsColors();
                                    bNeedsPreviewRender = true;
                                }
                            }
                            
                            using TransformType = decltype(group.Transforms)::value_type;
                            UI::VirtualListRenderer<TransformType>::Render("PropsList", group.Transforms, ImGui::GetTextLineHeightWithSpacing(), 
                                [](int i, const auto& t) {
                                    ImGui::Text("  [%d] Pos: %.1f, %.1f, %.1f", i, t.Position[0], t.Position[1], t.Position[2]);
                                });
                            
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::Unindent();
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Procedural Props & Decals");
        ImGui::Separator();
        
        ImGui::TextDisabled("Props use the same procedural layering system as terrain.");
        ImGui::TextDisabled("Mask density automatically defines prop density and placement chances.");
        ImGui::Spacing();
        
        std::vector<NoiseLayer> dummy;
        Widget_LayerManager::RenderLayerStack(params, dummy, &params.GeoLayers, true, bNeedsMapUpdate, LayerType::Prop);
        
        ImGui::Separator();
        ImGui::Text("Decal Rules");
        Widget_LayerManager::RenderLayerStack(params, dummy, &params.GeoLayers, true, bNeedsMapUpdate, LayerType::Decal);
    }

} // namespace UI
} // namespace SanmapGen
