import os

with open("gui/UIHelpers.h", "r", encoding="utf-8") as f:
    content = f.read()

# Replace Checkbox with Visibility and Lock
old_checkbox = """        if (ImGui::Checkbox("##enabled", &layer.Enabled)) {
            bNeedsMapUpdate = true;
            changed = true;
        }
        ImGui::SameLine();"""

new_buttons = """        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
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
        ImGui::PopStyleVar();"""

content = content.replace(old_checkbox, new_buttons)

old_delete = """        // Add Delete Button to the right side of the header
        bool bDeleteLayer = false;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(("X##dellayer" + std::to_string(i)).c_str())) {"""

new_delete = """        if (layer.Locked) ImGui::BeginDisabled();
        
        // Add Delete Button to the right side of the header
        bool bDeleteLayer = false;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(("X##dellayer" + std::to_string(i)).c_str())) {"""

content = content.replace(old_delete, new_delete)

old_end_render = """        if (expanded) {
            ImGui::Indent();
            renderLayerContent(layers[i], i, bNeedsMapUpdate, bDeleteLayer);
            ImGui::Unindent();
        }
        
        ImGui::PopID();"""

new_end_render = """        if (expanded) {
            ImGui::Indent();
            renderLayerContent(layers[i], i, bNeedsMapUpdate, bDeleteLayer);
            ImGui::Unindent();
        }
        
        if (layer.Locked) ImGui::EndDisabled();
        
        ImGui::PopID();"""

content = content.replace(old_end_render, new_end_render)

with open("gui/UIHelpers.h", "w", encoding="utf-8") as f:
    f.write(content)
