#pragma once
#include <imgui.h>
#include <string>
#include <vector>

namespace SanmapGen {
namespace UI {

    // Templated utility for rendering massive lists of data using ImGuiListClipper
    // Enforces Data-Oriented Design (SoA) and eliminates duplicate boilerplate
    template<typename T>
    class VirtualListRenderer {
    public:
        // Render a virtualized list
        // @param id The unique ImGui ID for the child window
        // @param items The data array (SoA structure or contiguous array)
        // @param itemHeight The fixed height of each row
        // @param renderCallback Lambda or function to render a single item
        template<typename RenderFunc>
        static void Render(const char* id, const std::vector<T>& items, float itemHeight, RenderFunc renderCallback) {
            if (items.empty()) {
                ImGui::TextDisabled("No items found.");
                return;
            }

            ImGui::BeginChild(id, ImVec2(0, 300), true);
            
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(items.size()), itemHeight);
            
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    renderCallback(i, items[i]);
                }
            }
            clipper.End();
            
            ImGui::EndChild();
        }
    };

} // namespace UI
} // namespace SanmapGen
