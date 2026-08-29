// PreviewDriver_PIPELINE.h — the two-tier dirty model, DERIVED from the dependency DAG.
// Layer: PIPELINE. One edit, one question: does any generation stage own the field that moved?
//   - yes -> `bNeedsMapUpdate`: run the pipeline (which rebuilds the marker spatial grid inside
//     the Placement stage, ARCH §8.3), then exactly ONE composite.
//   - no  -> `bNeedsPreviewRender`: the composite alone. No stage runs, so nothing re-simulates
//     and the spatial grid cannot move (ARCH §6.1 / UI_FRAMEWORK_SPEC: a cheap visual tweak
//     must never trigger a full regen).
// The answer comes from each stage's own `ComputeParameterHash` — the stage's declaration of
// what it consumes — so a new widget classifies itself and no per-widget list is maintained.
//
// The composite itself is INJECTED as a callback: PIPELINE may never depend on UI (ARCH §3.1),
// exactly as `Generation_PIPELINE` injects its stage closures. The driver owns *when* a
// composite happens and never *what* it is; `Ui::PreviewComposite` is bound by the UI caller.
//
// Distinct from `RegenerationTier` on the assembler, which answers a different question — was
// the last run gameplay-authoritative (determinism/authority, ARCH §4.5)? These two flags answer
// what an edit COSTS. The driver is the single owner of them; no rival toggle exists.
#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include "GenerationAssembler_PIPELINE.h"

namespace SanmapGen {
namespace Pipeline {

// What one refresh has to do. `Nothing` means both flags were already clear.
enum class RefreshTier { Nothing, PreviewRender, MapUpdate };

class PreviewDriver {
public:
    explicit PreviewDriver(GenerationAssembler& generationAssembler);

    // ARCH §14.18 item 6 — the callback now receives the tier it is servicing, so a UI-side
    // composite can gate its own baked-input uploads on it. `RefreshTier::PreviewRender` means "no
    // stage ran, so nothing re-simulates" (this file's own header comment, above) — the invariant
    // this whole wiring rests on: every `MapUpdate` refresh composites immediately after the stages
    // run, so a SUBSEQUENT `PreviewRender` compose is provably looking at byte-identical baked
    // fields.
    void SetPreviewCompositeCallback(std::function<void(RefreshTier)> composePreview) {
        previewCompositeCallback = std::move(composePreview);
    }

    // Call after ANY settings edit. Returns (and records) the tier the edit derived.
    RefreshTier NotifyParametersChanged();
    // A change no parameter hash can see — a resize, a recipe reload, new stratum art.
    void RequestMapUpdate() { bNeedsMapUpdate = true; }

    // Services whichever flag is set and clears it. Returns the tier actually serviced.
    RefreshTier Refresh();

    bool NeedsMapUpdate() const { return bNeedsMapUpdate; }
    bool NeedsPreviewRender() const { return bNeedsPreviewRender; }
    // The stage that claimed the last edit — empty when it was presentation-only.
    const std::string& OwningStageName() const { return owningStageName; }
    // Stages the last Refresh ran; empty after a preview-only refresh.
    const std::vector<std::string>& StagesThatRan() const { return stagesThatRanLastRefresh; }
    int PipelineRunCount() const { return pipelineRunCount; }
    int PreviewCompositeCount() const { return previewCompositeCount; }

private:
    void CacheStageParameterHashes();
    void RunPreviewComposite(RefreshTier tier);

    GenerationAssembler&              assembler;
    std::function<void(RefreshTier)>  previewCompositeCallback;
    std::vector<std::size_t>          cachedStageParameterHashes;
    std::vector<std::string>          stagesThatRanLastRefresh;
    std::string                       owningStageName;

    bool bNeedsMapUpdate      = true;    // nothing is generated yet
    bool bNeedsPreviewRender  = false;
    int  pipelineRunCount     = 0;
    int  previewCompositeCount = 0;
};

} // namespace Pipeline
} // namespace SanmapGen
