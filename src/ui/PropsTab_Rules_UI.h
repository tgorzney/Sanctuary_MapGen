// PropsTab_Rules_UI.h — the per-rule sections of the Props tab's procedural stack. Layer: UI.
// Accuracy class: Visual. TAB_REBUILD_PLAN "§ Props · Procedural Props stack".
// Split out of PropsTab_UI.cpp only to stay inside the ARCH §1.5 file ceilings.
//
// `PropsTabState` is referenced, never defined here — that keeps the include one-way
// (PropsTab_UI.h -> this header) with no cycle.
#pragma once
#include "PlacementRuleSections_UI.h"
#include "../io/TemplateIngest_IO.h"
#include "../params/ScatterRule_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct PropsTabState;

// Caller-owned section state for the two blocks below.
struct PropRuleDetailState {
    SectionState gateSection;
    SectionState affinitySection;
};

// The label a rule row shows. %.7s is not usable in a returned pointer, so the caller formats;
// this is the fallback for a rule whose tpId was never typed (Constitution §6 — never an empty row).
inline bool PropRuleHasTemplateIdentifier(const Params::PropRule& rule) {
    return rule.transform.templateIdentifier[0] != '\0';
}

// STEP96_FootprintBakeAndStalenessCheck_IO.md §2 — the "Resolve Footprint" bake action's pure core.
// Overwrites baseFootprintWidth/baseFootprintDepth/footprintBakeFingerprint ONLY (never
// spacingMinimum/obstacleDistanceMinimum) and ONLY when `record` is non-null; returns whether it
// wrote anything, so the caller (imgui-bound) can show the "no ingested data" inline message on a
// miss with no popup/modal. Pure -- testable with no imgui frame (PlacementRuleSections_UI.h's own
// "THE SPLIT" posture).
inline bool ApplyResolvedFootprintBake(Params::PropRule& rule, const Io::TemplateFootprintRecord* record) {
    if (record == nullptr) return false;
    rule.baseFootprintWidth  = record->baseFootprintWidth;
    rule.baseFootprintDepth  = record->baseFootprintDepth;
    rule.footprintBakeFingerprint.sourcePath   = record->sourceFingerprint.sourcePath;
    rule.footprintBakeFingerprint.byteSize     = record->sourceFingerprint.byteSize;
    rule.footprintBakeFingerprint.modifiedTime = record->sourceFingerprint.modifiedTime;
    rule.footprintBakeFingerprint.contentHash  = record->sourceFingerprint.contentHash;
    return true;
}

void DrawPropRuleGates(Params::PropRule& rule, PropsTabState& state,
                       Pipeline::PreviewDriver* previewDriver);
void DrawPropRuleAffinities(Params::PropRule& rule, PropsTabState& state,
                            Pipeline::PreviewDriver* previewDriver);
// Drawn by PropsTab_UI.cpp's DrawRuleStack, immediately after the shared template-id picker (that
// picker lives in PlacementRuleSections_UI.cpp and is NOT widened by this ticket — the button reads
// PropRule-specific fields the shared function never touches). Disabled when the rule's tpId is
// empty; fires only on click, never inside dirty-hash recompute. `templateIngestReport` is nullable
// (STEP90/91's session-scoped ingestion state; absent = "no ingested data" on every click).
void DrawResolvePropFootprintButton(Params::PropRule& rule, PropsTabState& state,
                                    const Io::TemplateIngestReport* templateIngestReport);

} // namespace Ui
} // namespace SanmapGen
