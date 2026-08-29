# STEP218 — GPU per-phase compose timing instrumentation + benchmark harness (`ComposeGpuTiming`, `PreviewComposite_GpuBenchmark_UI_Test`)

**Layer:** UI (test-only + one additive, opt-in production header/source change). **Domain:**
`PreviewComposite`'s Gpu compose path (`PreviewComposite_UI.h`, `PreviewComposite_Gpu_UI.cpp`) and a
brand-new benchmark executable (`PreviewComposite_GpuBenchmark_UI_Test.cpp`). **Executor:** SanGen
Coder. Authored by the SanGen UI Expert, relaying the SanGen Compute Optimization Expert's exact plan,
per `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` item 10 (the GPU benchmark gate Piece C is blocked
on) and building directly on `STEP216_TierGatedBakedInputUploads_UI.md`'s already-shipped
`ComposeRequest`/`BindComposeBuffers(..., bool bBakedInputsChanged)` shape. Every file cited here was
read directly against the live (post-STEP216) tree while drafting this ticket — no forward-looking/
not-yet-landed prerequisites. **This ticket produces a benchmark tool, not a final architectural
change: its own acceptance criteria are "the binary builds, runs, and prints real numbers," never "the
numbers meet some threshold" — no threshold is ratified anywhere in this document.**

## Summary
Nobody can measure the three numbers ARCH §14.18 item 10 needs (bind+dispatch issue cost, fence-wait
cost, entity-id-readback cost — isolated, not lumped into one wall-clock total) without a small,
additive hook inside `PreviewComposite::ComposeOnGpu`. This ticket adds exactly that hook — a new
nested `PreviewComposite::ComposeGpuTiming` struct and a trailing, optional `ComposeGpuTiming*
outTiming` parameter on `ComposeOnGpu` — with **zero change to any existing call site's observable
behavior** (every timing write is gated behind `outTiming != nullptr`, and `Compose()`'s public
signature/every production caller is untouched). It then adds a new, separate test executable,
`PreviewComposite_GpuBenchmark_UI_Test`, that builds four synthetic scenes (`mapSize` ∈ {256, 1024} ×
`instanceCount` ∈ {0, 100000}), runs one untimed warm-up compose to force GL program compile + buffer
allocation, then loops 50 timed `ComposeOnGpu({false, false}, &timing)` calls per configuration — the
exact steady-state `ComposeRequest` shape item 10's own Piece C would run under — and prints real
min/avg/max numbers per phase per configuration for a human/expert to judge.

## Required reading
`ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` item 10 (the benchmark gate this ticket exists to
satisfy) and `work_orders/STEP216_TierGatedBakedInputUploads_UI.md` in full (already shipped, all
tests passing — this ticket's `ComposeOnGpu`/`ComposeRequest`/`BindComposeBuffers` starting point is
exactly STEP216's own landed shape, verified fresh against the live tree below, not assumed from
STEP216's own text).

---

## 1. Modified: `src/ui/PreviewComposite_UI.h`

**New nested struct** — insert directly after the `ComposeRequest` struct's closing brace (currently
lines 49-60), before the constructor (currently line 62):

Currently:
```cpp
    struct ComposeRequest {
        // When false, skips only the Gpu texture->CPU CompositeTexels() readback (unchanged from the
        // retired `bNeedsTexelReadback` parameter — see Compose()'s own contract note below).
        bool bNeedsTexelReadback = true;
        // ARCH §14.18 item 6 — when false, the composite trusts that PackSurfaceStratumWeights() and
        // the five baked-input uploads (heightfield, flow, accumulation, slope, surface-stratum
        // weights) are BYTE-IDENTICAL to the last compose, and skips re-packing/re-uploading them.
        // Only `Pipeline::PreviewDriver`'s own callback wiring may set this false, and only when it
        // is servicing `RefreshTier::PreviewRender` (no stage ran, so the bake cannot have moved) —
        // see `PreviewDriver_PIPELINE.h`'s own invariant this rests on.
        bool bBakedInputsChanged = true;
    };

    PreviewComposite(const Params::Geometry& geometrySettings, const Params::Water& waterSettings,
```

