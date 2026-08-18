// Mask_Parity_PROC_Test.cpp — the CPU/GPU parity half of the M3-2 acceptance test. Runs both
// backends from the SAME inputs over a settings mix that exercises every branch (hard clamp,
// smoothstep+feather, invert, partial strength, all three merge modes, stored art at a different
// resolution, a distinct per-stratum feather shape). Needs a real GL context, so it spins up a
// hidden-window WGL one (test harness only, never app code).
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
std::vector<Params::Stratum> MakeParitySettings() {
    std::vector<Params::Stratum> strata(Data::MapFields::stratumCount);
    for (int index = 0; index < Data::MapFields::stratumCount; ++index) {
        Params::Stratum& stratum = strata[index];
        stratum.bSlopeUseGlobal = false;   // exercise every stratum's OWN window, not slopeDefaults
        stratum.bSlopeGateEnabled = index != 0;
        stratum.minimumSlopeDegrees = 5.0f * static_cast<float>(index);
        stratum.maximumSlopeDegrees = 20.0f + 6.0f * static_cast<float>(index);
        stratum.bUseSmoothstep = (index % 2) == 1;
        stratum.slopeFeatherDegreesLow = 3.0f + static_cast<float>(index);
        stratum.slopeFeatherDegreesHigh = 2.0f + 0.5f * static_cast<float>(index);
        stratum.bInvertSlopeGate = (index % 3) == 2;
        stratum.slopeGateStrength = 0.25f + 0.09f * static_cast<float>(index);
    }
    strata[2].importedMaskMode = Params::ImportedMaskMode::ProceduralStart;
    strata[5].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    strata[7].slopeFeatherDegreesLow  = 11.0f;   // a distinct feather shape as strata[7]'s
    strata[7].slopeFeatherDegreesHigh = 9.0f;    // non-identity differentiator
    return strata;
}

// The loaded art for the two merging strata, at a resolution deliberately NOT the map
// resolution, so the one bilinear resampler has to stretch it on both backends.
std::vector<Data::StratumArt> MakeParityArt() {
    std::vector<Data::StratumArt> stratumArt = NoStratumArt();
    const int side = 37;
    for (int index : { 2, 5 }) {
        std::vector<float> pixels(static_cast<std::size_t>(side) * side);
        for (int pixel = 0; pixel < side * side; ++pixel)
            pixels[pixel] = static_cast<float>((pixel * 17 + index * 5) % 101) * 0.0099f;
        SetImportedMask(stratumArt[index], pixels.data(), side, side);
    }
    return stratumArt;
}

void BuildInputs(const Params::Geometry& geometry, Data::MapFields& fields) {
    fields.Resize(geometry.VertexSize());
    FillTestHeightfield(fields, geometry.VertexSize());
    FillTestMaterialProportions(fields, geometry.VertexSize());
}

float LargestWeightDifference(const Data::MapFields& first, const Data::MapFields& second, int vertexSize) {
    float largestDifference = 0.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        for (int y = 0; y < vertexSize; ++y)
            for (int x = 0; x < vertexSize; ++x) {
                const float difference = std::fabs(first.surfaceStratumWeights[stratum].Get(x, y)
                                                 - second.surfaceStratumWeights[stratum].Get(x, y));
                largestDifference = difference > largestDifference ? difference : largestDifference;
            }
    return largestDifference;
}

} // namespace

void RunParityTests(const char* shaderDirectory) {
    Params::Geometry geometry;
    geometry.mapSize = kMapSize;
    const std::vector<Params::Stratum> strata = MakeParitySettings();
    const std::vector<Data::StratumArt> stratumArt = MakeParityArt();
    const Params::SlopeDefaults slopeDefaults;

    Data::MapFields cpuFields, gpuFields, untouchedFields;
    BuildInputs(geometry, cpuFields);
    BuildInputs(geometry, gpuFields);
    BuildInputs(geometry, untouchedFields);

    Proc::MaskStage cpuStage(geometry, strata, stratumArt, cpuFields, slopeDefaults);
    cpuStage.RunOnCpu();
    CheckWeightsAreResolved(cpuFields, geometry.VertexSize());
    CheckProportionsUntouched(cpuFields, untouchedFields,
                              "the Cpu path leaves materialProportions untouched");

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available — CPU/GPU parity not verified here\n");
        return;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    Check(manager.Initialize(), "GPU resource manager initializes");

    Proc::MaskStage gpuStage(geometry, strata, stratumArt, gpuFields, slopeDefaults);
    gpuStage.SetGpuResourceManager(&manager);
    gpuStage.RunOnGpu();
    Check(gpuStage.IsGpuAvailable(), "mask compute program compiled from its three GLSL units");
    Check(gpuStage.LastBackend() == Sys::ComputeBackend::Gpu, "the GPU path actually ran (no silent fallback)");

    const float largestDifference = LargestWeightDifference(cpuFields, gpuFields, geometry.VertexSize());
    std::printf("CPU/GPU largest surface-weight difference: %.9f (Visual tolerance %.6f)\n",
                largestDifference, kVisualTolerance);
    Check(largestDifference <= kVisualTolerance, "CPU and GPU agree within the Visual tolerance");
    float largestSlopeDifference = 0.0f;                       // the second output, M5-0c
    for (int y = 0; y < geometry.VertexSize(); ++y)
        for (int x = 0; x < geometry.VertexSize(); ++x) {
            const float difference = std::fabs(cpuFields.slope.Get(x, y) - gpuFields.slope.Get(x, y));
            largestSlopeDifference = difference > largestSlopeDifference ? difference : largestSlopeDifference;
        }
    Check(largestSlopeDifference <= kVisualTolerance, "CPU and GPU bake the same slope field");
    CheckProportionsUntouched(gpuFields, untouchedFields,
                              "the Gpu path leaves materialProportions untouched");

    // The program is compiled once, not per dispatch (DISPATCH_INTERFACE_SPEC §3).
    const int compileCountAfterFirstRun = manager.CompileCount();
    gpuStage.RunOnGpu();
    Check(manager.CompileCount() == compileCountAfterFirstRun, "the mask program is compiled exactly once");

    wglMakeCurrent(nullptr, nullptr); wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext); DestroyWindow(window);
}

} // namespace MaskTest
} // namespace SanmapGen
