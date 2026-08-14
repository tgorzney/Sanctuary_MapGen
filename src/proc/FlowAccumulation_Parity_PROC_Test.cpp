// FlowAccumulation_Parity_PROC_Test.cpp — the CPU/GPU parity half of the M3-5 acceptance
// list. Needs a real GL context, so it spins up a hidden-window WGL context (test harness,
// not app code) and reports "skipped" when none is available. The GPU relaxations converge to
// the CPU fixed point, so parity here is checked as an exact direction match plus a tight
// float tolerance on the surface, magnitude and accumulation fields.
#include "FlowAccumulation_TestSupport_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <cmath>
#include <vector>

using namespace SanmapGen;

namespace FlowAccumulationTest {
namespace {

constexpr float parityTolerance = 1.0e-4f;

bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "FlowAccumulationParityWindow";
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

float LargestDifference(const std::vector<float>& left, const float* right) {
    float largest = 0.0f;
    for (std::size_t index = 0; index < left.size(); ++index) {
        const float difference = std::fabs(left[index] - right[index]);
        if (difference > largest) largest = difference;
    }
    return largest;
}

// Runs the accuracy path, keeps its four outputs, then runs the speed path over the same
// inputs and compares. `label` distinguishes the filled / unfilled variants in the report.
void CompareBackendsOnce(Proc::FlowAccumulationStage& stage, Data::MapFields& fields,
                         const char* directionLabel, const char* fieldLabel) {
    const std::size_t cellCount = fields.accumulation.CellCount();
    stage.SetGenerationContext(Sys::GenerationContext::Output);
    Check(stage.Run() == Sys::ComputeBackend::Cpu, "Output context resolves to the Cpu path");
    const std::vector<int> cpuDirections = stage.FlowDirections();
    const std::vector<float> cpuSurface = stage.DrainageSurface();
    const std::vector<float> cpuFlow(fields.flow.Data(), fields.flow.Data() + cellCount);
    const std::vector<float> cpuAccumulation(fields.accumulation.Data(),
                                             fields.accumulation.Data() + cellCount);
    const int cpuSinkCount = stage.SinkCount();

    stage.SetGenerationContext(Sys::GenerationContext::Preview);
    Check(stage.Run() == Sys::ComputeBackend::Gpu, "Preview context resolves to the Gpu path");
    Check(!stage.WasGpuFallbackUsed(), "the Gpu backend actually ran (no capability fallback)");
    Check(stage.WasGpuConverged(), "both Gpu relaxations reached their fixed point inside budget");

    int directionMismatches = 0;
    for (std::size_t index = 0; index < cpuDirections.size(); ++index)
        if (cpuDirections[index] != stage.FlowDirections()[index]) ++directionMismatches;
    if (directionMismatches != 0)
        std::printf("  %s: %d/%zu cells differ\n", directionLabel, directionMismatches, cpuDirections.size());
    Check(directionMismatches == 0, directionLabel);
    Check(stage.SinkCount() == cpuSinkCount, "parity: both backends find the same sink count");

    const float surfaceDifference = LargestDifference(cpuSurface, stage.DrainageSurface().data());
    const float flowDifference = LargestDifference(cpuFlow, fields.flow.Data());
    const float accumulationDifference = LargestDifference(cpuAccumulation, fields.accumulation.Data());
    std::printf("  %s: surface %.3g, magnitude %.3g, accumulation %.3g"
                " (gpu iterations: fill %d, accumulation %d)\n",
                fieldLabel, surfaceDifference, flowDifference, accumulationDifference,
                stage.GpuFillIterationsUsed(), stage.GpuAccumulationIterationsUsed());
    Check(surfaceDifference <= parityTolerance && flowDifference <= parityTolerance
          && accumulationDifference <= parityTolerance, fieldLabel);
}

} // namespace

bool RunGpuParityChecks(int side, const char* shaderDirectory) {
    HWND window = nullptr;
    HDC deviceContext = nullptr;
    HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) return false;

    Sys::GpuResourceManager manager(shaderDirectory);
    if (!manager.Initialize()) { wglMakeCurrent(nullptr, nullptr); return false; }

    Params::Geometry geometry;
    geometry.mapSize = side - 1;
    Data::MapFields fields;
    BuildTiltedBowlTerrain(fields, side);
    Proc::FlowAccumulationStage stage(geometry, fields);
    stage.SetGpuResourceManager(&manager);
    stage.Constants().flowNoiseImpact = 0.35f;
    stage.Constants().flowNoiseSeed = 1234u;

    CompareBackendsOnce(stage, fields, "parity: identical flow directions (depressions filled)",
                        "parity: fields within tolerance (depressions filled)");
    stage.Constants().bFillDepressions = false;
    CompareBackendsOnce(stage, fields, "parity: identical flow directions (pits kept)",
                        "parity: fields within tolerance (pits kept)");
    Check(manager.CompileCount() == 3, "the three passes compile exactly once across every run");

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);
    return true;
}

} // namespace FlowAccumulationTest
