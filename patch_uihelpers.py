import os

with open("gui/UIHelpers.h", "r", encoding="utf-8") as f:
    content = f.read()

old_decl = """bool RenderDraggableLayerList(
    const char* listId, 
    std::vector<T>& layers, 
    std::function<void(T& layer, size_t index, bool& bNeedsUpdate, bool& bDeleteLayer)> renderLayerContent,
    bool& bNeedsMapUpdate)"""
new_decl = """bool RenderDraggableLayerList(
    const char* listId, 
    std::vector<T>& layers, 
    std::function<void(T& layer, size_t index, bool& bNeedsUpdate, bool& bDeleteLayer)> renderLayerContent,
    bool& bNeedsMapUpdate,
    int* selectedIndex = nullptr)"""

old_loop = """        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,   ImVec4(0.30f, 0.30f, 0.33f, 1.0f));
        bool expanded = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap);
        ImGui::PopStyleColor(3);"""

new_loop = """        bool isSelected = (selectedIndex != nullptr && *selectedIndex == (int)i);
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
"""

content = content.replace(old_decl, new_decl)
content = content.replace(old_loop, new_loop)

# Also need to remove the old bDeleteLayer = false declaration inside the expanded block since we moved it above
content = content.replace("        bool bDeleteLayer = false;\n        if (expanded) {", "        if (expanded) {")

with open("gui/UIHelpers.h", "w", encoding="utf-8") as f:
    f.write(content)