New:
```cpp
    struct ComposeRequest {
        // When false, skips only the Gpu texture->CPU CompositeTexels() readback (unchanged from the
        // retired `bNeedsTexelReadback` parameter — see Compose()'s own contract note below).
        bool bNeedsTexelReadback = true;
        // ARCH §14.18 item 6 — when false, the composite trusts that PackSurfaceStratumWeights() and
        // the five baked-input uploads (heightfield, flow, accumulation, slope, surface-stratum
        // weights) are BYTE-IDENTICAL to the last compose, and skips re-packing/re-uploading them.
        // Only `Pipeline::PreviewDriver`'s own callback wiring may set this false, and only when it
        // is servicing `RefreshTier::PreviewRender` (no stage ran, so the bake cannot have moved) —
        // see `PreviewDriver_PIPELINE.h`'s own invariant this rests on.
        bool bBakedInputsChanged = true;
    };

    // Gpu-path-specific observability (ARCH §14.18 item 10's benchmark prerequisite; STEP218). Not a
    // second implementation of anything — a pure timing side-channel `ComposeOnGpu` fills in ONLY
    // when a caller hands it a non-null pointer (see `ComposeOnGpu` below). Nested beside
    // `ComposeRequest` for the same reason that struct is nested: this exists only to be
    // `ComposeOnGpu()`'s own vocabulary, not a general-purpose type (STEP216's own nesting
    // precedent, `PreviewComposite_UI.h`'s "Interpretation calls made" §1).
    struct ComposeGpuTiming {
        double bindAndDispatchMillis  = 0.0;  // buffer binds/uploads + all pass dispatches, issue-side
        double fenceWaitMillis        = 0.0;  // WaitForCompletion's spin, isolated
        double texelReadbackMillis    = 0.0;  // only non-zero when request.bNeedsTexelReadback
        double entityIdReadbackMillis = 0.0;  // the unconditional entity-id readback, isolated
    };

    PreviewComposite(const Params::Geometry& geometrySettings, const Params::Water& waterSettings,
```

**`ComposeOnGpu` declaration** — replace (currently line 96):

Currently:
```cpp
    void ComposeOnGpu(ComposeRequest request = ComposeRequest());   // PreviewComposite_Gpu_UI.cpp
```

New:
```cpp
    // `outTiming` (ARCH §14.18 item 10; STEP218) — Gpu-path-specific observability, same spirit as
    // LastRunUsedGpu()/ExecutedPassCount() already being Gpu-path-only accessors on this class. Null
    // by default: every existing call site (including Compose()'s own forwarding call, unchanged
    // below) pays no cost beyond a handful of `!= nullptr` branch checks — no clock read, no write,
    // no behavior change of any kind when omitted.
    void ComposeOnGpu(ComposeRequest request = ComposeRequest(), ComposeGpuTiming* outTiming = nullptr);   // PreviewComposite_Gpu_UI.cpp
```

No other line in this file changes — `Compose()`'s own declaration (line 94) is untouched, and
`Compose()`'s single forwarding call site in `PreviewComposite_UI.cpp` (`ComposeOnGpu(request);`)
needs no edit: the new parameter defaults to `nullptr`.

---

## 2. Modified: `src/ui/PreviewComposite_Gpu_UI.cpp` (full file)

