// PreviewComposite_Gpu_UI.cpp — the Gpu path's pass sequence: bind, dispatch clear -> one pass
// per enabled field layer -> overlay -> entity id, fence, read the image + ids back.
// Layer: UI. Everything GL goes through `Sys::GpuResourceManager` (ARCH §3.2, §5.4): no private
// GL pipeline, no shader path here, no GL handle — only the opaque handles the SYS seam
// exposes. Program compilation lives in PreviewComposite_GpuProgram_UI.cpp and buffer packing
// in PreviewComposite_GpuBuffers_UI.cpp, behind the same header.
// With no GL context/manager the composite falls back to its Cpu twin and reports Cpu, rather
// than silently producing nothing.
#include "PreviewComposite_UI.h"
#include "../sys/GpuResource_SYS.h"
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

} // namespace

void PreviewComposite::ComposeOnGpu() {
    PrepareRun();
    const int resolution = configuration.previewResolution;
    if (resolution <= 0) return;
    if (!EnsureGpuResources()) { ComposeOnCpu(); return; }        // no GL -> the Cpu twin

    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    BindComposeBuffers(manager);

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

    WaitForCompletion(manager);
    manager.ReadbackBuffer(CompositeBufferName::kCompositeTexels, compositeTexels.data(),
                           compositeTexels.size() * sizeof(unsigned int));
    manager.ReadbackBuffer(CompositeBufferName::kEntityIdentifiers, entityIdentifierBuffer.Data(),
                           entityIdentifierBuffer.CellCount() * sizeof(unsigned int));
    bLastRunUsedGpu = true;
}

} // namespace Ui
} // namespace SanmapGen
