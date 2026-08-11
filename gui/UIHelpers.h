#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <cmath>

namespace SanmapGen {
namespace UI {

inline bool RangeSliderFloat(const char* label, float* v_min, float* v_max, float v_min_limit, float v_max_limit, const char* format = "%.3f", float min_increment = 0.001f) {
    bool value_changed = false;

    ImGui::PushID(label);

    ImGui::Text("%s", label);

    float avail_width = ImGui::GetContentRegionAvail().x;
    float track_height = ImGui::GetFrameHeight();
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    float handle_width = 10.0f;
    float usable_width = avail_width - handle_width;

    ImGui::InvisibleButton("##track", ImVec2(avail_width, track_height));
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();

    float range = v_max_limit - v_min_limit;
    if (range < 0.00001f) range = 1.0f; // Prevent div-by-zero, fallback to 1.0 range
    
    float normalized_min = std::clamp((*v_min - v_min_limit) / range, 0.0f, 1.0f);
    float normalized_max = std::clamp((*v_max - v_min_limit) / range, 0.0f, 1.0f);

    static ImGuiID active_slider_id = 0;
    static int active_handle = -1;
    static bool was_dragging = false;
    
    ImGuiID id = ImGui::GetID("##track");

    ImGuiID rt_id = ImGui::GetID("##rt_toggle");
    bool* p_rt = ImGui::GetStateStorage()->GetBoolRef(rt_id, true);
    bool update_rt = *p_rt;

    if (is_active && ImGui::IsMouseClicked(0)) {
        active_slider_id = id;
        was_dragging = false;
        float mouse_x = ImGui::GetIO().MousePos.x;
        float handle_min_x = pos.x + normalized_min * usable_width + handle_width * 0.5f;
        float handle_max_x = pos.x + normalized_max * usable_width + handle_width * 0.5f;
        
        if (std::abs(mouse_x - handle_min_x) < std::abs(mouse_x - handle_max_x)) {
            active_handle = 0;
        } else {
            active_handle = 1;
        }
    }

    if (is_active && active_slider_id == id && ImGui::IsMouseDragging(0, 0.0f)) {
        float mouse_x = ImGui::GetIO().MousePos.x;
        float relative_x = mouse_x - (pos.x + handle_width * 0.5f);
        float new_normalized = std::clamp(relative_x / usable_width, 0.0f, 1.0f);
        float new_value = v_min_limit + new_normalized * range;
        
        if (active_handle == 0) {
            float upper = std::max(v_min_limit, *v_max - min_increment);
            *v_min = std::clamp(new_value, v_min_limit, upper);
            if (update_rt) value_changed = true;
            was_dragging = true;
        } else if (active_handle == 1) {
            float lower = std::min(v_max_limit, *v_min + min_increment);
            *v_max = std::clamp(new_value, lower, v_max_limit);
            if (update_rt) value_changed = true;
            was_dragging = true;
        }
    }

    if (!ImGui::IsMouseDown(0) && active_slider_id == id) {
        if (!update_rt && was_dragging) {
            value_changed = true;
        }
        active_slider_id = 0;
        active_handle = -1;
        was_dragging = false;
    }

    normalized_min = std::clamp((*v_min - v_min_limit) / range, 0.0f, 1.0f);
    normalized_max = std::clamp((*v_max - v_min_limit) / range, 0.0f, 1.0f);

    ImU32 bg_col = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 fill_col = ImGui::GetColorU32(ImGuiCol_SliderGrab);
    draw_list->AddRectFilled(pos, ImVec2(pos.x + avail_width, pos.y + track_height), bg_col, ImGui::GetStyle().FrameRounding);

    ImVec2 fill_min_pos(pos.x + normalized_min * usable_width + handle_width * 0.5f, pos.y);
    ImVec2 fill_max_pos(pos.x + normalized_max * usable_width + handle_width * 0.5f, pos.y + track_height);
    draw_list->AddRectFilled(fill_min_pos, fill_max_pos, fill_col, ImGui::GetStyle().FrameRounding);

    ImU32 handle_col = ImGui::GetColorU32(ImGuiCol_Button);
    ImU32 handle_active_col = ImGui::GetColorU32(ImGuiCol_ButtonActive);
    
    ImVec2 handle_min_p0(pos.x + normalized_min * usable_width, pos.y);
    ImVec2 handle_min_p1(handle_min_p0.x + handle_width, pos.y + track_height);
    draw_list->AddRectFilled(handle_min_p0, handle_min_p1, (active_slider_id == id && active_handle == 0) ? handle_active_col : handle_col, ImGui::GetStyle().FrameRounding);

    ImVec2 handle_max_p0(pos.x + normalized_max * usable_width, pos.y);
    ImVec2 handle_max_p1(handle_max_p0.x + handle_width, pos.y + track_height);
    draw_list->AddRectFilled(handle_max_p0, handle_max_p1, (active_slider_id == id && active_handle == 1) ? handle_active_col : handle_col, ImGui::GetStyle().FrameRounding);

    float rt_btn_width = 30.0f;
    float input_width = (avail_width - ImGui::GetStyle().ItemSpacing.x * 2.0f - rt_btn_width) * 0.5f;
    
    ImGui::SetNextItemWidth(input_width);
    if (ImGui::DragFloat("##min_input", v_min, min_increment, v_min_limit, *v_max - min_increment, format)) {
        float upper = std::max(v_min_limit, *v_max - min_increment);
        *v_min = std::clamp(*v_min, v_min_limit, upper);
        value_changed = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(input_width);
    if (ImGui::DragFloat("##max_input", v_max, min_increment, *v_min + min_increment, v_max_limit, format)) {
        float lower = std::min(v_max_limit, *v_min + min_increment);
        *v_max = std::clamp(*v_max, lower, v_max_limit);
        value_changed = true;
    }
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, update_rt ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_Button));
    if (ImGui::Button("RT", ImVec2(rt_btn_width, 0))) {
        *p_rt = !(*p_rt);
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle Realtime Update");
    }

    ImGui::PopID();
    return value_changed;
}

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
