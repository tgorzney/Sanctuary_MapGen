// Erosion_Gpu_PROC_Test.cpp — Cpu/Gpu parity acceptance test for the erosion stage (M3-3).
// Needs a real GL context, so it spins up a hidden-window WGL context (test harness, not app
// code), exactly like GpuResource_SYS_Test. Build with MSVC from src/proc:
//   cl /EHsc /std:c++17 /O2 Erosion_Gpu_PROC_Test.cpp Erosion_PROC.cpp Erosion_Field_PROC.cpp \
//      Erosion_Rain_PROC.cpp Erosion_Spawn_PROC.cpp Erosion_Droplet_PROC.cpp \
//      Erosion_DropletTransfer_PROC.cpp Erosion_Accumulation_PROC.cpp Erosion_Gpu_PROC.cpp \
//      ..\sys\GpuResource_*.cpp ..\sys\GpuGlFunctions_SYS.cpp opengl32.lib gdi32.lib user32.lib
// argv[1] = shader directory (defaults to "."; the .glsl files live beside this file).
// Both backends start from the identical terrain, rain field and spawn list, so the numbers
// below measure the KERNELS, not the setup.
#include "Erosion_TestFixture_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <cmath>
#include <string>

using namespace SanmapGen;
using namespace SanmapGen::ErosionTest;

static bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "ErosionGpuTestWindow";
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
    const int pixelFormat = ChoosePixelFormat(outDeviceContext, &descriptor);
    if (!pixelFormat || !SetPixelFormat(outDeviceContext, pixelFormat, &descriptor)) return false;
    outGlContext = wglCreateContext(outDeviceContext);
    return outGlContext && wglMakeCurrent(outDeviceContext, outGlContext);
}

// Visual-class equivalence: same magnitude of change, in the same places.
static void CompareBackends(const std::vector<float>& original, const std::vector<float>& cpuHeights,
                            const std::vector<float>& gpuHeights) {
    double absoluteDifference = 0.0, deltaProduct = 0.0, cpuDeltaSquared = 0.0, gpuDeltaSquared = 0.0;
    bool bFinite = true;
    float highest = 0.0f;
    for (std::size_t index = 0; index < original.size(); ++index) {
        const double cpuDelta = static_cast<double>(cpuHeights[index]) - original[index];
        const double gpuDelta = static_cast<double>(gpuHeights[index]) - original[index];
        absoluteDifference += std::fabs(static_cast<double>(cpuHeights[index]) - gpuHeights[index]);
        deltaProduct += cpuDelta * gpuDelta;
        cpuDeltaSquared += cpuDelta * cpuDelta;
        gpuDeltaSquared += gpuDelta * gpuDelta;
        if (!(gpuHeights[index] == gpuHeights[index])) bFinite = false;
        if (gpuHeights[index] > highest) highest = gpuHeights[index];
    }
    const double originalVolume = TotalVolume(original);
    const double gpuDrift = (TotalVolume(gpuHeights) - originalVolume) / originalVolume;
    const double cpuDrift = (TotalVolume(cpuHeights) - originalVolume) / originalVolume;
    const double meanDifference = absoluteDifference / static_cast<double>(original.size());
    const double correlation = deltaProduct / (std::sqrt(cpuDeltaSquared * gpuDeltaSquared) + 1e-12);
    std::printf("gpu volume drift %+.6f%%, mean |cpu-gpu| %.5f, erosion correlation %.3f, peak %.4f\n",
                gpuDrift * 100.0, meanDifference, correlation, highest);

    Check(bFinite, "no NaN in the Gpu heightfield");
    Check(gpuDrift < 0.001 && gpuDrift > -0.001, "Gpu run conserves volume to 0.1%");
    Check(cpuDrift < 0.001 && cpuDrift > -0.001, "Cpu run conserves volume to 0.1%");
    Check(highest < 2.0f, "no runaway peak on the Gpu");
    Check(meanDifference < 0.02, "Cpu and Gpu agree within Visual tolerance (mean |diff| < 0.02)");
    Check(correlation > 0.5, "both backends eroded the same places (delta correlation > 0.5)");
}

int main(int argc, char** argv) {
    const std::string shaderDirectory = (argc > 1) ? argv[1] : ".";
    Params::Geometry geometry;
    geometry.mapSize = 128;
    const int vertexSize = geometry.VertexSize();

    Data::MapFields originalFields;
    BuildTestTerrain(originalFields, vertexSize);
    const std::vector<float> original = HeightfieldCopy(originalFields);

    Data::MapFields cpuFields;
    BuildTestTerrain(cpuFields, vertexSize);
    Proc::ErosionStage cpuStage(geometry, cpuFields);
    ConfigureStage(cpuStage);
    cpuStage.SetGenerationContext(Sys::GenerationContext::Output);
    Check(cpuStage.Run() == Sys::ComputeBackend::Cpu, "Output context resolves erosion to the Cpu");
    const std::vector<float> cpuHeights = HeightfieldCopy(cpuFields);

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available in this environment\n");
        return 2;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    Check(manager.Initialize(), "Gpu resource manager initialises");

    Data::MapFields gpuFields;
    BuildTestTerrain(gpuFields, vertexSize);
    Proc::ErosionStage gpuStage(geometry, gpuFields);
    ConfigureStage(gpuStage);
    gpuStage.SetGpuResourceManager(&manager);
    gpuStage.SetGenerationContext(Sys::GenerationContext::Preview);   // Preview => Gpu / Visual
    Check(gpuStage.Run() == Sys::ComputeBackend::Gpu, "Preview context resolves erosion to the Gpu");
    if (!gpuStage.IsGpuAvailable()) {
        std::printf("SKIP: erosion compute program did not compile in this environment\n");
        return 2;
    }
    Check(manager.CompileCount() == 1, "the three-unit erosion program compiled exactly once");

    bool bNonNegative = true;
    for (int ticks : gpuStage.ThicknessFixedPoint()) if (ticks < 0) bNonNegative = false;
    Check(bNonNegative, "Gpu atomics never drove a stratum below zero (no float RMW race)");
    CompareBackends(original, cpuHeights, HeightfieldCopy(gpuFields));

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", FailureCount());
    return 1;
}
