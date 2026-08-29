// PreviewDriver_PIPELINE.cpp — the derivation and the two service paths.
#include "PreviewDriver_PIPELINE.h"

namespace SanmapGen {
namespace Pipeline {

PreviewDriver::PreviewDriver(GenerationAssembler& generationAssembler)
    : assembler(generationAssembler) {
    CacheStageParameterHashes();
}

void PreviewDriver::CacheStageParameterHashes() {
    cachedStageParameterHashes.resize(assembler.StageCount());
    for (std::size_t index = 0; index < cachedStageParameterHashes.size(); ++index)
        cachedStageParameterHashes[index] = assembler.ComputeStageParameterHash(index);
}

// THE derivation (no per-widget list): a parameter belongs to whichever stage's own hash it
// moves, because that hash IS the stage's declaration of what it consumes. The first stage that
// claims the edit is also the stage the dirty-hash conductor will re-run from, so its name is
// reported as well. An edit no stage claims cannot alter a generated field — by construction it
// is presentation (a gradient ramp, a tint, the preview resolution) and the composite alone
// services it.
RefreshTier PreviewDriver::NotifyParametersChanged() {
    owningStageName.clear();
    for (std::size_t index = 0; index < cachedStageParameterHashes.size(); ++index) {
        if (assembler.ComputeStageParameterHash(index) == cachedStageParameterHashes[index])
            continue;
        owningStageName = assembler.StageDescriptions()[index].name;
        bNeedsMapUpdate = true;
        return RefreshTier::MapUpdate;
    }
    bNeedsPreviewRender = true;
    return RefreshTier::PreviewRender;
}

// The map tier wins when both are pending: its own composite already carries the visual edit.
// The cached hashes are refreshed only AFTER the pipeline ran, so an edit made between a notify
// and a refresh is still seen as pending rather than silently swallowed.
RefreshTier PreviewDriver::Refresh() {
    if (bNeedsMapUpdate) {
        stagesThatRanLastRefresh = assembler.Run();
        ++pipelineRunCount;
        CacheStageParameterHashes();
        bNeedsMapUpdate = false;
        bNeedsPreviewRender = false;
        RunPreviewComposite(RefreshTier::MapUpdate);
        return RefreshTier::MapUpdate;
    }
    if (bNeedsPreviewRender) {
        stagesThatRanLastRefresh.clear();   // no stage ran: the bake is reused as-is
        bNeedsPreviewRender = false;
        RunPreviewComposite(RefreshTier::PreviewRender);
        return RefreshTier::PreviewRender;
    }
    return RefreshTier::Nothing;
}

// A headless caller (no composite bound) still resolves its flags correctly — it just has no
// image to show, rather than a driver that refuses to run (Constitution §6). `tier` is exactly
// the tier this Refresh() call is servicing (ARCH §14.18 item 6) — never re-derived here.
void PreviewDriver::RunPreviewComposite(RefreshTier tier) {
    if (!previewCompositeCallback) return;
    previewCompositeCallback(tier);
    ++previewCompositeCount;
}

} // namespace Pipeline
} // namespace SanmapGen
