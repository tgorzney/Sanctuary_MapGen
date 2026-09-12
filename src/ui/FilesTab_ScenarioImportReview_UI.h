// FilesTab_ScenarioImportReview_UI.h — STEP266's human-gated wiring step for the three
// non-auto-attached candidate lists STEP265's `Io::ImportFullScenarioDataFromScenarioScriptFile`
// surfaces (match conditions / slot patterns / unit placements). Layer: UI.
// `ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part B rules that wiring an
// extracted shape to a scenario is "a separate human authoring action" — nothing in this module is
// ever auto-selected or auto-applied; every field below is written ONLY by an explicit combo pick
// or button click (FilesTab_ScenarioImportReview_Draw_UI.cpp), never inferred.
//
// PLACEMENT NOTE (ticket §2 left this to the coder): lives in the Files-tab module, not a new
// Scenarios-tab file, because both halves of this feature already live there — `FilesTabState`
// stashes the candidates (§1), and `DrawFilesTab`/`RunFilesTabAction` already receive
// `Params::MapRecipe&` by reference, so `recipe.scenarios` needs no new cross-tab plumbing. This
// mirrors `FilesTab_ScenarioExportRow_Draw_UI.cpp`'s own precedent of a Scenario-flavored concern
// living beside the Files-tab action that drives it.
//
// Split in two for the ARCH §1.5 ceiling: the pure resolve/append/overwrite logic below is fully
// headless (FilesTab_ScenarioImportReviewAssign_UI.cpp, directly unit-testable, no imgui at all);
// the imgui composition (FilesTab_ScenarioImportReview_Draw_UI.cpp) calls into it.
#pragma once
#include <string>
#include <vector>
#include "ConfirmDialog_UI.h"
#include "Section_UI.h"
#include "../io/ScenarioScript_MatchConditionExtract_IO.h"
#include "../io/ScenarioScript_SlotPatternExtract_IO.h"
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Ui {

// The last full-data import's three candidate lists, plus each row's own "which scenario is
// currently picked in its target combo" scratch and the slot-pattern overwrite confirm gate.
// Wholesale-replaced by ResetScenarioImportReviewState — never merged onto a stale previous
// import's leftovers (mirrors RunOpenSanmap's own fresh-scratch-then-commit posture).
struct ScenarioImportReviewState {
    SectionState section;

    std::vector<Io::ScenarioMatchConditionCandidate>    matchConditionCandidates;
    std::vector<Io::ScenarioSlotPatternExtractionEntry> slotPatternCandidates;
    std::vector<Params::ScenarioUnitPlacement>          unitPlacementCandidates;

    // Index-parallel to the two vectors above — -1 = "no target picked yet" (never auto-selected,
    // per ARCH_15_14 Part B). The unit-placement batch has exactly one shared picker (§2: "the
    // whole batch at once ... coder's call, but batch-append is the simpler v1").
    std::vector<int> matchConditionTargetComboIndex;
    std::vector<int> slotPatternTargetComboIndex;
    int              unitPlacementTargetComboIndex = -1;

    // §2's destructive-overwrite confirm gate: which slot-pattern candidate row is pending
    // confirmation right now (-1 = none), one at a time — mirrors FilesTabState's own
    // bConfirmActionPending/pendingConfirmAction single-pending-action posture.
    int                pendingSlotPatternOverwriteRow = -1;
    ConfirmDialogState slotPatternOverwriteConfirm;
};

// Replaces every candidate/scratch field above wholesale with a fresh import's results (fresh -1
// combo scratch, sized to match) — called once by RunImportScenarioFullData
// (FilesTab_Actions_UI.cpp), never merged onto a stale previous import's leftovers.
void ResetScenarioImportReviewState(
    ScenarioImportReviewState& state,
    std::vector<Io::ScenarioMatchConditionCandidate> matchConditionCandidates,
    std::vector<Io::ScenarioSlotPatternExtractionEntry> slotPatternCandidates,
    std::vector<Params::ScenarioUnitPlacement> unitPlacementCandidates);

// A target-scenario combo is drawn as ONE flattened list — every patternScenarios row, then every
// countScenarios row, then exactly one defaultScenario row, in that order (matches
// BuildScenarioImportReviewTargetLabels in the Draw unit) — this decodes a combo's raw index back
// into which one that was. Out-of-range decodes to `None`/index -1 rather than reading off the end
// (Constitution §6).
enum class ScenarioImportReviewTargetKind { None, Pattern, Count, Default };
struct ScenarioImportReviewTarget {
    ScenarioImportReviewTargetKind kind = ScenarioImportReviewTargetKind::None;
    int index = -1;   // meaningful for Pattern/Count only
};
ScenarioImportReviewTarget ResolveScenarioImportReviewTarget(const Params::Scenarios& scenarios,
                                                              int comboIndex);

// Resolves a target down to the concrete record a given shape's Append/Set button needs — nullptr
// when the picked target's kind doesn't carry that field (§2: e.g. a Pattern/Default target has no
// `conditions`) or the index is stale (a scenario was deleted since the combo was last drawn).
Params::ScenarioBody*   ScenarioImportReviewTargetBody(Params::Scenarios& scenarios,
                                                       const ScenarioImportReviewTarget& target);
Params::CountScenario*   ScenarioImportReviewTargetCountScenario(Params::Scenarios& scenarios,
                                                                 const ScenarioImportReviewTarget& target);
Params::PatternScenario* ScenarioImportReviewTargetPatternScenario(Params::Scenarios& scenarios,
                                                                    const ScenarioImportReviewTarget& target);

// §2's additive append: the whole condition-set becomes new trailing entries in `target.conditions`,
// verbatim order — every OTHER scenario's own conditions are untouched (the caller already resolved
// which CountScenario this is).
void AppendScenarioMatchConditionsToCountScenario(
    Params::CountScenario& target, const std::vector<Params::ScenarioCountCondition>& conditions);

// §2's destructive-overwrite gate: true when `target.slotPattern` already holds authored data, so
// the caller must route through a confirm dialog before calling
// SetScenarioSlotPatternOnPatternScenario below — never silently overwrite on the first click.
bool ScenarioSlotPatternOverwriteNeedsConfirmation(const Params::PatternScenario& target);

// §2's destructive overwrite itself — always overwrites, unconditionally; the confirmation gate
// above is the caller's job, not this function's.
void SetScenarioSlotPatternOnPatternScenario(Params::PatternScenario& target,
                                             const std::string& slotPattern);

// §2's additive append, generic over any ScenarioBody (Pattern/Count/Default all qualify, "any of
// CountScenario/PatternScenario/default, all have body.unitPlacements") — the whole placement batch
// becomes new trailing entries in `target.unitPlacements`, verbatim order.
void AppendScenarioUnitPlacementsToBody(Params::ScenarioBody& target,
                                        const std::vector<Params::ScenarioUnitPlacement>& placements);

// --- draw helpers, split across three FilesTab_ScenarioImportReview*_Draw_UI.cpp translation
// units for the ARCH §1.5 ceiling; declared once here rather than duplicated per file ---

// ONE flattened option list shared by every row's own combo — patternScenarios, then
// countScenarios, then exactly one defaultScenario entry, matching
// ResolveScenarioImportReviewTarget's own decode order exactly.
std::vector<std::string> BuildScenarioImportReviewTargetLabels(const Params::Scenarios& scenarios);
// One combo, drawn against that flattened label list — the shared shape every row below uses.
void DrawScenarioImportReviewTargetCombo(const std::vector<std::string>& targetLabels,
                                         int& targetComboIndex);

// Match-conditions subsection (FilesTab_ScenarioImportReviewMatchConditions_Draw_UI.cpp).
void DrawScenarioImportReviewMatchConditionRows(ScenarioImportReviewState& state,
                                                Params::Scenarios& scenarios,
                                                const std::vector<std::string>& targetLabels);
// Slot-patterns subsection + its own destructive-overwrite confirm dialog
// (FilesTab_ScenarioImportReviewSlotPatterns_Draw_UI.cpp).
void DrawScenarioImportReviewSlotPatternRows(ScenarioImportReviewState& state,
                                             Params::Scenarios& scenarios,
                                             const std::vector<std::string>& targetLabels);
void DrawScenarioImportReviewSlotPatternOverwriteConfirmDialog(ScenarioImportReviewState& state,
                                                                Params::Scenarios& scenarios);
// Unit-placements subsection — one shared batch row (FilesTab_ScenarioImportReview_Draw_UI.cpp).
void DrawScenarioImportReviewUnitPlacementRow(ScenarioImportReviewState& state,
                                              Params::Scenarios& scenarios,
                                              const std::vector<std::string>& targetLabels);

// Draws the whole review/assign panel: one subsection per shape, each row a label + a
// target-scenario combo + its Append/Set button (§2), plus the slot-pattern overwrite confirm
// dialog. `recipe.scenarios` is mutated in place ONLY on an explicit button click (and only past its
// own confirm step for the destructive slot-pattern case), never implicitly. Lives on the Files tab,
// immediately reachable after the import button that populates `state` — see this header's own
// PLACEMENT NOTE above (FilesTab_ScenarioImportReview_Draw_UI.cpp).
void DrawScenarioImportReviewSection(ScenarioImportReviewState& state, Params::MapRecipe& recipe);

} // namespace Ui
} // namespace SanmapGen