```cpp
// PreviewComposite_Gpu_UI.cpp — the Gpu path's pass sequence: bind, dispatch clear -> one pass
// per enabled field layer -> overlay -> entity id, fence, read the image + ids back.
// Layer: UI. Everything GL goes through `Sys::GpuResourceManager` (ARCH §3.2, §5.4): no private
// GL pipeline, no shader path here, no GL handle — only the opaque handles the SYS seam
// exposes. Program compilation lives in PreviewComposite_GpuProgram_UI.cpp and buffer packing
// in PreviewComposite_GpuBuffers_UI.cpp, behind the same header.
// With no GL context/manager the composite falls back to its Cpu twin and reports Cpu, rather
// than silently producing nothing.
// STEP218 (ARCH §14.18 item 10) — `outTiming`, when non-null, records four isolated phase
// durations into `ComposeGpuTiming` (see PreviewComposite_UI.h). Every clock read below is gated
// behind `outTiming != nullptr`, so an ordinary production/test call (which never passes it) costs
// nothing beyond the branch itself — zero behavior change, zero new allocation, zero new syscall.
#include "PreviewComposite_UI.h"
#include "../sys/GpuResource_SYS.h"
#include <chrono>
#include <thread>

namespace SanmapGen {
namespace Ui {
namespace {

// The fence poll is an early-out, not a correctness requirement (the readback is ordered after
// the dispatch by GL itself), so it yields instead of burning a core.
constexpr int fencePollLimit = 100000;

unsigned TileGroupCount(int extent, int tileExtent) {
    return extent > 0 ? static_cast<unsigned>((extent + tileExtent - 1) / tileExtent) : 0u;
}

void WaitForCompletion(Sys::GpuResourceManager& manager) {
    const Sys::GpuFenceHandle fence = manager.InsertFence();
    for (int poll = 0; poll < fencePollLimit && !manager.IsFenceSignaled(fence); ++poll)
        std::this_thread::yield();
    manager.DeleteFence(fence);
}

// `outTiming != nullptr` is checked once per call site rather than hoisted into a helper, so the
// four measurement windows below stay visually paired with the exact GL call each one brackets —
// legible at the call site, not hidden behind an indirection.
std::chrono::steady_clock::time_point NowIfTimed(const PreviewComposite::ComposeGpuTiming* outTiming) {
    return outTiming != nullptr ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point();
}

double ElapsedMillisSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

} // namespace

void PreviewComposite::ComposeOnGpu(ComposeRequest request, ComposeGpuTiming* outTiming) {
    if (outTiming != nullptr) *outTiming = ComposeGpuTiming();
    PrepareRun();
    const int resolution = configuration.previewResolution;
    if (resolution <= 0) return;
    if (!EnsureGpuResources()) { ComposeOnCpu(); return; }        // no GL -> the Cpu twin

    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    if (!EnsureCompositeTexture(manager)) { ComposeOnCpu(); return; }   // no image -> the Cpu twin

    // bindAndDispatchMillis: buffer binds/uploads (BindComposeBuffers) through the last dispatch
    // call, issue-side only — this does NOT wait for the GPU to finish (that's fenceWaitMillis,
    // measured separately below).
    const std::chrono::steady_clock::time_point bindAndDispatchStart = NowIfTimed(outTiming);
    BindComposeBuffers(manager, request.bBakedInputsChanged);

    const unsigned pixelGroupsX = TileGroupCount(resolution, Sys::WorkgroupSize::kFieldTileWidth);
    const unsigned pixelGroupsY = TileGroupCount(resolution, Sys::WorkgroupSize::kFieldTileHeight);
    const int threadsPerGroup =
        Sys::WorkgroupSize::kFieldTileWidth * Sys::WorkgroupSize::kFieldTileHeight;
    const unsigned entityGroups = TileGroupCount(configuration.entityCount, threadsPerGroup);

    manager.SetUniformInt(program, "passIndex", CompositePass::kClear);
    manager.Dispatch(program, pixelGroupsX, pixelGroupsY, 1);
    ++executedPassCount;

    manager.SetUniformInt(program, "passIndex", CompositePass::kFieldLayer);
    for (int layerIndex = 0; layerIndex < configuration.layerCount; ++layerIndex) {
        manager.SetUniformInt(program, "layerIndex", layerIndex);
        manager.Dispatch(program, pixelGroupsX, pixelGroupsY, 1);
        ++executedPassCount;
    }

    // The two entity passes run one thread per RESOLVED instance, not per pixel. With no
    // instances they are counted (the sequence is the same) but never dispatched — a zero-group
    // dispatch is not a legal GL call.
    for (int entityPass = CompositePass::kOverlay; entityPass <= CompositePass::kEntityIdentifier;
         ++entityPass) {
        if (entityGroups > 0u) {
            manager.SetUniformInt(program, "passIndex", entityPass);
            manager.Dispatch(program, entityGroups, 1, 1);
        }
        ++executedPassCount;
    }
    if (outTiming != nullptr) outTiming->bindAndDispatchMillis = ElapsedMillisSince(bindAndDispatchStart);

    // fenceWaitMillis: WaitForCompletion's own spin, isolated from the issue-side cost above.
    const std::chrono::steady_clock::time_point fenceWaitStart = NowIfTimed(outTiming);
    WaitForCompletion(manager);
    if (outTiming != nullptr) outTiming->fenceWaitMillis = ElapsedMillisSince(fenceWaitStart);

    // The canvas draws the texture itself; this readback exists so the Cpu twin stays the parity
    // reference and a headless caller still gets bytes. RGBA8 texels come back R,G,B,A per pixel,
    // which is exactly the byte order `Ui::PackRgba8` packs into one unsigned int. Skipped when
    // the caller knows nothing will read `CompositeTexels()` this run (e.g. the production
    // hot path, where the canvas samples `CompositeTexture()` directly). texelReadbackMillis stays
    // at its ComposeGpuTiming() default (0.0) whenever this branch does not run — the benchmark's
    // own request shape (`bNeedsTexelReadback=false`) always takes this path.
    if (request.bNeedsTexelReadback) {
        const std::chrono::steady_clock::time_point texelReadbackStart = NowIfTimed(outTiming);
        manager.ReadbackTexture(compositeTexture, compositeTexels.data(),
                                compositeTexels.size() * sizeof(unsigned int));
        if (outTiming != nullptr) outTiming->texelReadbackMillis = ElapsedMillisSince(texelReadbackStart);
    }
    // Unaffected by EITHER `request` flag: `MapCanvas::ApplyClick` reads this unconditionally on
    // both backends for click-picking, on every recomposite (ARCH §14.18 item 7's own note —
    // skipping this readback is a picking-correctness argument this ruling explicitly does NOT
    // make; it is named there as the next lever if the item-10 benchmark misses, not in scope here).
    const std::chrono::steady_clock::time_point entityIdReadbackStart = NowIfTimed(outTiming);
    manager.ReadbackBuffer(CompositeBufferName::kEntityIdentifiers, entityIdentifierBuffer.Data(),
                           entityIdentifierBuffer.CellCount() * sizeof(unsigned int));
    if (outTiming != nullptr) outTiming->entityIdReadbackMillis = ElapsedMillisSince(entityIdReadbackStart);
    bLastRunUsedGpu = true;
}

} // namespace Ui
} // namespace SanmapGen
```

