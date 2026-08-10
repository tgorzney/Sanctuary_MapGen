#include "../UITabs.h"
#include "../UIHelpers.h"
#include "../widgets/Widget_LayerManager.h"
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
                    
                    ImGui::Spacing();
                    
                    for (int groupIdx = 0; groupIdx < (int)layer.Groups.size(); groupIdx++) {
                        auto& group = layer.Groups[groupIdx];
                        ImGui::PushID(groupIdx);
                        
                        std::string headerName = group.BlueprintPath + " (" + std::to_string(group.Transforms.size()) + ")";
                        bool groupOpen = ImGui::TreeNode(headerName.c_str());
                        
                        ImGui::SameLine();
                        if (ImGui::ColorEdit4("##TypeColor", group.Color, ImGuiColorEditFlags_NoInputs)) {
                            if (!layer.UseGroupColor) {
                                params.UpdateStaticPropsColors();
                                bNeedsPreviewRender = true;
                            }
                        }
                        
                        if (groupOpen) {
                            ImGuiListClipper clipper;
                            clipper.Begin((int)group.Transforms.size());
                            while (clipper.Step()) {
                                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                                    const auto& t = group.Transforms[i];
                                    ImGui::Text("  [%d] Pos: %.1f, %.1f, %.1f", i, t.Position[0], t.Position[1], t.Position[2]);
                                }
                            }
                            clipper.End();
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
        
        // Reclaim density moved to markers tab
}


} // namespace UI
} // namespace SanmapGen
