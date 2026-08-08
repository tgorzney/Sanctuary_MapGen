#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include <functional>

namespace SanmapGen {
namespace UI {

template <typename T>
bool RenderDraggableLayerList(
    const char* listId, 
    std::vector<T>& layers, 
    std::function<void(T& layer, size_t index, bool& bNeedsUpdate, bool& bDeleteLayer)> renderLayerContent,
    bool& bNeedsMapUpdate,
    int* selectedIndex = nullptr,
    bool* pbNeedsPreviewRender = nullptr)
{
    bool changed = false;
    ImGui::PushID(listId);
    for (size_t i = 0; i < layers.size(); ) {
        ImGui::PushID(static_cast<int>(i));
        
        auto& layer = layers[i];
        
        float headerWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 headerPos = ImGui::GetCursorScreenPos();
        float headerHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;

        ImGui::GetWindowDrawList()->AddRectFilled(
            headerPos, ImVec2(headerPos.x + headerWidth, headerPos.y + headerHeight), IM_COL32(45, 45, 48, 255)
        );

        bool isSelected = (selectedIndex != nullptr && *selectedIndex == (int)i);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.2f, 0.4f, 0.8f, 0.8f)); // Light Blue
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.9f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.30f, 0.30f, 0.33f, 1.0f));
        }
        
        bool expanded = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap);
        
        // Handle selection on click
        if (selectedIndex != nullptr && ImGui::IsItemClicked(0)) {
            *selectedIndex = (int)i;
        }
        
        ImGui::PopStyleColor(3);
        
        // Add Vis and Lock buttons
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        if (ImGui::Button((layer.Enabled ? "[o]##vis" + std::to_string(i) : "[-]##vis" + std::to_string(i)).c_str())) {
            layer.Enabled = !layer.Enabled;
            if (pbNeedsPreviewRender) *pbNeedsPreviewRender = true;
            else bNeedsMapUpdate = true;
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button((layer.Locked ? "[L]##lock" + std::to_string(i) : "[U]##lock" + std::to_string(i)).c_str())) {
            layer.Locked = !layer.Locked;
            changed = true;
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        
        if (layer.Locked) ImGui::BeginDisabled();
        
        // Add Delete Button to the right side of the header
        bool bDeleteLayer = false;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(("X##dellayer" + std::to_string(i)).c_str())) {
            ImGui::OpenPopup(("DeleteLayerConfirm" + std::to_string(i)).c_str());
        }
        ImGui::PopStyleColor(2);
        
        if (ImGui::BeginPopupModal(("DeleteLayerConfirm" + std::to_string(i)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete this layer?");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                bDeleteLayer = true;
                if (selectedIndex != nullptr && *selectedIndex == (int)i) *selectedIndex = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }


        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload(listId, &i, sizeof(size_t));
            ImGui::Text("Moving: %s", layer.Name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(listId)) {
                size_t source_i = *(const size_t*)payload->Data;
                if (source_i < layers.size() && source_i != i) {
                    T movingLayer = layers[source_i];
                    layers.erase(layers.begin() + source_i);
                    size_t insert_i = (source_i < i) ? i - 1 : i;
                    layers.insert(layers.begin() + insert_i, movingLayer);
                    bNeedsMapUpdate = true;
                    changed = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (expanded) {
            ImGui::Indent();
            renderLayerContent(layers[i], i, bNeedsMapUpdate, bDeleteLayer);
            ImGui::Unindent();
        }
        
        if (layer.Locked) ImGui::EndDisabled();
        
        ImGui::PopID();
        
        if (bDeleteLayer) {
            layers.erase(layers.begin() + i);
            bNeedsMapUpdate = true;
            changed = true;
        } else {
            ++i;
        }
    }
    ImGui::PopID();
    return changed;
}

}
}