No change to `PreviewComposite_UI.cpp`'s `Compose()` (its one forwarding call, `ComposeOnGpu(request);`,
compiles unchanged — the new parameter defaults to `nullptr`), no change to
`PreviewComposite_GpuBuffers_UI.cpp`, no change to `PreviewComposite_Cpu_UI.cpp`.

---

## 3. New file: `src/ui/PreviewComposite_GpuBenchmark_UI_Test.cpp`

Mirrors `PreviewComposite_Gpu_UI_Test.cpp`'s hidden-WGL-context boilerplate verbatim (same class of
test harness, not app code — `CreateHiddenGlContext`, the `argv[1]`-shader-directory convention, the
"no GL context → skip with exit code 2" contract).

```cpp
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
```

---

## 4. Modified: `CMakeLists.txt`

Insert directly after the existing `PreviewComposite_Gpu_UI_Test` registration (currently line 564),
before the STEP200 comment block that belongs to `PreviewComposite_BlendModes_UI_Test`:

Currently:
```cmake
add_sangen_test(PreviewComposite_Gpu_UI_Test      src/ui/PreviewComposite_Gpu_UI_Test.cpp)
# STEP200 — the six v1-parity PreviewBlendMode enumerators (Subtract..HardLight): Cpu CombineChannel
```

