// Application_Window_UI.cpp — the platform bring-up and teardown: the GLFW window, the OpenGL 4.3
// context, the imgui runtime, and the ONE Sys::GpuResourceManager the whole process shares.
// Layer: UI. This is the only SanGen translation unit that calls glfwInit/glfwCreateWindow; every
// GL object in the process still belongs to GpuResourceManager (ARCH §3.2) — nothing here creates
// a texture, a buffer or a program, it only makes a context current for the seam that does.
#include "Application_UI.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

GLFWwindow* AsGlfwWindow(void* windowHandle) { return static_cast<GLFWwindow*>(windowHandle); }

void ReportGlfwError(int errorCode, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", errorCode, description);
}

} // namespace

bool Application::Initialize() {
    glfwSetErrorCallback(&ReportGlfwError);
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, settings.glContextMajorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, settings.glContextMinorVersion);
    glfwWindowHint(GLFW_VISIBLE, settings.bWindowVisible ? GLFW_TRUE : GLFW_FALSE);
    windowHandle = glfwCreateWindow(settings.windowWidth, settings.windowHeight,
                                    settings.windowTitle.c_str(), nullptr, nullptr);
    if (windowHandle == nullptr) { glfwTerminate(); return false; }
    glfwMakeContextCurrent(AsGlfwWindow(windowHandle));
    glfwSwapInterval(settings.bVerticalSyncEnabled ? 1 : 0);
    InitializeImgui();
    InitializeGpuResources();
    return true;
}

void Application::InitializeImgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
        ImGui::GetStyle().WindowRounding = 0.0f;
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    ImGui_ImplGlfw_InitForOpenGL(AsGlfwWindow(windowHandle), true);
    ImGui_ImplOpenGL3_Init(settings.glslVersionDirective.c_str());
    bImguiReady = true;
}

// One manager for the process: the PROC stage kernels and the preview composite share its compiled
// programs and persistent buffers. With no manager both fall back to their Cpu twins rather than
// failing, so a machine without a compute-capable driver still runs (Constitution §6).
void Application::InitializeGpuResources() {
    gpuResourceManager.reset(new Sys::GpuResourceManager(settings.shaderSearchDirectories));
    if (!gpuResourceManager->Initialize()) { gpuResourceManager.reset(); return; }
    assembler.SetGpuResourceManager(gpuResourceManager.get());
    composite.SetGpuResourceManager(gpuResourceManager.get());
}

// Order matters: every GL object is released while the context is STILL CURRENT, then imgui, then
// the window. Idempotent, so the destructor may call it after an explicit Shutdown().
void Application::Shutdown() {
    assembler.SetGpuResourceManager(nullptr);
    composite.SetGpuResourceManager(nullptr);
    canvas.SetPreviewTexture(nullptr, Sys::GpuTextureHandle(), composite.Resolution());
    atlasResidency.Clear();
    gpuResourceManager.reset();
    if (bImguiReady) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        bImguiReady = false;
    }
    if (windowHandle != nullptr) {
        glfwDestroyWindow(AsGlfwWindow(windowHandle));
        windowHandle = nullptr;
        glfwTerminate();
    }
}

bool Application::IsWindowOpen() const {
    return windowHandle != nullptr && glfwWindowShouldClose(AsGlfwWindow(windowHandle)) == 0;
}

void Application::RequestClose() {
    if (windowHandle != nullptr) glfwSetWindowShouldClose(AsGlfwWindow(windowHandle), GLFW_TRUE);
}

} // namespace Ui
} // namespace SanmapGen
