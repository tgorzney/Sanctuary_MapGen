// PreviewComposite_Gpu_UI_Test.cpp — acceptance test, part 3: the Gpu path. The composite's
// default backend is the Gpu (ARCH §4.2 "preview color = Gpu / Visual"), it must reach it only
// through Sys::GpuResourceManager, and it must reproduce the Cpu twin's image inside the
// Visual-class tolerance. Needs a real GL context, so it spins up a hidden-window WGL context
// (test harness, not app code) exactly like Bake_Gpu_PROC_Test / GpuResource_SYS_Test.
// argv[1] = the directory holding the PreviewComposite *.glsl units (defaults to ".").
// Returns 2 (and skips) when this machine has no GL context.
#include "PreviewComposite_TestScene_UI.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

namespace {

void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

// One quantization step per channel: both backends run the same expressions in the same order,
// so only float rounding may differ.
constexpr int visualParityToleranceBytes = 1;

bool TexelsWithinTolerance(const std::vector<unsigned int>& left, const std::vector<unsigned int>& right,
                           int toleranceBytes = visualParityToleranceBytes) {
    if (left.size() != right.size() || left.empty()) return false;
    for (std::size_t index = 0; index < left.size(); ++index)
        for (int channel = 0; channel < 4; ++channel) {
            const int difference = Ui::ChannelByte(left[index], channel)
                                 - Ui::ChannelByte(right[index], channel);
            if (difference > toleranceBytes || difference < -toleranceBytes) return false;
        }
    return true;
}

// STEP200: Overlay/Screen/SoftLight/HardLight all multiply a per-channel term by a coefficient of
// 2 (e.g. Overlay's `2*d*s`), so an ordinary sub-1-byte Cpu/Gpu float rounding difference already
// tolerated everywhere else (in `source`, from upstream Lut sampling) can legitimately double to
// ~2 bytes once it passes through one of these formulas — measured directly against this test's
// own varied (worst-case, full-gradient) scene, never more than 2. Every other blend mode (no
// coefficient above 1) keeps the strict 1-byte `visualParityToleranceBytes` every other assertion
// in this file already uses.
bool BlendModeHasAmplifiedGain(Ui::PreviewBlendMode blendMode) {
    return blendMode == Ui::PreviewBlendMode::Overlay || blendMode == Ui::PreviewBlendMode::Screen
        || blendMode == Ui::PreviewBlendMode::SoftLight || blendMode == Ui::PreviewBlendMode::HardLight;
}

// Divide is worse than a fixed x2 gain: CombineChannel divides by `source`, so an ordinary sub-1-
// byte Cpu/Gpu float difference IN `source` gets amplified by 1/source, unboundedly for a small
// source — PreviewComposite_Color_UI.h's own STEP200 fix already clamps the truly-unbounded case
// (result > 1.0) to a deterministic 1.0 on both backends, but a source small enough to amplify the
// baseline noise while the quotient STAYS under 1.0 is still inherently noisier than every other
// blend mode. Measured directly against this test's own varied scene: never more than 5 (confirmed
// empirically — clamping the >1.0 case first, then re-measuring, showed this 5-byte case sits
// entirely below the clamp, i.e. it is the division's own amplification, not the unbounded blowup).
constexpr int kDivideToleranceBytes = 6;

bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "PreviewCompositeTestWindow";
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

// A varied bake, so parity is not a trivial match of one flat color: a height gradient across
// the map, a second stratum, and two resolved instances.
void BuildVariedScene(Ui::PreviewTestScene& scene) {
    Ui::BuildPreviewTestScene(scene);
    for (int cellY = 0; cellY < scene.geometry.VertexSize(); ++cellY)
        for (int cellX = 0; cellX < scene.geometry.VertexSize(); ++cellX) {
            scene.fields.heightfield.Set(cellX, cellY, static_cast<float>(cellX + cellY) * 0.1f);
            scene.fields.flow.Set(cellX, cellY, static_cast<float>(cellX) * 0.3f);
            scene.fields.surfaceStratumWeights[0].Set(cellX, cellY, static_cast<float>(cellX) * 0.2f);
            scene.fields.surfaceStratumWeights[1].Set(cellX, cellY, static_cast<float>(cellY) * 0.2f);
        }
    scene.strata[1].bEnabled = true;
    scene.strata[1].tintRed = 0.1f; scene.strata[1].tintGreen = 0.9f; scene.strata[1].tintBlue = 0.3f;
    Data::PlacementInstance second;
    second.positionX = 1.0f; second.positionZ = 3.0f;
    scene.instances.Append(second);
    scene.water.bEnabled = true;
    scene.water.waterLevelMaximum = 30.0f; scene.water.deepWaterDepthMaximum = 30.0f;
}

void ConfigureVariedSettings(Ui::PreviewCompositeSettings& settings) {
    Ui::ConfigurePreviewSettings(settings);
    settings.previewResolution = 32;                       // upsamples the 5x5 bake, the real path
    settings.gradientRamps.push_back(Ui::MakeConstantRamp(0.0f, 0.2f, 1.0f, 0.8f));
    settings.fieldLayers.push_back(
        Ui::MakeLayer(Ui::PreviewLayerKind::Water, Ui::PreviewBlendMode::AlphaBlend, 2, 0.0f, 1.0f));
    settings.entityMarkRadiusPixels = 2.5f;
}

bool HasVariedTexels(const std::vector<unsigned int>& texels) {
    for (unsigned int texel : texels)
        if (texel != texels[0]) return true;
    return false;
}

void CheckGpuPathAndParity(Sys::GpuResourceManager& manager) {
    Ui::PreviewTestScene gpuScene, cpuScene;
    BuildVariedScene(gpuScene);
    BuildVariedScene(cpuScene);
    Ui::PreviewComposite gpuComposite(gpuScene.geometry, gpuScene.water, gpuScene.strata, gpuScene.areas,
                                      gpuScene.fields, gpuScene.instances, gpuScene.entityIdentifiers);
    Ui::PreviewComposite cpuComposite(cpuScene.geometry, cpuScene.water, cpuScene.strata, cpuScene.areas,
                                      cpuScene.fields, cpuScene.instances, cpuScene.entityIdentifiers);
    ConfigureVariedSettings(gpuComposite.Settings());
    ConfigureVariedSettings(cpuComposite.Settings());
    gpuComposite.SetGpuResourceManager(&manager);          // the only route to GL (ARCH §3.2)

    gpuComposite.Compose();
    cpuComposite.ComposeOnCpu();
    check(gpuComposite.LastRunUsedGpu(), "the composite ran on the Gpu through GpuResource_SYS");
    check(HasVariedTexels(gpuComposite.CompositeTexels()),
          "the varied bake composites to a varied image (so parity is not a trivial match)");
    check(TexelsWithinTolerance(gpuComposite.CompositeTexels(), cpuComposite.CompositeTexels()),
          "the Gpu composite matches the Cpu twin within the Visual tolerance (1/255 per channel)");
    check(gpuComposite.ExecutedPassCount() == cpuComposite.ExecutedPassCount(),
          "both backends run the same pass sequence");
    bool bIdentifiersMatch = gpuScene.entityIdentifiers.CellCount()
                          == cpuScene.entityIdentifiers.CellCount();
    for (std::size_t cell = 0; bIdentifiersMatch && cell < gpuScene.entityIdentifiers.CellCount(); ++cell)
        bIdentifiersMatch = gpuScene.entityIdentifiers.Data()[cell]
                         == cpuScene.entityIdentifiers.Data()[cell];
    check(bIdentifiersMatch, "the Gpu entity-id buffer matches the Cpu twin exactly");

    gpuComposite.Compose();                                // a second run must not recompile
    check(manager.CompileCount() == 1, "the composite program compiled exactly once");
    check(sizeof(Ui::PreviewCompositeConfiguration) % 16 == 0
       && sizeof(Ui::PreviewLayerConfiguration) % 16 == 0
       && sizeof(Ui::PreviewStratumConfiguration) % 16 == 0
       && sizeof(Ui::PreviewEntityPoint) % 16 == 0, "every kernel record is a 16-byte multiple");
}

// STEP200: all 12 PreviewBlendMode enumerators — including the six v1-parity additions
// (Subtract..HardLight) — composite the same on the Gpu as the Cpu, per representative pixel. The
// varied scene's Water layer (ConfigureVariedSettings) is the one swept across every mode; its
// destination is the varied height ramp underneath, so every mode sees a real, non-flat
// (destination, source) pair, not a degenerate 0/1 case.
void CheckAllBlendModesParity(Sys::GpuResourceManager& manager) {
    for (int modeIndex = 0; modeIndex < Ui::kPreviewBlendModeCount; ++modeIndex) {
        Ui::PreviewTestScene gpuScene, cpuScene;
        BuildVariedScene(gpuScene);
        BuildVariedScene(cpuScene);
        Ui::PreviewComposite gpuComposite(gpuScene.geometry, gpuScene.water, gpuScene.strata, gpuScene.areas,
                                          gpuScene.fields, gpuScene.instances, gpuScene.entityIdentifiers);
        Ui::PreviewComposite cpuComposite(cpuScene.geometry, cpuScene.water, cpuScene.strata, cpuScene.areas,
                                          cpuScene.fields, cpuScene.instances, cpuScene.entityIdentifiers);
        ConfigureVariedSettings(gpuComposite.Settings());
        ConfigureVariedSettings(cpuComposite.Settings());
        const Ui::PreviewBlendMode blendMode = static_cast<Ui::PreviewBlendMode>(modeIndex);
        gpuComposite.Settings().fieldLayers.back().blendMode = blendMode;   // the Water layer
        cpuComposite.Settings().fieldLayers.back().blendMode = blendMode;
        gpuComposite.SetGpuResourceManager(&manager);

        gpuComposite.Compose();
        cpuComposite.ComposeOnCpu();
        char label[96];
        std::snprintf(label, sizeof(label),
                      "blend mode %d composites identically on Gpu and Cpu (within tolerance)", modeIndex);
        int toleranceBytes = visualParityToleranceBytes;
        if (blendMode == Ui::PreviewBlendMode::Divide) toleranceBytes = kDivideToleranceBytes;
        else if (BlendModeHasAmplifiedGain(blendMode)) toleranceBytes = 2 * visualParityToleranceBytes;
        check(TexelsWithinTolerance(gpuComposite.CompositeTexels(), cpuComposite.CompositeTexels(), toleranceBytes),
              label);
    }
}

} // namespace

int main(int argumentCount, char** argumentValues) {
    const std::string shaderDirectory = argumentCount > 1 ? argumentValues[1] : ".";
    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("GPU SKIPPED (no GL context)\n");
        return 2;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    check(manager.Initialize(), "the Gpu resource manager initializes");
    CheckGpuPathAndParity(manager);
    CheckAllBlendModesParity(manager);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
