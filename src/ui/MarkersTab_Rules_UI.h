// MarkersTab_Rules_UI.h — the per-rule detail sections of the Markers tab, and the enum label
// tables the rule list and the detail rows share. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "§ Markers · Procedural layers": one MarkerRule carries far more settings than
// fit one file under the ARCH §1.5 ceilings, so the sections live here and in
// MarkersTab_Area_UI.cpp behind the one small header MarkersTab_UI.h includes.
//
// `MarkersTabState` is only ever referenced, never defined here — that is what keeps the include
// one-way (MarkersTab_UI.h -> this header) with no cycle.
#pragma once
#include "PlacementRuleSections_UI.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState;

// The rule list rows and the Type dropdown must never disagree about what a category is called,
// so the vocabulary is declared once (Constitution §8: labels are settings, not literals).
enum : int { kMarkerCategoryCount = 4, kMarkerPriorityCount = 3, kMarkerFocusGradientCount = 4 };
inline const char* const markerCategoryLabels[kMarkerCategoryCount] = {
    "Generic", "Spawn", "Alloys", "Expansion"
};
inline const char* const markerPriorityLabels[kMarkerPriorityCount] = {
    "Largest Area", "Smallest Area", "Least Variance"
};
inline const char* const markerFocusGradientLabels[kMarkerFocusGradientCount] = {
    "None", "Center Focus", "Edge Focus", "Torus"
};

// Caller-owned limits and RT toggles for the sections below (Constitution §8 — every limit is a
// setting, never a literal at a use site).
struct MarkerRuleDetailState {
    SectionState gateSection;
    SectionState quantitySection;
    SectionState areaSection;
    SectionState focusSection;

    ScalarSliderRange areaRadiusMinimumRange{ 0.0f, 200.0f, 0.0f };
    ScalarSliderRange areaRadiusMaximumRange{ 0.0f, 500.0f, 0.0f };
    ScalarSliderRange areaHeightRange{ 0.0f, 10.0f, 0.0f };
    ScalarSliderRange focusRadiusRange{ 0.0f, 1000.0f, 0.0f };
    ScalarSliderRange focusStrengthRange{ 0.0f, 5.0f, 0.0f };
    ScalarSliderRange focusContrastRange{ 0.1f, 5.0f, 0.0f };

    RealtimeToggle areaRadiusMinimumToggle;
    RealtimeToggle areaRadiusMaximumToggle;
    RealtimeToggle areaHeightRangeToggle;
    RealtimeToggle focusRadiusToggle;
    RealtimeToggle focusStrengthToggle;
    RealtimeToggle focusContrastToggle;

    int categoryIndex      = 0;   // mirrors of the three enum fields, for Combo_UI
    int priorityIndex      = 0;
    int focusGradientIndex = 0;
};

// rule -> the three enum mirrors. Run whenever no edit is pending.
inline void LoadMarkerRuleEnumIndices(const Params::MarkerRule& rule, MarkerRuleDetailState& state) {
    state.categoryIndex      = static_cast<int>(rule.category);
    state.priorityIndex      = static_cast<int>(rule.priority);
    state.focusGradientIndex = static_cast<int>(rule.focusGradient);
}

// The label a rule list row shows for a category, never null: a category from a recipe written
// against a longer table falls back rather than reading off the end (Constitution §6).
inline const char* MarkerCategoryLabel(Params::MarkerCategory category) {
    const int categoryIndex = static_cast<int>(category);
    if (categoryIndex < 0 || categoryIndex >= kMarkerCategoryCount) return "Generic";
    return markerCategoryLabels[categoryIndex];
}

// MarkersTab_Rules_UI.cpp
void DrawMarkerRuleGates(Params::MarkerRule& rule, MarkersTabState& state,
                         Pipeline::PreviewDriver* previewDriver);
void DrawMarkerRuleQuantity(Params::MarkerRule& rule, MarkersTabState& state,
                            Pipeline::PreviewDriver* previewDriver);
// MarkersTab_Area_UI.cpp
void DrawMarkerRuleArea(Params::MarkerRule& rule, MarkerRuleDetailState& state,
                        Pipeline::PreviewDriver* previewDriver);
void DrawMarkerRuleFocus(Params::MarkerRule& rule, MarkerRuleDetailState& state,
                         Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