New:
```cmake
add_sangen_test(PreviewComposite_Gpu_UI_Test      src/ui/PreviewComposite_Gpu_UI_Test.cpp)
# STEP218 — ARCH_14_18 item 10's GPU benchmark harness: per-phase (bind+dispatch issue-side /
# fence-wait / entity-id-readback) min/avg/max timing across {256,1024} mapSize x {0,100000}
# instanceCount, via ComposeOnGpu's new optional ComposeGpuTiming* parameter. A measurement tool —
# it prints numbers for review, it does not assert a threshold (none is ratified).
add_sangen_test(PreviewComposite_GpuBenchmark_UI_Test src/ui/PreviewComposite_GpuBenchmark_UI_Test.cpp)
# STEP200 — the six v1-parity PreviewBlendMode enumerators (Subtract..HardLight): Cpu CombineChannel
```

`add_sangen_test` (the function itself, `CMakeLists.txt:396-401`) already hands every registered test
binary the shader directory as `argv[1]` and the Lua resource directory as `argv[2]` automatically —
no further wiring is needed for this new binary to receive its shader directory.

---

## ARCH rules invoked
- `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` item 10 — this ticket's entire reason to exist: Piece
  C (the drag-suppression-vs-gated-recompose ruling) cannot be authored without the three per-phase
  numbers this benchmark produces.
- `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` items 6-7 / `STEP216_TierGatedBakedInputUploads_UI.md`
  — this ticket's `ComposeRequest`/`BindComposeBuffers` starting point; unmodified by this ticket.
- Constitution §8 — `kBenchmarkIterationCount`/`kBenchmarkPreviewResolution` are named constants, not
  inline literals scattered through the benchmark body.
- Constitution §1 (UI owns no sim logic) — untouched; this ticket adds observability to an existing
  Gpu dispatch path, no new sim/param coupling of any kind.

## Explicit out-of-scope
- **No change to `Compose()`'s non-Gpu overload or public signature.** `Compose(ComposeRequest)`'s own
  declaration and its one forwarding call to `ComposeOnGpu` are untouched.
- **No change to any production call site.** `Application_UI.cpp`'s `SetPreviewCompositeCallback`
  lambda (STEP216) keeps calling `composite.Compose({...})` — it never sees `ComposeGpuTiming` at all.
- **No change to the Cpu twin** (`PreviewComposite_Cpu_UI.cpp`) — it never dispatches/fences/reads
  back a GPU buffer, so there is nothing in it for `ComposeGpuTiming` to measure.
- **No change to `ComposeRequest` itself** — only a new, separate, optional trailing pointer parameter
  is added to `ComposeOnGpu`'s signature; `ComposeRequest`'s own two fields are unmodified.
- **No threshold assertion of any kind.** Every `check()` in the new benchmark file is a loose sanity
  check ("this duration is non-negative and finite"), never a `< N milliseconds` pass/fail gate. The
  printed numbers are this ticket's deliverable; judging them is explicitly a follow-up step for the
  SanGen Compute Optimization Expert / SanGen UI Optimization Expert, not this ticket.
- **Piece C itself** (the drag-suppression-vs-gated-recompose change, ARCH §14.18 items 3/4/8/9) —
  not authored here, and cannot be until this ticket's numbers are reviewed.

## Acceptance test
1. Full `SanGenV2` build stays clean.
2. Every existing test continues to pass unmodified — no test other than
   `PreviewComposite_Gpu_UI_Test.cpp` (unaffected: `ComposeOnGpu`'s new parameter defaults to
   `nullptr`, reproducing every call in that file byte-for-byte) references `ComposeOnGpu` or
   `ComposeGpuTiming` at all.
3. `PreviewComposite_Gpu_UI_Test`'s existing `CheckGatedUploadParity`/`CheckAllBlendModesParity`/
   `CheckGpuPathAndParity` assertions (including `manager.CompileCount() == 1`) are unaffected.
4. New `PreviewComposite_GpuBenchmark_UI_Test` binary: with no GL context, prints
   `GPU SKIPPED (no GL context)` and returns exit code 2 (same contract as
   `PreviewComposite_Gpu_UI_Test`). With a GL context, it builds all 4 configurations, runs the
   50-iteration timed loop for each, and prints a basis-tag header (build config + compile
   timestamp) followed by real min/avg/max numbers (in milliseconds) for `bindAndDispatchMillis`,
   `fenceWaitMillis`, and `entityIdReadbackMillis` per configuration, then `ALL PASS`.
