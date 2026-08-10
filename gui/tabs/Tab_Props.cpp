#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

    void RenderPropsTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Props & Decals");
        ImGui::Separator();
        
        if (ImGui::Button("Add Prop Rule", ImVec2(-1, 30))) {
            PropRule rule;
            rule.Name = "Prop " + std::to_string(params.Props.size());
            params.Props.push_back(rule);
            bNeedsPreviewRender = true;
        }
        ImGui::Spacing();
        
        for (int i = 0; i < (int)params.Props.size(); ++i) {
            ImGui::PushID(i + 1000);
            char label[64]; snprintf(label, sizeof(label), "Prop %d - %s", i, params.Props[i].Name.c_str());
            if (ImGui::CollapsingHeader(label)) {
                if (ImGui::Checkbox("Enabled", &params.Props[i].Enabled)) bNeedsPreviewRender = true;
                
                char nameBuf[128]; strncpy(nameBuf, params.Props[i].Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Props[i].Name = nameBuf;
                
                char bpBuf[256]; strncpy(bpBuf, params.Props[i].BlueprintPath.c_str(), sizeof(bpBuf));
                if (ImGui::InputText("Blueprint", bpBuf, IM_ARRAYSIZE(bpBuf))) params.Props[i].BlueprintPath = bpBuf;
                
                if (ImGui::SliderFloat("Density", &params.Props[i].Density, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                
                if (UI::RangeSliderFloat("Slope Range", &params.Props[i].MinSlope, &params.Props[i].MaxSlope, 0.0f, 90.0f)) {
                    bNeedsPreviewRender = true;
                }
                
                if (UI::RangeSliderFloat("Height Range", &params.Props[i].MinHeight, &params.Props[i].MaxHeight, params.TerrainMinHeight, params.TerrainMaxHeight)) {
                    bNeedsPreviewRender = true;
                }
                
                if (ImGui::Checkbox("Avoid Water", &params.Props[i].AvoidWater)) bNeedsPreviewRender = true;
                if (ImGui::Checkbox("Near Cliffs", &params.Props[i].NearCliffs)) bNeedsPreviewRender = true;
                
                if (ImGui::Button("Delete Rule", ImVec2(-1, 20))) {
                    params.Props.erase(params.Props.begin() + i);
                    bNeedsPreviewRender = true;
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Add Decal Rule", ImVec2(-1, 30))) {
            SanmapGen::DecalRule rule;
            rule.Name = "Decal " + std::to_string(params.Decals.size());
            params.Decals.push_back(rule);
            bNeedsPreviewRender = true;
        }
        ImGui::Spacing();
        
        for (int i = 0; i < (int)params.Decals.size(); ++i) {
            ImGui::PushID(i + 2000);
            char label[64]; snprintf(label, sizeof(label), "Decal %d - %s", i, params.Decals[i].Name.c_str());
            if (ImGui::CollapsingHeader(label)) {
                if (ImGui::Checkbox("Enabled", &params.Decals[i].Enabled)) bNeedsPreviewRender = true;
                
                char nameBuf[128]; strncpy(nameBuf, params.Decals[i].Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Decals[i].Name = nameBuf;
                
                char bpBuf[256]; strncpy(bpBuf, params.Decals[i].BlueprintPath.c_str(), sizeof(bpBuf));
                if (ImGui::InputText("Blueprint", bpBuf, IM_ARRAYSIZE(bpBuf))) params.Decals[i].BlueprintPath = bpBuf;
                
                if (ImGui::SliderFloat("Density", &params.Decals[i].Density, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                
                if (UI::RangeSliderFloat("Slope Range", &params.Decals[i].MinSlope, &params.Decals[i].MaxSlope, 0.0f, 90.0f)) {
                    bNeedsPreviewRender = true;
                }
                
                if (UI::RangeSliderFloat("Height Range", &params.Decals[i].MinHeight, &params.Decals[i].MaxHeight, params.TerrainMinHeight, params.TerrainMaxHeight)) {
                    bNeedsPreviewRender = true;
                }
                
                if (ImGui::Button("Delete Rule", ImVec2(-1, 20))) {
                    params.Decals.erase(params.Decals.begin() + i);
                    bNeedsPreviewRender = true;
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
        
        // Reclaim density moved to markers tab
    ImGui::EndChild();
}


} // namespace UI
} // namespace SanmapGen
