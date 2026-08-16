// PropsTab_Rules_UI.h — the per-rule sections of the Props tab's procedural stack. Layer: UI.
// Accuracy class: Visual. TAB_REBUILD_PLAN "§ Props · Procedural Props stack".
// Split out of PropsTab_UI.cpp only to stay inside the ARCH §1.5 file ceilings.
//
// `PropsTabState` is referenced, never defined here — that keeps the include one-way
// (PropsTab_UI.h -> this header) with no cycle.
#pragma once
#include "PlacementRuleSections_UI.h"
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

void DrawPropRuleGates(Params::PropRule& rule, PropsTabState& state,
                       Pipeline::PreviewDriver* previewDriver);
void DrawPropRuleAffinities(Params::PropRule& rule, PropsTabState& state,
                            Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
