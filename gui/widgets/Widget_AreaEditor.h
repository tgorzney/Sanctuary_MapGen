#pragma once
#include <imgui.h>
#include "../core/Parameters.h"
#include <string>

namespace SanmapGen {
    class Widget_AreaEditor {
    private:
        static std::string draggingArea;
        static int dragCorner; // 0=none, 1=topleft, 2=topright, 3=bottomright, 4=bottomleft, 5=center
        static ImVec2 dragStartMouse;
        static MapArea dragStartArea;
        
    public:
        static void RenderOverlay(GenerationParams& params, ImVec2 p0, ImVec2 p1, ImVec2 uv0, ImVec2 uv1, float renderSize, float mapZoom, ImVec2 mapOffset, bool& bNeedsMapUpdate);
    };
}
