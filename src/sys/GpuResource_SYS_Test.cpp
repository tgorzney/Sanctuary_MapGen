// GpuResource_SYS_Test.cpp — acceptance test for GpuResource_SYS (M0-9 programs/buffers,
// M5-0b textures + shader search path). Needs a real GL context, so the shared harness spins
// up a hidden-window WGL context once (test scaffolding, not app code). Build with MSVC:
//   cl /EHsc /std:c++17 /I. GpuResource_SYS_Test.cpp GpuResource_Texture_SYS_Test.cpp \
//      GpuResource_ShaderPath_SYS_Test.cpp GpuResource_Program_SYS.cpp GpuResource_Buffer_SYS.cpp \
//      GpuResource_Texture_SYS.cpp GpuResource_ShaderPath_SYS.cpp GpuResource_ProgramParts_SYS.cpp \
//      GpuGlFunctions_SYS.cpp opengl32.lib gdi32.lib user32.lib
// Optional argv[1] = a writable shader directory (defaults to the working directory).
#include "GpuResource_SYS.h"
#include "GpuResource_TestSupport_SYS.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace SanmapGen::Sys;
using namespace GpuResourceTest;

int main(int argc, char** argv) {
    const std::string shaderDirectory = (argc > 1) ? argv[1] : ".";
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
    WriteTestFile(shaderDirectory + "/" + shaderName, kernelSource);

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available in this environment\n");
        return 2;
    }

    GpuResourceManager manager(shaderDirectory);
    Check(manager.Initialize(), "manager initializes against the GL loader");

    const std::string defines = "#define WORKGROUP_SIZE " + std::to_string(WorkgroupSize::kDropletLinear);

    // 1. Compile-once: many GetOrCompileProgram calls, a single actual compile.
    GpuProgramHandle program = manager.GetOrCompileProgram(shaderName, defines);
    Check(program.IsValid(), "program compiles");
    for (int i = 0; i < 16; ++i) manager.GetOrCompileProgram(shaderName, defines);
    Check(manager.CompileCount() == 1, "program compiled exactly once across 17 requests");

    // 2. Persistent buffers reallocate only on size change.
    const int elementCount = 1024;
    const size_t byteSize = elementCount * sizeof(float);
    Check(manager.EnsureBuffer("data", byteSize), "first EnsureBuffer allocates");
    Check(!manager.EnsureBuffer("data", byteSize), "same-size EnsureBuffer reuses (no realloc)");
    Check(manager.EnsureBuffer("data", byteSize * 2), "larger EnsureBuffer reallocates");
    Check(manager.EnsureBuffer("data", byteSize), "shrink EnsureBuffer reallocates");
    Check(manager.ReallocationCount() == 3, "exactly three (re)allocations occurred");

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
    Check(signaled, "fence signals completion (async, no blocking map)");
    manager.DeleteFence(fence);

    std::vector<float> output(elementCount, -1.0f);
    manager.ReadbackBuffer("data", output.data(), byteSize);
    bool correct = true;
    for (int i = 0; i < elementCount; ++i) if (output[i] != static_cast<float>(i) * 2.0f) correct = false;
    Check(correct, "kernel doubled every element, readback correct");

    // 4. M5-0b: the managed texture primitive and the multi-directory shader search path.
    RunTextureChecks(shaderDirectory);
    RunShaderSearchPathChecks(shaderDirectory);

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", FailureCount());
    return 1;
}
