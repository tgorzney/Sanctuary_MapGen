// Mask_Parity_PROC_Test.cpp — the CPU/GPU parity half of the M3-2 acceptance test. Runs both
// backends from the SAME inputs over a settings mix that exercises every branch (hard clamp,
// smoothstep+feather, invert, partial strength, all three merge modes, stored art at a different
// resolution, a non-identity remap). Needs a real GL context, so it spins up a hidden-window WGL
// one (test harness only, never app code).
#include "Mask_TestSupport_PROC.h"
#include "Mask_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <vector>

namespace SanmapGen {
namespace MaskTest {
namespace {

constexpr int kMapSize = 128;
constexpr float kVisualTolerance = 1.0e-4f;

bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "MaskParityTestWindow";
    RegisterClassA(&windowClass);
    outWindow = CreateWindowExA(0, windowClass.lpszClassName, "hidden", 0, 0, 0, 8, 8,
                                nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!outWindow) return false;
    outDeviceContext = GetDC(outWindow);
    PIXELFORMATDESCRIPTOR descriptor = {};
    descriptor.nSize = sizeof(descriptor); descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;  descriptor.cColorBits = 32;
    int pixelFormat = ChoosePixelFormat(outDeviceContext, &descriptor);
    if (!pixelFormat || !SetPixelFormat(outDeviceContext, pixelFormat, &descriptor)) return false;
    outGlContext = wglCreateContext(outDeviceContext);
    return outGlContext && wglMakeCurrent(outDeviceContext, outGlContext);
}

// One setting combination per stratum, so a single run covers every branch of the kernel.
std::vector<Params::StratumMask> MakeParitySettings() {
    std::vector<Params::StratumMask> stratumMasks(Data::MapFields::stratumCount);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        Params::StratumMask& stratumMask = stratumMasks[stratum];
        stratumMask.bSlopeGateEnabled = stratum != 0;
        stratumMask.minimumSlopeDegrees = 5.0f * static_cast<float>(stratum);
        stratumMask.maximumSlopeDegrees = 20.0f + 6.0f * static_cast<float>(stratum);
        stratumMask.bUseSmoothstep = (stratum % 2) == 1;
        stratumMask.slopeFeatherDegreesLow = 3.0f + static_cast<float>(stratum);
        stratumMask.slopeFeatherDegreesHigh = 2.0f + 0.5f * static_cast<float>(stratum);
        stratumMask.bInvertSlopeGate = (stratum % 3) == 2;
        stratumMask.slopeGateStrength = 0.25f + 0.09f * static_cast<float>(stratum);
    }
    stratumMasks[2].importedMaskMode = Params::ImportedMaskMode::ProceduralStart;
    stratumMasks[5].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    const int side = 37;   // deliberately not the map resolution: the resampler must stretch
    for (int stratum : { 2, 5 }) {
        stratumMasks[stratum].importedMaskWidth = side;
        stratumMasks[stratum].importedMaskHeight = side;
        stratumMasks[stratum].importedMaskData.resize(static_cast<std::size_t>(side) * side);
        for (int index = 0; index < side * side; ++index)
            stratumMasks[stratum].importedMaskData[index] =
                static_cast<float>((index * 17 + stratum * 5) % 101) * 0.0099f;
    }
    stratumMasks[7].maskRemapMinimum = 0.1f;
    stratumMasks[7].maskRemapMaximum = 0.8f;
    return stratumMasks;
}

void BuildInputs(const Params::Geometry& geometry, Data::MapFields& fields) {
    fields.Resize(geometry.VertexSize());
    FillTestHeightfield(fields, geometry.VertexSize());
    FillTestProceduralMasks(fields, geometry.VertexSize());
}

float LargestFieldDifference(const Data::MapFields& first, const Data::MapFields& second, int vertexSize) {
    float largestDifference = 0.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        for (int y = 0; y < vertexSize; ++y)
            for (int x = 0; x < vertexSize; ++x) {
                const float difference = std::fabs(first.materialMasks[stratum].Get(x, y)
                                                 - second.materialMasks[stratum].Get(x, y));
                largestDifference = difference > largestDifference ? difference : largestDifference;
            }
    return largestDifference;
}

// Guards against a trivially-equal comparison: the stage must really have rewritten the field,
// and the result must still span a range of weights instead of collapsing to a constant.
void CheckStageDidRealWork(const Data::MapFields& processed, const Data::MapFields& untouched, int vertexSize) {
    int changedCellCount = 0;
    float smallestWeight = 2.0f, largestWeight = -1.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        for (int y = 0; y < vertexSize; ++y)
            for (int x = 0; x < vertexSize; ++x) {
                const float weight = processed.materialMasks[stratum].Get(x, y);
                if (std::fabs(weight - untouched.materialMasks[stratum].Get(x, y)) > 1e-6f) ++changedCellCount;
                if (weight < smallestWeight) smallestWeight = weight;
                if (weight > largestWeight) largestWeight = weight;
            }
    Check(changedCellCount > 1000, "the stage actually rewrote the mask field (parity is not trivial)");
    Check(largestWeight - smallestWeight > 0.5f, "the masked field still spans a range of weights");
}

} // namespace

void RunParityTests(const char* shaderDirectory) {
    Params::Geometry geometry;
    geometry.mapSize = kMapSize;
    const std::vector<Params::StratumMask> stratumMasks = MakeParitySettings();

    Data::MapFields cpuFields, gpuFields;
    BuildInputs(geometry, cpuFields);
    BuildInputs(geometry, gpuFields);

    Proc::MaskStage cpuStage(geometry, stratumMasks, cpuFields);
    cpuStage.RunOnCpu();
    CheckStageDidRealWork(cpuFields, gpuFields, geometry.VertexSize());   // gpuFields = untouched input

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available — CPU/GPU parity not verified here\n");
        return;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    Check(manager.Initialize(), "GPU resource manager initializes");

    Proc::MaskStage gpuStage(geometry, stratumMasks, gpuFields);
    gpuStage.SetGpuResourceManager(&manager);
    gpuStage.RunOnGpu();
    Check(gpuStage.IsGpuAvailable(), "mask compute program compiled from its three GLSL units");
    Check(gpuStage.LastBackend() == Sys::ComputeBackend::Gpu, "the GPU path actually ran (no silent fallback)");

    const float largestDifference = LargestFieldDifference(cpuFields, gpuFields, geometry.VertexSize());
    std::printf("CPU/GPU largest mask difference: %.9f (Visual tolerance %.6f)\n",
                largestDifference, kVisualTolerance);
    Check(largestDifference <= kVisualTolerance, "CPU and GPU agree within the Visual tolerance");

    // The program is compiled once, not per dispatch (DISPATCH_INTERFACE_SPEC §3).
    const int compileCountAfterFirstRun = manager.CompileCount();
    gpuStage.RunOnGpu();
    Check(manager.CompileCount() == compileCountAfterFirstRun, "the mask program is compiled exactly once");

    wglMakeCurrent(nullptr, nullptr); wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext); DestroyWindow(window);
}

} // namespace MaskTest
} // namespace SanmapGen
