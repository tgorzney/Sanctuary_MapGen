// SystemTab_UI.h — the execution tab: backend, generation context, determinism, asset cache.
// Layer: UI. Accuracy class: Visual (it selects HOW work runs; it computes nothing).
//
// This is the ONE tab that does not touch `Params::MapRecipe`, and that is deliberate:
// MapRecipe_PARAMS.h "excludes execution concerns (dispatch/backend) — those are not
// reproducible-recipe content". The settings here therefore land on the SYS dispatch vocabulary
// (`Sys::DispatchPolicy`, ARCH §4.1) and on the PIPELINE fan-out that already exists
// (`GenerationAssembler::SetGlobalBackend` / `SetGenerationContext`, ARCH §4.3) — the UI drives
// PIPELINE and SYS, never PROC (ARCH §3.1).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; both are reported, not invented):
//  1. ASSET CACHE DIRECTORY has no settings home in the tree: `Io::AssetAtlasCache::BuildOrLoad`
//     takes it as a call argument and `Io::AtlasBuildSettings` holds no path. It is edited below
//     as caller-owned UI state that the app shell (M5-7) passes to the cache. That buffer is not
//     a rival settings home; a durable one needs its own work-order.
//  2. DETERMINISM is modelled by `Sys::DispatchPolicy::bDeterministic`, so the toggle binds to a
//     policy the CALLER owns — but no PIPELINE-level fan-out exists (GenerationAssembler fans out
//     context and global backend, not a policy), so applying it to every stage is the app shell's
//     job until such a setter is ordered.
#pragma once
#include "../sys/Dispatch_SYS.h"
#include <string>

namespace SanmapGen {
namespace Pipeline { class GenerationAssembler; class PreviewDriver; }
namespace Ui {

struct SystemTabState {
    char assetCacheDirectory[260]              = { 0 };   // see SCOPE NOTE 1
    Sys::ComputeBackend    globalBackend       = Sys::ComputeBackend::Automatic;
    Sys::GenerationContext generationContext   = Sys::GenerationContext::Preview;
    bool                   bDeterministic      = false;   // see SCOPE NOTE 2

    // STEP96_FootprintBakeAndStalenessCheck_IO.md §3.1 call site 2 — a plain, caller-owned status
    // string (never a `Params::MapRecipe`/`Io::TemplateIngestReport` field: this tab's own SCOPE NOTE
    // above still holds verbatim). Set by `Application::DrawPerformancePanel` immediately after a
    // "Force Regenerate" click that found a stale bake; empty otherwise. Non-blocking, no modal — the
    // house warning posture (STEP73 §0 / STEP82), surfaced here since this IS the one discrete,
    // human-clicked regeneration trigger left in the current UI (STEP55 retired the toolbar's own).
    std::string lastRegenerateStalenessWarning;
};

// state -> the caller's dispatch policy. Pure and headless-testable: determinism is the one
// execution setting `Sys::DispatchPolicy` already models. Reports whether the policy moved.
inline bool ApplySystemTabSettings(const SystemTabState& state, Sys::DispatchPolicy& dispatchPolicy) {
    const bool bMoved = dispatchPolicy.bDeterministic != state.bDeterministic;
    dispatchPolicy.bDeterministic = state.bDeterministic;
    return bMoved;
}

// Every pointer is nullable; a tab drawn with nothing behind it still edits its own state. Returns
// whether "Force Regenerate" was clicked THIS frame — RequestRegeneration() already fired
// unconditionally either way (STEP96 §3.1 call site 2: the check must never gate the click); the
// caller (Application::DrawPerformancePanel, which alone touches both `Params::MapRecipe` and
// `Io::TemplateIngestReport`) uses the return value only to decide whether to also run the
// non-blocking staleness check and populate `state.lastRegenerateStalenessWarning`.
bool DrawSystemTab(SystemTabState& state, Sys::DispatchPolicy* dispatchPolicy,
                   Pipeline::GenerationAssembler* generationAssembler,
                   Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
