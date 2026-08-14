// GpuResource_SYS_Test.cpp — acceptance test for GpuResource_SYS (M0-9). Needs a real GL
// context, so it spins up a hidden-window WGL context (this is a test harness, not app
// code). Build with MSVC:
//   cl /EHsc /std:c++17 /I. GpuResource_SYS_Test.cpp GpuResource_Program_SYS.cpp \
//      GpuResource_Buffer_SYS.cpp GpuGlFunctions_SYS.cpp opengl32.lib gdi32.lib user32.lib
// Optional argv[1] = shader directory (defaults to a temp dir the test writes into).
#include "GpuResource_SYS.h"
#include "GpuGlFunctions_SYS.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace SanmapGen::Sys;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

// Minimal hidden-window GL context so the manager has something to talk to.
static bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "GpuResourceTestWindow";
    RegisterClassA(&windowClass);
    outWindow = CreateWindowExA(0, windowClass.lpszClassName, "hidden", 0, 0, 0, 8, 8,
                                nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!outWindow) return false;
    outDeviceContext = GetDC(outWindow);
    PIXELFORMATDESCRIPTOR descriptor = {};
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    int pixelFormat = ChoosePixelFormat(outDeviceContext, &descriptor);
    if (!pixelFormat || !SetPixelFormat(outDeviceContext, pixelFormat, &descriptor)) return false;
    outGlContext = wglCreateContext(outDeviceContext);
    return outGlContext && wglMakeCurrent(outDeviceContext, outGlContext);
}

int main(int argc, char** argv) {
    std::string shaderDirectory = (argc > 1) ? argv[1] : ".";
    const std::string shaderName = "DoubleBuffer_Test.glsl";
    // Named workgroup size is injected as a #define so GLSL and dispatch math share it.
    const std::string kernelSource =
        "#version 430 core\n"
        "layout(local_size_x = WORKGROUP_SIZE) in;\n"
        "layout(std430, binding = 0) buffer Data { float values[]; };\n"
        "uniform int elementCount;\n"
        "void main() {\n"
        "    uint index = gl_GlobalInvocationID.x;\n"
        "    if (index < uint(elementCount)) values[index] *= 2.0;\n"
        "}\n";
    { std::ofstream out(shaderDirectory + "/" + shaderName); out << kernelSource; }

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available in this environment\n");
        return 2;
    }

    GpuResourceManager manager(shaderDirectory);
    check(manager.Initialize(), "manager initializes against the GL loader");

    const std::string defines = "#define WORKGROUP_SIZE " + std::to_string(WorkgroupSize::kDropletLinear);

    // 1. Compile-once: many GetOrCompileProgram calls, a single actual compile.
    GpuProgramHandle program = manager.GetOrCompileProgram(shaderName, defines);
    check(program.IsValid(), "program compiles");
    for (int i = 0; i < 16; ++i) manager.GetOrCompileProgram(shaderName, defines);
    check(manager.CompileCount() == 1, "program compiled exactly once across 17 requests");

    // 2. Persistent buffers reallocate only on size change.
    const int elementCount = 1024;
    const size_t byteSize = elementCount * sizeof(float);
    check(manager.EnsureBuffer("data", byteSize), "first EnsureBuffer allocates");
    check(!manager.EnsureBuffer("data", byteSize), "same-size EnsureBuffer reuses (no realloc)");
    check(manager.EnsureBuffer("data", byteSize * 2), "larger EnsureBuffer reallocates");
    check(manager.EnsureBuffer("data", byteSize), "shrink EnsureBuffer reallocates");
    check(manager.ReallocationCount() == 3, "exactly three (re)allocations occurred");

    // 3. Round-trip: upload -> dispatch doubling kernel -> async readback (no blocking map).
    std::vector<float> input(elementCount);
    for (int i = 0; i < elementCount; ++i) input[i] = static_cast<float>(i);
    manager.UploadBuffer("data", input.data(), byteSize);
    manager.BindBuffer("data", 0);
    manager.SetUniformInt(program, "elementCount", elementCount);
    unsigned groups = (elementCount + WorkgroupSize::kDropletLinear - 1) / WorkgroupSize::kDropletLinear;
    manager.Dispatch(program, groups, 1, 1);

    GpuFenceHandle fence = manager.InsertFence();
    bool signaled = false;
    for (int spin = 0; spin < 100000 && !signaled; ++spin) signaled = manager.IsFenceSignaled(fence);
    check(signaled, "fence signals completion (async, no blocking map)");
    manager.DeleteFence(fence);

    std::vector<float> output(elementCount, -1.0f);
    manager.ReadbackBuffer("data", output.data(), byteSize);
    bool correct = true;
    for (int i = 0; i < elementCount; ++i) if (output[i] != static_cast<float>(i) * 2.0f) correct = false;
    check(correct, "kernel doubled every element, readback correct");

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
