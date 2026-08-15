// Placement_Gpu_PROC_Test.cpp — Cpu/Gpu parity for the placement gate (ARCH §6.1: a stage is
// done only when BOTH backends are implemented and parity-checked). The Gpu preview gate must
// produce the SAME placement as the authoritative Cpu bake — the gate expression is binary
// (pass/reject) for these rules, so parity here is exact, not merely in-tolerance.
// Needs a real GL context, so the test spins up a hidden-window WGL context (test harness,
// not app code) and exits 2 (SKIP) where no context exists. Build from src/proc:
//   cl /EHsc /std:c++17 /O2 Placement_Gpu_PROC_Test.cpp <the Placement_*_PROC.cpp set>
//      ..\sys\GpuResource_*.cpp ..\sys\GpuGlFunctions_SYS.cpp opengl32.lib gdi32.lib user32.lib
// argv[1] = directory holding Placement_PROC.glsl (defaults to the current directory).
#include "Placement_PROC.h"
#include "Placement_Test_Terrain.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace SanmapGen;

static int failures = 0;
static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failures; }
}

static bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "PlacementGpuTestWindow";
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

static Params::MapRecipe MakeParityRecipe() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = PlacementTest::mapSize;
    recipe.geometry.seed = 20250814u;
    recipe.geometry.terrainMaxHeight = 128.0f;
    recipe.water.bEnabled = true;
    recipe.water.waterLevelMaximum = 32.0f;          // exercises the water gate on both backends

    Params::MarkerRule markerRule;
    markerRule.category = Params::MarkerCategory::Alloys;
    markerRule.count = 8;
    markerRule.clearanceSpacing = 14.0f;
    markerRule.mapEdgePadding = 6;
    markerRule.minHeight = 0.4f; markerRule.maxHeight = 0.6f;
    markerRule.maxSlope = 12.0f;
    markerRule.bRandomSelection = true;
    markerRule.transform = PlacementTest::MakeTransform("alloy01", 1.0f, 1.0f);
    recipe.markerRules.push_back(markerRule);

    Params::PropRule propRule;
    propRule.density = 0.4f;
    propRule.spacingMinimum = 5.0f;
    propRule.mapEdgePadding = 4;
    propRule.minHeight = 0.4f; propRule.maxHeight = 0.9f;
    propRule.maxSlope = 45.0f;
    propRule.bAvoidWater = true;
    propRule.maskStratumIndex = PlacementTest::maskStratumIndex;
    propRule.maskWeightMinimum = 0.5f;
    propRule.transform = PlacementTest::MakeTransform("edbm014", 0.5f, 2.0f);
    recipe.propRules.push_back(propRule);
    return recipe;
}

static bool InstancesEqual(const Data::PlacementInstances& first, const Data::PlacementInstances& second) {
    if (first.Count() != second.Count()) return false;
    for (std::size_t index = 0; index < first.Count(); ++index)
        if (first.positionX[index] != second.positionX[index]
            || first.positionZ[index] != second.positionZ[index]
            || first.positionY[index] != second.positionY[index]) return false;
    return true;
}

int main(int argc, char** argv) {
    const std::string shaderDirectory = (argc > 1) ? argv[1] : ".";
    Data::MapFields fields;
    PlacementTest::BuildTestFields(fields);
    const Data::FloatField bakedSlope = fields.slope;      // Mask owns it; Placement only reads
    const Params::MapRecipe recipe = MakeParityRecipe();

    Data::PlacementResults cpuResults;
    Proc::PlacementStage cpuStage(recipe, fields, cpuResults);
    cpuStage.RunOnCpu();

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available in this environment\n");
        return 2;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    if (!manager.Initialize()) { std::printf("SKIP: GL compute entry points unavailable\n"); return 2; }

    Data::PlacementResults gpuResults;
    Proc::PlacementStage gpuStage(recipe, fields, gpuResults);
    gpuStage.SetGpuResourceManager(&manager);
    gpuStage.RunOnGpu();

    std::printf("cpu markers=%zu props=%zu | gpu markers=%zu props=%zu | gpuGate=%d\n",
                cpuResults.markers.Count(), cpuResults.props.Count(),
                gpuResults.markers.Count(), gpuResults.props.Count(),
                gpuStage.WasGpuGateUsed() ? 1 : 0);
    Check(gpuStage.IsGpuAvailable(), "Placement_PROC.glsl compiles");
    Check(gpuStage.WasGpuGateUsed(), "the Gpu density gate actually ran");
    Check(cpuResults.markers.Count() > 0 && cpuResults.props.Count() > 0, "the Cpu run placed instances");
    Check(InstancesEqual(cpuResults.markers, gpuResults.markers), "Cpu/Gpu parity: markers identical");
    Check(InstancesEqual(cpuResults.props, gpuResults.props), "Cpu/Gpu parity: props identical");
    Check(manager.CompileCount() == 1, "the gate program compiles exactly once");
    // The gate really consumes the bake: a scene whose slope field was never written would gate
    // as flat ground and quietly accept the cone, so the input is asserted non-trivial first.
    float largestSlope = 0.0f;
    for (std::size_t cell = 0; cell < bakedSlope.CellCount(); ++cell)
        largestSlope = bakedSlope.Data()[cell] > largestSlope ? bakedSlope.Data()[cell] : largestSlope;
    Check(largestSlope > 1.0f, "the scene's baked slope field is non-trivial (the cone is steep)");
    // Single writer (ARCH §3.4.1, M5-0c): neither backend of Placement may touch MapFields.slope.
    Check(fields.slope.CellCount() == bakedSlope.CellCount()
          && std::memcmp(fields.slope.Data(), bakedSlope.Data(),
                         bakedSlope.CellCount() * sizeof(float)) == 0,
          "Placement leaves the baked slope field byte-identical");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
