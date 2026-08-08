import os

# Fix UIHelpers.h ID conflict
with open("gui/UIHelpers.h", "r", encoding="utf-8") as f:
    content = f.read()

# Add PushID and PopID to the function
if "bool changed = false;" in content and "ImGui::PushID(listId);" not in content:
    content = content.replace("bool changed = false;\n    for (size_t i = 0; i < layers.size(); ) {", "bool changed = false;\n    ImGui::PushID(listId);\n    for (size_t i = 0; i < layers.size(); ) {")
    content = content.replace("    return changed;\n}", "    ImGui::PopID();\n    return changed;\n}")

with open("gui/UIHelpers.h", "w", encoding="utf-8") as f:
    f.write(content)

# Fix main.cpp dragging bug
with open("gui/main.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Disable map drag if dragging marker
if "if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {" in content:
    content = content.replace("if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {", "if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !isDraggingMarker) {")

# Increase hit padding for marker picking
old_hit = "bool hit = (mousePos.x >= iconP0.x && mousePos.x <= iconP1.x && mousePos.y >= iconP0.y && mousePos.y <= iconP1.y);"
new_hit = """float pad = 8.0f; // Padding to make grabbing instant and easier
                                  bool hit = (mousePos.x >= iconP0.x - pad && mousePos.x <= iconP1.x + pad && mousePos.y >= iconP0.y - pad && mousePos.y <= iconP1.y + pad);"""

content = content.replace(old_hit, new_hit)

with open("gui/main.cpp", "w", encoding="utf-8") as f:
    f.write(content)

