// ListWidget_TestFrame_UI.h — a headless imgui frame for the M5-2 list-widget acceptance tests.
// Test-support only: no GL, no window, no backend, no platform layer.
//
// ImGuiListClipper, button activation and drag-drop are all pure CPU logic inside imgui, so a
// context plus NewFrame/Render is the entire requirement. The font atlas is rasterized once in the
// session constructor because a legacy backend (one that does not advertise
// ImGuiBackendFlags_RendererHasTextures) is the party imgui expects to do that; nothing here ever
// uploads or draws the resulting pixels.
#pragma once
#include <cfloat>
#include <cstdio>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

inline int listWidgetTestFailureCount = 0;

inline void CheckListWidgetExpectation(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++listWidgetTestFailureCount; }
}

// The synthetic pointer for one frame. The default position is imgui's "no mouse anywhere".
struct HeadlessMouseState {
    ImVec2 position        = ImVec2(-FLT_MAX, -FLT_MAX);
    bool   bLeftButtonDown = false;
};

// Owns one imgui context for the lifetime of a test.
class HeadlessImguiSession {
public:
    HeadlessImguiSession() {
        IMGUI_CHECKVERSION();
        imguiContext = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime   = 1.0f / 60.0f;
        io.IniFilename = nullptr;                 // a test never writes imgui.ini
        io.LogFilename = nullptr;
        unsigned char* atlasPixels = nullptr;
        int atlasWidth = 0;
        int atlasHeight = 0;
        io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    }
    ~HeadlessImguiSession() { ImGui::DestroyContext(imguiContext); }
    HeadlessImguiSession(const HeadlessImguiSession&) = delete;
    HeadlessImguiSession& operator=(const HeadlessImguiSession&) = delete;

private:
    ImGuiContext* imguiContext = nullptr;
};

// One complete frame: feed the synthetic mouse, open a window whose position and size are pinned
// (so every hit-test coordinate is reproducible), run `drawContents`, finish the frame.
template <typename DrawContentsFunction>
void RunHeadlessFrame(const HeadlessMouseState& mouse, const ImVec2& windowSize,
                      DrawContentsFunction drawContents) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(mouse.position.x, mouse.position.y);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, mouse.bLeftButtonDown);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("ListWidgetTestWindow", nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
    drawContents();
    ImGui::End();
    ImGui::Render();
}

} // namespace Ui
} // namespace SanmapGen
