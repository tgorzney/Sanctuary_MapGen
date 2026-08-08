import os

with open("gui/UIHelpers.h", "r", encoding="utf-8") as f:
    content = f.read()

# Remove the old buttons
old_buttons_block = """        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        
        if (ImGui::Button(layer.Enabled ? "[o]##vis" : "[-]##vis")) {
            layer.Enabled = !layer.Enabled;
            bNeedsMapUpdate = true;
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(layer.Locked ? "[L]##lock" : "[U]##lock")) {
            layer.Locked = !layer.Locked;
            changed = true;
        }
        ImGui::SameLine();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

"""
content = content.replace(old_buttons_block, "")

# Insert the buttons on the right side
old_right_side = """        ImGui::PopStyleColor(3);
        
        if (layer.Locked) ImGui::BeginDisabled();
        
        // Add Delete Button to the right side of the header
        bool bDeleteLayer = false;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);"""

new_right_side = """        ImGui::PopStyleColor(3);
        
        // Add Vis and Lock buttons
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        if (ImGui::Button((layer.Enabled ? "[o]##vis" + std::to_string(i) : "[-]##vis" + std::to_string(i)).c_str())) {
            layer.Enabled = !layer.Enabled;
            bNeedsMapUpdate = true;
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
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);"""

content = content.replace(old_right_side, new_right_side)

with open("gui/UIHelpers.h", "w", encoding="utf-8") as f:
    f.write(content)
