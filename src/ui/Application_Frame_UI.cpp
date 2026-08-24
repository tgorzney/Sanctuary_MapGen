// Application_Frame_UI.cpp — the frame loop. Layer: UI.
// The order of the steps below IS the two-tier dirty contract in action (UI_FRAMEWORK_SPEC
// "regen is dispatched off these flags in the main loop, not inline in the widget"):
//   1 events        2 begin frame        3 the pending asset load (announced, then performed)
//   4 the panels    - a committed edit calls PreviewDriver::NotifyParametersChanged(), which
//                     DERIVES bNeedsMapUpdate vs bNeedsPreviewRender from the stages' own hashes
//   5 execution     - the Performance toggles onto every stage's DispatchPolicy; a policy no
//                     parameter hash can see asks for a map update outright
//   6 icon bridge   - a new atlas pick becomes the selected rule's template id
//   7 service tier  - Refresh() runs the pipeline OR only the composite, then re-points the canvas
//   8 the canvas    - drawn AFTER the service, so an edit is visible in the SAME frame
//   9 end frame
#include "Application_UI.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace SanmapGen {
namespace Ui {
namespace {

GLFWwindow* AsGlfwWindow(void* windowHandle) { return static_cast<GLFWwindow*>(windowHandle); }

} // namespace

int Application::Run() {
    if (!Initialize()) return 1;
    while (RunOneFrame()) {}
    SaveAppSettingsAtShutdown();   // the clean-exit flush (STEP19_AppSettings_IO) — Run() only
    Shutdown();
    return 0;
}

bool Application::RunOneFrame() {
    if (!IsWindowOpen()) return false;
    PumpWindowEvents();
    BeginImguiFrame();
    if (ServiceAssetLoadRequest()) {       // the announce frame is presented on its own
        EndImguiFrame();
        ++frameCount;
        return IsWindowOpen();
    }
    if (ServiceTemplateIngestRequest()) {  // same announce-then-perform pattern, ticket 91
        EndImguiFrame();
        ++frameCount;
        return IsWindowOpen();
    }
    DrawSettingsWindow();
    ApplyExecutionPolicy();
    ResolveIconSelections();
    ServiceDirtyTier();
    DrawCanvasWindow();
    EndImguiFrame();
    ++frameCount;
    return IsWindowOpen();
}

// Poll while work is pending; block on events (with a timeout, so a hidden shell can never hang)
// once nothing is dirty — a settled app costs no CPU.
void Application::PumpWindowEvents() {
    if (previewDriver.NeedsMapUpdate() || previewDriver.NeedsPreviewRender() ||
        assetBridge.bAssetLoadRequested)
        glfwPollEvents();
    else
        glfwWaitEventsTimeout(static_cast<double>(settings.idleWaitSeconds));
}

void Application::BeginImguiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::EndImguiFrame() {
    ImGui::Render();
    int framebufferWidth = 0, framebufferHeight = 0;
    glfwGetFramebufferSize(AsGlfwWindow(windowHandle), &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(settings.backgroundColor[0], settings.backgroundColor[1],
                 settings.backgroundColor[2], settings.backgroundColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
        GLFWwindow* const contextBeforeViewports = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(contextBeforeViewports);
    }
    glfwSwapBuffers(AsGlfwWindow(windowHandle));
}

} // namespace Ui
} // namespace SanmapGen
