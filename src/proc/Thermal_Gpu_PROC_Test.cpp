// Thermal_Gpu_PROC_Test.cpp — the Cpu/Gpu parity half of the Thermal acceptance test (M3-4).
// Runs the SAME rough field through both backends and compares heightfield and material masks
// cell for cell, with material transport on and off. Needs a real GL context, so it spins up a
// hidden-window WGL context (test harness, not app code); returns -1 when no context is
// available so the caller can report a skip rather than a false pass.
#include "../sys/GpuGlFunctions_SYS.h"
#include "../sys/GpuResource_SYS.h"
#include "Thermal_Test_PROC.h"

namespace SanmapGen {
namespace ThermalTest {
namespace {

constexpr int   parityVertexSize     = 65;
constexpr int   parityIterationCount = 24;
constexpr float parityTolerance      = 1.0e-4f;

int parityFailures = 0;
void CheckParity(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++parityFailures; }
}

bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "ThermalParityTestWindow";
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

// A different talus angle per stratum, so the mask-weighted threshold really varies cell to cell.
void ConfigureStage(Proc::ThermalStage& stage, bool bTransportMaterialMasks) {
    stage.Constants().iterationCount = parityIterationCount;
    stage.Constants().bTransportMaterialMasks = bTransportMaterialMasks;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        stage.Constants().talusAngleDegrees[stratum] = 20.0f + static_cast<float>(stratum) * 5.0f;
}

// One identical rough field down each backend. Also asserts the dispatch policy resolved the way
// ARCH §4.2 says it should for this stage.
void RunBothBackends(Sys::GpuResourceManager& resourceManager, bool bTransportMaterialMasks,
                     Data::MapFields& cpuFields, Data::MapFields& gpuFields) {
    const Params::Geometry geometry = MakeGeometry(parityVertexSize);
    BuildRoughField(cpuFields, parityVertexSize);
    BuildRoughField(gpuFields, parityVertexSize);

    Proc::ThermalStage cpuStage(geometry, cpuFields);
    ConfigureStage(cpuStage, bTransportMaterialMasks);
    CheckParity(cpuStage.Run() == Sys::ComputeBackend::Cpu, "Output context resolves to the Cpu path");

    Proc::ThermalStage gpuStage(geometry, gpuFields);
    ConfigureStage(gpuStage, bTransportMaterialMasks);
    gpuStage.SetGpuResourceManager(&resourceManager);
    gpuStage.SetGenerationContext(Sys::GenerationContext::Preview);
    CheckParity(gpuStage.Run() == Sys::ComputeBackend::Gpu, "Preview context resolves to the Gpu speed path");
    CheckParity(gpuStage.CompletedIterationCount() == parityIterationCount, "the Gpu ran every sweep");
}

void CompareBackendResults(const Data::MapFields& cpuFields, const Data::MapFields& gpuFields) {
    const float heightDifference = MaximumFieldDifference(cpuFields.heightfield, gpuFields.heightfield);
    float maskDifference = 0.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const float difference = MaximumFieldDifference(cpuFields.materialMasks[stratum],
                                                        gpuFields.materialMasks[stratum]);
        if (difference > maskDifference) maskDifference = difference;
    }
    std::printf("parity: max height difference %.3e, max material-mask difference %.3e\n",
                static_cast<double>(heightDifference), static_cast<double>(maskDifference));
    CheckParity(heightDifference <= parityTolerance, "Cpu and Gpu heightfields agree within tolerance");
    CheckParity(maskDifference <= parityTolerance, "Cpu and Gpu material masks agree within tolerance");
    CheckParity(MaximumNeighbourDrop(gpuFields.heightfield)
                < MaximumNeighbourDrop(cpuFields.heightfield) + parityTolerance,
                "the Gpu relaxed the field, it did not merely echo the input");
}

// The material-transport toggle (Constitution §8) must mean the same thing on both backends:
// heights still relax, masks are left exactly as they were found.
void CheckTransportDisabled(Sys::GpuResourceManager& resourceManager) {
    Data::MapFields cpuFields, gpuFields, untouchedFields;
    BuildRoughField(untouchedFields, parityVertexSize);
    RunBothBackends(resourceManager, false, cpuFields, gpuFields);
    CheckParity(MaximumFieldDifference(cpuFields.heightfield, gpuFields.heightfield) <= parityTolerance,
                "transport disabled: Cpu and Gpu heightfields still agree");
    float leftoverDifference = 0.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const Data::FloatField& original = untouchedFields.materialMasks[stratum];
        leftoverDifference += MaximumFieldDifference(cpuFields.materialMasks[stratum], original);
        leftoverDifference += MaximumFieldDifference(gpuFields.materialMasks[stratum], original);
    }
    CheckParity(leftoverDifference == 0.0f,
                "transport disabled: material masks are untouched on both backends");
}

} // namespace

int RunThermalGpuParityChecks(const char* shaderDirectory) {
    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) return -1;

    Sys::GpuResourceManager resourceManager(shaderDirectory);
    Data::MapFields cpuFields, gpuFields;
    RunBothBackends(resourceManager, true, cpuFields, gpuFields);
    CompareBackendResults(cpuFields, gpuFields);
    CheckTransportDisabled(resourceManager);
    CheckParity(resourceManager.CompileCount() == 1, "the kernel compiled exactly once across all runs");

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);
    return parityFailures;
}

} // namespace ThermalTest
} // namespace SanmapGen
