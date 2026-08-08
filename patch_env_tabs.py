import os

with open("gui/EnvironmentTabs.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# For the Placed Markers
# We need to change:
# if (ImGui::CollapsingHeader(label)) {
# To:
# bool expanded = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_AllowOverlap);
# ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
# if (ImGui::ColorEdit4("##ColorOverride", marker.Color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreview)) localUpdate = true;
# ImGui::SameLine();
# ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
# ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
# if (ImGui::Button(("X##delmarker" + key).c_str())) ImGui::OpenPopup(("DeleteMarkerConfirm" + key).c_str());
# ImGui::PopStyleColor(2);
# if (ImGui::BeginPopupModal(("DeleteMarkerConfirm" + key).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
#     ImGui::Text("Delete this marker?");
#     if (ImGui::Button("OK", ImVec2(80,0))) { markerToDelete = key; ImGui::CloseCurrentPopup(); }
#     ImGui::SameLine();
#     if (ImGui::Button("Cancel", ImVec2(80,0))) { ImGui::CloseCurrentPopup(); }
#     ImGui::EndPopup();
# }
# if (expanded) {

old_marker_header = """                              if (ImGui::CollapsingHeader(label)) {
                                  bool localUpdate = false;"""

new_marker_header = """                              bool expanded = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_AllowOverlap);
                              bool localUpdate = false;
                              ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
                              if (ImGui::ColorEdit4(("##ColorOverride" + key).c_str(), marker.Color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreview)) localUpdate = true;
                              ImGui::SameLine();
                              ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                              ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                              if (ImGui::Button(("X##delmarker" + key).c_str())) ImGui::OpenPopup(("DeleteMarkerConfirm" + key).c_str());
                              ImGui::PopStyleColor(2);
                              
                              if (ImGui::BeginPopupModal(("DeleteMarkerConfirm" + key).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                                  ImGui::Text("Delete this marker?");
                                  ImGui::Separator();
                                  if (ImGui::Button("OK", ImVec2(80,0))) { markerToDelete = key; ImGui::CloseCurrentPopup(); }
                                  ImGui::SameLine();
                                  if (ImGui::Button("Cancel", ImVec2(80,0))) { ImGui::CloseCurrentPopup(); }
                                  ImGui::EndPopup();
                              }
                              
                              if (expanded) {"""

content = content.replace(old_marker_header, new_marker_header)
content = content.replace('if (ImGui::ColorEdit4("Color Override", marker.Color)) localUpdate = true;', '')

# Also for Procedural Rules, we might want to do the same for rules delete. The user didn't explicitly say for rules but "Move the Marker Color Overide... and have te trash icon to right of that". Procedural rules don't have a single color override right now, but they do have a delete.
# I'll just add the trash icon for Procedural Rules too to match.
old_rule_header = """                          if (ImGui::CollapsingHeader(label)) {
                              char nameBuf[128];"""
                              
new_rule_header = """                          bool expanded = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_AllowOverlap);
                          ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
                          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                          if (ImGui::Button(("X##delrule" + std::to_string(i)).c_str())) ImGui::OpenPopup(("DeleteRuleConfirm" + std::to_string(i)).c_str());
                          ImGui::PopStyleColor(2);
                          
                          if (ImGui::BeginPopupModal(("DeleteRuleConfirm" + std::to_string(i)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                              ImGui::Text("Delete this rule?");
                              ImGui::Separator();
                              if (ImGui::Button("OK", ImVec2(80,0))) { ruleToDelete = i; ImGui::CloseCurrentPopup(); }
                              ImGui::SameLine();
                              if (ImGui::Button("Cancel", ImVec2(80,0))) { ImGui::CloseCurrentPopup(); }
                              ImGui::EndPopup();
                          }
                          
                          if (expanded) {
                              char nameBuf[128];"""
                              
content = content.replace(old_rule_header, new_rule_header)
content = content.replace('if (ImGui::Button("Delete Rule", ImVec2(-1, 25))) {\n                                  ruleToDelete = i;\n                              }', '')

with open("gui/EnvironmentTabs.cpp", "w", encoding="utf-8") as f:
    f.write(content)
