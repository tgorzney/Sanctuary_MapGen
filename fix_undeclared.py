import os

with open("gui/main.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Remove the old static declarations
old_statics = """            static std::string draggingMarker = "";
            static bool isDraggingMarker = false;
            static ImVec2 dragOffset(0, 0);"""

content = content.replace(old_statics, "")

# Insert them before the map dragging check
target = "if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !isDraggingMarker) {"
new_statics = """            static std::string draggingMarker = "";
            static bool isDraggingMarker = false;
            static ImVec2 dragOffset(0, 0);
            
            if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !isDraggingMarker) {"""

content = content.replace(target, new_statics)

with open("gui/main.cpp", "w", encoding="utf-8") as f:
    f.write(content)