5. No assertion in the new binary can ever fail purely because a number is "too slow" — every check is
   a finite/non-negative/ordering sanity check, never a hard-coded threshold.

## Interpretation calls made beyond the relayed plan's literal text
1. **`ComposeGpuTiming` is a nested `PreviewComposite::ComposeGpuTiming` struct**, not a free
   `Ui::ComposeGpuTiming`. The relayed plan showed it as a bare `struct ComposeGpuTiming { ... };`
   without a scope qualifier; nesting it beside `ComposeRequest` follows STEP216's own explicit
   precedent/reasoning for nesting `ComposeRequest` there ("nesting matches this file's own existing
   convention for structs that exist only to be `Compose()`'s own vocabulary") — `ComposeGpuTiming`
   exists only to be `ComposeOnGpu()`'s own output vocabulary, the same class of type.
2. **The four timing windows are placed exactly where the plan named them** — "before/after
   `BindComposeBuffers` + the dispatch loop, before/after `WaitForCompletion`, before/after each of
   the two readbacks" — confirmed against the live, post-STEP216 function body read fresh for this
   ticket. `PrepareRun()`/`EnsureGpuResources()`/`EnsureCompositeTexture()` (all of which run before
   the first timing window opens) are deliberately NOT measured — the plan names only four phases,
   not these setup calls, and the benchmark's own warm-up compose already forces their one-time cost
   (program compile, texture/initial buffer allocation) out of the timed loop.
3. **`NowIfTimed`/`ElapsedMillisSince` local helpers** factor the repeated
   "`outTiming != nullptr` ? read the clock : default-constructed time point" / "duration since"
   pattern into two one-line functions inside the file's own anonymous namespace, rather than
   repeating the ternary inline four times — a legibility choice, not a new dependency (still
   zero-cost when `outTiming` is null: `NowIfTimed` still branches on the same pointer check before
   ever calling `steady_clock::now()`).
4. **The benchmark scene's fill/spread formulas** (the modulo-based per-cell height/flow/
   accumulation/slope/stratum-weight patterns, and the raster-scan instance placement
   `positionX = index % vertexSize`, `positionZ = (index / vertexSize) % vertexSize`) are one
   concrete realization of the plan's qualitative requirement ("varied, non-degenerate data... not a
   single degenerate point") — any other non-degenerate, full-extent fill would satisfy the same
   intent equally well.
5. **`kBenchmarkIterationCount = 50`/`kBenchmarkPreviewResolution = 512`** are declared as named
   constants (Constitution §8) rather than literals repeated inline, even though the plan itself
   states both numbers directly.
6. **`CMakeLists.txt` insertion point** — directly after the `PreviewComposite_Gpu_UI_Test` line and
   before the STEP200 comment block (which describes `PreviewComposite_BlendModes_UI_Test`, an
   unrelated binary) — the plan says only "next to the existing registration," not an exact line.
7. **The printed report's exact layout** (one header block, then one `mapSize=.../instanceCount=...`
   line followed by 3 indented phase lines per configuration) is a display-only choice satisfying the
   plan's "one line per configuration per phase" requirement; any other legible tabulation would be
   equally acceptable.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_14_18_AreaLiveBlendFidelityAndPalette.md`,
`D:\Projects\Sanctuary\Map Generator\work_orders\STEP216_TierGatedBakedInputUploads_UI.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Gpu_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_GpuBuffers_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Gpu_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_TestScene_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\data\PlacementInstances_DATA.h`,
`D:\Projects\Sanctuary\Map Generator\src\data\PlacementInstance_DATA.h`,
`D:\Projects\Sanctuary\Map Generator\src\data\MapFields_DATA.h`,
`D:\Projects\Sanctuary\Map Generator\src\data\FloatField_DATA.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\Geometry_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt` (the `add_sangen_test` function definition,
lines 396-401, and the `PreviewComposite_*` registration cluster around lines 561-577),
`D:\Projects\Sanctuary\Map Generator\work_orders\STEP210_AreaCanvasGesture_UI.md` (format/rigor
template).
