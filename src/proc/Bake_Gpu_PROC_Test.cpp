// Bake_Gpu_PROC_Test.cpp — the Gpu half of the bake acceptance test: the stage's DEFAULT
// policy must resolve to the Gpu path (ARCH §4.2 "Bake = Gpu/Visual in both contexts"), the
// Gpu composite must reproduce the hand-checked blend, and it must agree with the Cpu twin
// inside the Visual-class tolerance declared below. Needs a real GL context, so it spins up
// a hidden-window WGL context (test harness, not app code) exactly like GpuResource_SYS_Test.
#include "Bake_TestScene_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

namespace {

using Proc::AllTexelsEqual;
using Proc::BakeSceneInputs;
using Proc::BuildTwoStratumScene;
using Proc::CpuVisualPolicy;
using Proc::ExpectedTexel;
using Proc::HasVariedTexels;
void check(bool bCondition, const char* label) { Proc::CheckBakeExpectation(bCondition, label); }

// Visual-class parity tolerance: one quantization step (1/255) per channel. The two backends
// run the same expressions in the same order, so only float rounding may differ.
constexpr int visualParityToleranceBytes = 1;

bool TexelsWithinTolerance(const std::vector<unsigned int>& left, const std::vector<unsigned int>& right) {
    if (left.size() != right.size() || left.empty()) return false;
    for (std::size_t index = 0; index < left.size(); ++index)
        for (int channel = 0; channel < 4; ++channel) {
            const int leftChannel  = static_cast<int>((left[index] >> (channel * 8)) & 0xFFu);
            const int rightChannel = static_cast<int>((right[index] >> (channel * 8)) & 0xFFu);
            const int difference = leftChannel > rightChannel ? leftChannel - rightChannel
                                                              : rightChannel - leftChannel;
            if (difference > visualParityToleranceBytes) return false;
        }
    return true;
}

bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "BakeStageTestWindow";
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

// The same two-stratum scene as the Cpu test, plus a 2x2 tiled texture on stratum 1, baked
// at 4x the map size so the surface weights are bilinearly upsampled (the real bake path).
void BuildGpuScene(Proc::BakeStage& stage, Params::Geometry& geometry, Data::MapFields& fields,
                   BakeSceneInputs& scene, const unsigned int* texels) {
    BuildTwoStratumScene(geometry, fields, scene, stage);
    stage.Constants().outputResolutionMultiplier = 4;
    scene.stratumArt[1].albedoTexels = texels;
    scene.stratumArt[1].albedoWidth = 2;
    scene.stratumArt[1].albedoHeight = 2;
    scene.strata[1].tileCount = 3.0f;
    scene.strata[1].tintRed = 1.0f; scene.strata[1].tintGreen = 1.0f; scene.strata[1].tintBlue = 1.0f;
}

// The textured scene on both backends: the Gpu path must be chosen by the DEFAULT policy and
// must land within the Visual tolerance of the Cpu twin.
void CheckGpuPathAndParity(Sys::GpuResourceManager& manager, const unsigned int* checkerTexels) {
    Params::Geometry geometry; geometry.mapSize = 16;
    Data::MapFields gpuFields, cpuFields;
    BakeSceneInputs gpuScene, cpuScene;
    Proc::BakedTextureSet gpuTextures, cpuTextures;
    Proc::BakeStage gpuStage(geometry, gpuScene.strata, gpuScene.stratumArt, gpuFields, gpuTextures);
    Proc::BakeStage cpuStage(geometry, cpuScene.strata, cpuScene.stratumArt, cpuFields, cpuTextures);
    BuildGpuScene(gpuStage, geometry, gpuFields, gpuScene, checkerTexels);
    BuildGpuScene(cpuStage, geometry, cpuFields, cpuScene, checkerTexels);
    gpuStage.SetGpuResourceManager(&manager);          // policy left at the ARCH §4.2 default
    cpuStage.SetDispatchPolicy(CpuVisualPolicy());

    const Sys::ComputeBackend gpuBackend = gpuStage.Run();
    cpuStage.Run();
    check(gpuBackend == Sys::ComputeBackend::Gpu, "the default bake policy runs on the Gpu path");
    check(gpuStage.IsGpuAvailable(), "the bake compute program compiled");
    check(gpuTextures.resolution == 64, "Gpu bake sized 4x the map");
    check(HasVariedTexels(gpuTextures.compositeAlbedo),
          "the tiled checker texture makes a varying composite (so parity is not a trivial match)");
    check(TexelsWithinTolerance(gpuTextures.compositeAlbedo, cpuTextures.compositeAlbedo),
          "Gpu composite matches the Cpu twin within the Visual tolerance (1/255 per channel)");
    check(TexelsWithinTolerance(gpuTextures.stratumMaskLow, cpuTextures.stratumMaskLow),
          "Gpu packed stratum masks match the Cpu twin");
    // The std430 record was re-padded to 48 bytes after the two remap floats were deleted; a
    // stride mismatch would scramble every stratum past the first, so this parity result is
    // also the layout check (DISPATCH_INTERFACE_SPEC §4).
    check(sizeof(Proc::StratumKernelConfiguration) % 16 == 0,
          "StratumKernelConfiguration is a 16-byte multiple (std430 array stride)");
}

// The hand-checked flat-tint blend, on the Gpu: 0.25 * red + 0.75 * blue.
void CheckGpuFlatBlend(Sys::GpuResourceManager& manager) {
    Params::Geometry geometry; geometry.mapSize = 16;
    Data::MapFields fields;
    BakeSceneInputs scene;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, scene.strata, scene.stratumArt, fields, textures);
    BuildTwoStratumScene(geometry, fields, scene, stage);
    stage.SetGpuResourceManager(&manager);
    check(stage.Run() == Sys::ComputeBackend::Gpu, "flat-tint bake also runs on the Gpu");
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(64, 0, 191, 255)),
          "Gpu two-stratum blend matches the expected weighted colour");
    check(manager.CompileCount() == 1, "the bake program compiled exactly once across all runs");
}

} // namespace

// Returns 0 when the Gpu checks ran, 2 when no GL context exists in this environment.
int RunBakeGpuAcceptance(const std::string& shaderDirectory) {
    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) return 2;

    const unsigned int checkerTexels[4] = { ExpectedTexel(0, 255, 0, 255), ExpectedTexel(0, 0, 255, 255),
                                            ExpectedTexel(255, 255, 0, 255), ExpectedTexel(0, 255, 255, 255) };
    Sys::GpuResourceManager manager(shaderDirectory);
    check(manager.Initialize(), "Gpu resource manager initializes");
    CheckGpuPathAndParity(manager, checkerTexels);
    CheckGpuFlatBlend(manager);

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);
    return 0;
}
