// PreviewComposite_GpuBenchmark_UI_Test.cpp — ARCH §14.18 item 10's GPU benchmark harness. Produces
// per-phase (bind+dispatch issue-side / fence-wait / entity-id-readback) min/avg/max timings across
// {256, 1024} mapSize x {0, 100000} instanceCount, using ComposeOnGpu's new optional
// ComposeGpuTiming* parameter (STEP218). This is a MEASUREMENT TOOL, not an acceptance gate: every
// check() below is a loose sanity check (a duration is non-negative and finite) — none of them
// assert a threshold, because no threshold is ratified yet. The printed numbers are the actual
// deliverable, for a human/the SanGen Compute Optimization Expert to review before ARCH §14.18's
// Piece C (the drag-suppression-vs-gated-recompose ruling) can be authored.
// Needs a real GL context, so it spins up a hidden-window WGL context exactly like
// PreviewComposite_Gpu_UI_Test.cpp / Bake_Gpu_PROC_Test / GpuResource_SYS_Test.
// argv[1] = the directory holding the PreviewComposite *.glsl units (defaults to ".").
// Returns 2 (and skips) when this machine has no GL context.
#include "PreviewComposite_TestScene_UI.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/GpuGlFunctions_SYS.h"
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

using namespace SanmapGen;

namespace {

void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

bool CreateHiddenGlContext(HWND& outWindow, HDC& outDeviceContext, HGLRC& outGlContext) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = GetModuleHandleA(nullptr);
    windowClass.lpszClassName = "PreviewCompositeBenchmarkTestWindow";
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

constexpr int kBenchmarkIterationCount     = 50;
constexpr int kBenchmarkPreviewResolution  = 512;

struct BenchmarkConfiguration { int mapSize; int instanceCount; };

// mapSize x instanceCount, exactly the 4 configurations item 10 names.
constexpr BenchmarkConfiguration kBenchmarkConfigurations[] = {
    { 256, 0 }, { 256, 100000 }, { 1024, 0 }, { 1024, 100000 },
};

// Fills heightfield/flow/accumulation/slope and all 9 surfaceStratumWeights fields with varied,
// non-degenerate data (distinct modulo patterns per field so no two fields alias each other), and
// spreads `instanceCount` placement instances across the full map extent (a raster scan over the
// vertex grid, not a single repeated point) — so upload byte counts and entity-pass thread counts
// are realistic, not degenerate-case artifacts.
void BuildBenchmarkScene(Ui::PreviewTestScene& scene, int mapSize, int instanceCount) {
    scene.geometry.mapSize = mapSize;
    scene.geometry.terrainMaxHeight = 100.0f;
    const int vertexSize = scene.geometry.VertexSize();
    scene.fields.Resize(vertexSize, 0.0f);
    for (int cellY = 0; cellY < vertexSize; ++cellY) {
        for (int cellX = 0; cellX < vertexSize; ++cellX) {
            const float heightValue = static_cast<float>((cellX * 7 + cellY * 13) % 101) * 0.01f;
            scene.fields.heightfield.Set(cellX, cellY, heightValue);
            scene.fields.flow.Set(cellX, cellY, static_cast<float>(cellX % 97) * 0.01f);
            scene.fields.accumulation.Set(cellX, cellY, static_cast<float>(cellY % 89) * 0.01f);
            scene.fields.slope.Set(cellX, cellY, heightValue * 0.5f);
            for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
                const float weight = static_cast<float>((cellX + cellY + stratum * 11) % 103) * 0.01f;
                scene.fields.surfaceStratumWeights[stratum].Set(cellX, cellY, weight);
            }
        }
    }

    scene.strata.assign(Data::MapFields::stratumCount, Params::Stratum());
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        scene.strata[stratum].bEnabled = true;
        scene.strata[stratum].tintRed = static_cast<float>(stratum) / Data::MapFields::stratumCount;
        scene.strata[stratum].tintGreen = 0.5f;
        scene.strata[stratum].tintBlue = 1.0f - scene.strata[stratum].tintRed;
    }
    scene.water.bEnabled = true;
    scene.water.waterLevelMaximum = 30.0f;
    scene.water.deepWaterDepthMaximum = 30.0f;

    scene.instances.Clear();
    scene.instances.Reserve(static_cast<std::size_t>(instanceCount));
    for (int index = 0; index < instanceCount; ++index) {
        Data::PlacementInstance instance;
        instance.positionX = static_cast<float>(index % vertexSize);
        instance.positionZ = static_cast<float>((index / vertexSize) % vertexSize);
        scene.instances.Append(instance);
    }
}

// min/avg/max over kBenchmarkIterationCount samples for one phase.
struct PhaseStats {
    double minMillis   = std::numeric_limits<double>::infinity();
    double maxMillis    = 0.0;
    double sumMillis    = 0.0;
    int    sampleCount = 0;

