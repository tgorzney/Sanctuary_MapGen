#pragma once
#include <imgui.h>
#include "../core/Parameters.h"
#include "../PreviewRenderer.h"
#include <GLFW/glfw3.h>

namespace SanmapGen {
    class Widget_MapCanvas {
    private:
        static bool isDraggingMarker;
        static std::string draggingMarker;
        static ImVec2 dragOffset;
    public:
        static void Render(GenerationParams& params, GLuint previewTexture, bool& bNeedsMapUpdate, 
                           int& activeTab, std::string& selectedMarkerKey, bool& bNeedsPreviewRender, bool& bResetPreviewTransform, FloatMask& dummyMap);
    };
}
