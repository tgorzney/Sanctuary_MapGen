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