    void Accumulate(double millis) {
        if (millis < minMillis) minMillis = millis;
        if (millis > maxMillis) maxMillis = millis;
        sumMillis += millis;
        ++sampleCount;
    }
    double AverageMillis() const { return sampleCount > 0 ? sumMillis / sampleCount : 0.0; }
};

// Loose sanity check only — never a threshold gate (no threshold is ratified). Confirms the
// instrumentation itself produced a well-formed number, nothing about whether that number is
// "fast enough."
void CheckPhaseStats(const PhaseStats& stats, const char* phaseLabel) {
    char label[128];
    std::snprintf(label, sizeof(label), "%s: recorded one sample per iteration", phaseLabel);
    check(stats.sampleCount == kBenchmarkIterationCount, label);
    std::snprintf(label, sizeof(label), "%s: minimum is a non-negative, finite duration", phaseLabel);
    check(std::isfinite(stats.minMillis) && stats.minMillis >= 0.0, label);
    std::snprintf(label, sizeof(label), "%s: maximum is finite and at least the minimum", phaseLabel);
    check(std::isfinite(stats.maxMillis) && stats.maxMillis >= stats.minMillis, label);
}

void RunBenchmarkConfiguration(Sys::GpuResourceManager& manager, int mapSize, int instanceCount) {
    Ui::PreviewTestScene scene;
    BuildBenchmarkScene(scene, mapSize, instanceCount);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas,
                                   scene.fields, scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kBenchmarkPreviewResolution;
    composite.SetGpuResourceManager(&manager);

    // Untimed warm-up: forces GL program compile + buffer allocation + a full baked-input upload
    // before timing starts, so the timed loop below never measures one-time setup cost.
    composite.Compose();

    PhaseStats bindAndDispatchStats, fenceWaitStats, entityIdReadbackStats;
    for (int iteration = 0; iteration < kBenchmarkIterationCount; ++iteration) {
        Ui::PreviewComposite::ComposeGpuTiming timing;
        // The EXACT ComposeRequest shape ARCH §14.18 item 10 needs measured: no texel readback (the
        // production hot path never needs it), baked inputs unchanged (the steady-state
        // PreviewRender-tier case Piece C would run under).
        composite.ComposeOnGpu({ /*bNeedsTexelReadback=*/false, /*bBakedInputsChanged=*/false }, &timing);
        bindAndDispatchStats.Accumulate(timing.bindAndDispatchMillis);
        fenceWaitStats.Accumulate(timing.fenceWaitMillis);
        entityIdReadbackStats.Accumulate(timing.entityIdReadbackMillis);
        // texelReadbackMillis is skipped (per the plan) — it is always 0.0 under this request shape.
    }

    CheckPhaseStats(bindAndDispatchStats, "bindAndDispatch");
    CheckPhaseStats(fenceWaitStats, "fenceWait");
    CheckPhaseStats(entityIdReadbackStats, "entityIdReadback");

    std::printf("mapSize=%-5d instanceCount=%-7d\n", mapSize, instanceCount);
    std::printf("    bindAndDispatchMillis  min=%9.4f avg=%9.4f max=%9.4f\n",
                bindAndDispatchStats.minMillis, bindAndDispatchStats.AverageMillis(), bindAndDispatchStats.maxMillis);
    std::printf("    fenceWaitMillis        min=%9.4f avg=%9.4f max=%9.4f\n",
                fenceWaitStats.minMillis, fenceWaitStats.AverageMillis(), fenceWaitStats.maxMillis);
    std::printf("    entityIdReadbackMillis min=%9.4f avg=%9.4f max=%9.4f\n",
                entityIdReadbackStats.minMillis, entityIdReadbackStats.AverageMillis(), entityIdReadbackStats.maxMillis);
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

#if defined(NDEBUG)
    const char* const buildConfigurationLabel = "Release";
#else
    const char* const buildConfigurationLabel = "Debug";
#endif
    std::printf("=== PreviewComposite_GpuBenchmark_UI_Test (ARCH_14_18 item 10) ===\n");
    std::printf("build=%s compiled=%s %s\n", buildConfigurationLabel, __DATE__, __TIME__);
    std::printf("request={bNeedsTexelReadback=false, bBakedInputsChanged=false}, "
                "previewResolution=%d, iterations=%d\n", kBenchmarkPreviewResolution, kBenchmarkIterationCount);
    std::printf("These numbers are for human/expert review; no pass/fail threshold is asserted here.\n\n");

    for (const BenchmarkConfiguration& benchmarkConfiguration : kBenchmarkConfigurations)
        RunBenchmarkConfiguration(manager, benchmarkConfiguration.mapSize, benchmarkConfiguration.instanceCount);

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
