// MapCanvas_ScenarioEditMode_UI.h — the interactive "Scenario Edit Mode" canvas overlay: a
// dedicated, transient, single-scenario draw+interaction pass for authoring
// Params::ScenarioBody's spawns/alloys by dragging on the map, instead of only STEP74's flat
// numeric fields. Layer: UI. NOT the generic stackable OverlayLayer_UI machinery — its source is
// the OPEN scenario's own spawns/alloys/alloysToAdd/alloysToRemove plus the baked baseline
// Alloy/SpawnsArmies overlay layers, read as desaturated-in-spirit ghost context (real baked
// positions, drawn with this module's own neutral/greyed states rather than the live overlay's
// own tint — STEP78's Fix section item 1).
//
// Cardinality is tens of entries (spawns/alloys per scenario) — a linear screen-rect hit test is
// correct and sufficient; this module never touches Data::SpatialGrid/Picking_UI (STEP78's own
// explicit "do not over-engineer" flag).
//
// §0 — SanGen bakes NO per-instance army identity for Spawn/Alloy marker instances: confirmed
// directly against Placement_Rules_PROC.cpp (only AppendUnitRules sets configuration.armyIndex;
// AppendPropRules/AppendDecalRules and the marker-rule path never do). Two positional conventions
// this module therefore documents as flagged, reasoned coder choices rather than verified
// gameplay fact (a visual authoring aid, Accuracy class: Visual — never gameplay-authoritative):
//   - a baked SPAWN instance's "owning army" is its 0-based position across the SpawnsArmies
//     overlay layer's sub-layers, walked in the SAME flat order
//     Application_OverlaySetup_UI.cpp's SeedMarkerDomains seeded them in, matched positionally
//     against `armies[i]`.
//   - a baked ALLOY instance's synthesized `markerName` ("alloy_r<ruleIndex>_<bucketPosition>")
//     is this module's OWN naming convention for round-tripping a right-click "remove" back onto
//     the same baked instance — `ScenarioAlloyOverride::markerName`/`ScenarioAlloyRemoval::
//     markerName` are free-text with no baked-instance linkage in the ratified PARAMS shape
//     (confirmed: ScenariosTab_DetailAlloys_UI.cpp's own header comment already flags this as
//     "STEP78's job").
//
// Split across sibling translation units/headers (Constitution §1.5 file-size ceiling):
// _State_UI.h (ScenarioEditModeState, Activate/Deactivate), _Ops_UI.h (the imgui-adjacent entry
// points), _Baseline_UI.cpp (real baked instances), _ClassifySpawns_UI.cpp/_ClassifyAlloys_UI.cpp
// (the merge), _PreviewAs_UI.cpp, _Interaction_UI.cpp, _Commit_UI.cpp, _DrawMarkers_UI.cpp,
// _Chrome_UI.cpp — each self-documented at its own top.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace Data { struct PlacementResults; struct RuleBucketIndexSet; }
namespace Params { struct Army; }
namespace Ui {

struct OverlayLayerSettings;

// The six visually-distinct states STEP78's Fix table names, verbatim order.
enum class ScenarioMarkerVisualState_UI : int {
    SpawnNoOverride, SpawnExplicit, AlloyKept, AlloyDeleted, AlloyAdded, AlloyRemovedGhost
};
enum class ScenarioMarkerKind_UI : int { Spawn, Alloy };

// -1 = no known baked-instance/army/row identity (Constitution §6 sentinel, mirrors
// Ui::kNoMarkerPicked's own convention).
inline constexpr int kScenarioEditModeNoIndex = -1;

// One resolved marker this frame's draw AND interaction both consume — the SAME resolve pass
// backs both (cached on ScenarioEditModeState::lastResolvedCandidates below), so a drag can never
// hit-test against a stale frame's positions.
struct ScenarioEditMarkerCandidate_UI {
    ScenarioMarkerKind_UI        kind  = ScenarioMarkerKind_UI::Spawn;
    ScenarioMarkerVisualState_UI state = ScenarioMarkerVisualState_UI::SpawnNoOverride;
    float worldX = 0.0f, worldY = 0.0f, worldZ = 0.0f;
    std::string templateIdentifier;               // baked instance's own, when known; else empty
    int   armyIndex = kScenarioEditModeNoIndex;    // into the recipe's armies[]; unknown (§0) for
                                                    // a baseline Alloy or an AlloyAdded candidate
    std::string markerName;    // synthesized ("alloy_r<ruleIndex>_<bucketPosition>", §0) for a
                                // baseline alloy, or the authored override/removal's own text.
    // Back-references into the edited ScenarioBody's own arrays; kScenarioEditModeNoIndex means
    // "no row yet" — a drag/right-click on such a candidate APPENDS a new row (materialization).
    int  spawnRowIndex         = kScenarioEditModeNoIndex;   // body.spawns
    int  alloyOverrideRowIndex = kScenarioEditModeNoIndex;   // body.alloys OR body.alloysToAdd
    bool bAlloyOverrideIsAdd   = false;                       // which of the two arrays above
    int  alloyRemovalRowIndex  = kScenarioEditModeNoIndex;    // body.alloysToRemove
};

// The frame's read-only sources for baseline resolution — mirrors DrawOverlayIconLayersInput's own
// push-in-pointer shape (STEP48's pattern) rather than reaching back through Application.
struct ScenarioEditModeResolveInput {
    const OverlayLayerSettings*      overlayLayerSettings = nullptr;
    const Data::PlacementResults*    placements            = nullptr;
    const Data::RuleBucketIndexSet*  ruleBucketIndex        = nullptr;
    const std::vector<Params::Army>* armies                 = nullptr;
};

// _Baseline_UI.cpp — one baked-instance candidate per marker STEP50's CSR bucket walk visits.
struct ScenarioEditModeBaselineInstance_UI {
    float worldX = 0.0f, worldY = 0.0f, worldZ = 0.0f;
    std::string templateIdentifier;
    std::string markerName;     // alloys only; spawns instead carry armyIndex (see §0 above)
    int armyIndex = kScenarioEditModeNoIndex;
};
void ResolveScenarioEditModeBaselineAlloys(const ScenarioEditModeResolveInput& input,
                                           std::vector<ScenarioEditModeBaselineInstance_UI>& outBaseline);
void ResolveScenarioEditModeBaselineSpawns(const ScenarioEditModeResolveInput& input,
                                           std::vector<ScenarioEditModeBaselineInstance_UI>& outBaseline);

// _ClassifySpawns_UI.cpp / _ClassifyAlloys_UI.cpp
void AppendScenarioEditModeSpawnCandidates(const ScenarioEditModeResolveInput& input,
                                           const Params::ScenarioBody& body,
                                           std::vector<ScenarioEditMarkerCandidate_UI>& outCandidates);
void AppendScenarioEditModeAlloyCandidates(const ScenarioEditModeResolveInput& input,
                                           const Params::ScenarioBody& body,
                                           const std::string& previewAsSlotPattern,
                                           std::vector<ScenarioEditMarkerCandidate_UI>& outCandidates);

// The merge, called once per frame by the draw pass (MapCanvas_ScenarioEditMode_Ops_UI.h) and
// reused by both it and the interaction pass via ScenarioEditModeState::lastResolvedCandidates
// (MapCanvas_ScenarioEditMode_State_UI.h).
inline void ResolveScenarioEditModeCandidates(const ScenarioEditModeResolveInput& input,
                                              const Params::ScenarioBody& body,
                                              const std::string& previewAsSlotPattern,
                                              std::vector<ScenarioEditMarkerCandidate_UI>& outCandidates) {
    outCandidates.clear();
    AppendScenarioEditModeSpawnCandidates(input, body, outCandidates);
    AppendScenarioEditModeAlloyCandidates(input, body, previewAsSlotPattern, outCandidates);
}

// _PreviewAs_UI.cpp — pure synthesis: the first (total,human,ai) triple (ascending) satisfying
// `conditions` (vacuously true when empty, MatchesScenarioConditions's own contract), rendered as
// a slot pattern with the first `human` slots 'h', the next `ai` slots 'A', the rest '-'. An
// all-'-' fallback when no triple in [0, maxArmySlotCount] satisfies it.
std::string SynthesizeScenarioPreviewAsSlotPattern(
    const std::vector<Params::ScenarioCountCondition>& conditions, int maxArmySlotCount);

} // namespace Ui
} // namespace SanmapGen
