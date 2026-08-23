// ScenariosTab_UI.h — the Scenarios tab shell: state, and the pure/cross-file API the six
// ScenariosTab_*_UI.cpp translation units share. Layer: UI. Accuracy class: Visual/Exact.
// STEP74 (`ARCH_15_05_ParamsScenariosType.md` §15.5 / `ARCH_15_06_CountScenariosOrdering.md` §15.6 /
// `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10). Supersedes `DESIGN_ScenariosTabAndLuaEditor_R1.md`
// §1-§6 on every type name/shape: authored against the ratified `Params::Scenarios` shape verbatim,
// no new PARAMS type.
//
// Split by concern: ScenariosTab_Lists_UI.cpp (three-tier list management, the entry point
// DrawScenariosTab), ScenariosTab_MatchRules_UI.cpp (Tier1 slot-pattern / Tier2 clause editors +
// reachability), ScenariosTab_SpawnsWarning_UI.cpp (the mandatory-spawns risk, three visibility
// tiers), ScenariosTab_Detail_UI.cpp/ScenariosTab_DetailAlloys_UI.cpp (the shared ScenarioBody field
// editor — split in two for the ARCH §1.5 ceiling), ScenariosTab_Matrix_UI.cpp (the live composition
// preview) and ScenariosTab_Settings_UI.cpp (maxArmySlotCount authoring). Every signature crossing
// one of those file boundaries — the only reason this header exists rather than several tiny ones —
// is declared once, here.
//
// `previewDriver` is accepted by DrawScenariosTab for interface parity with every other tab but is
// NEVER called: `recipe.scenarios` feeds no PROC stage, so no edit here trips a dirty flag or
// requests a regen (Backend policy: N/A).
#pragma once
#include <string>
#include <vector>
#include "ConfirmDialog_UI.h"
#include "LuaCodeEditor_UI.h"
#include "Section_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

enum class ScenarioSelectedTier { None, Pattern, Count, Default };

struct ScenariosTabState {
    SectionState settingsSection;      // maxArmySlotCount + its warning banner
    SectionState patternSection;       // Tier 1 DraggableList
    SectionState countSection;         // Tier 2 DraggableList
    SectionState defaultSection;       // Tier 3 fixed panel
    SectionState matrixSection;        // live composition preview

    // STEP77 — the file-based runtime-script editor (Fix §2; the correction over
    // DESIGN_ScenariosTabAndLuaEditor_R1.md §7's buffer-in-PARAMS assumption, this ticket's own
    // top-of-file note). `runtimeScriptSection` is collapsed by default — "advanced".
    SectionState       runtimeScriptSection{false};
    LuaCodeEditorState  runtimeScriptEditor;
    bool                bRuntimeScriptLoaded = false;   // one-shot load-on-first-open guard
    ConfirmDialogState  runtimeScriptResetConfirm;       // "Reset to bundled default"'s own gate
    bool                bRuntimeScriptDiffBannerDismissed = false;   // re-armed on each fresh load
    bool                bViewingBundledDefaultPanel       = false;   // "[View Bundled Default]"
    // Io::LoadScenarioRuntimeText's OWN resolution outcome (neither bundled nor override readable)
    // — DISTINCT from runtimeScriptEditor's Sys::CheckLuaSyntax result: a file that failed to
    // resolve was never even handed to the syntax checker.
    bool                bRuntimeScriptResolutionSucceeded = true;
    std::string         runtimeScriptResolutionAdvisory;
    // Caller-owned pointer/copy into Application-level machine-local settings (STEP64/§5) — same
    // nullable posture as FilesTabState's own pair; nullptr degrades to "not configured" safely.
    std::string*        scenarioRuntimeOverridePath = nullptr;
    std::string         scenarioRuntimeResourceDirectory;   // resolved once at startup, copied

    ScenarioSelectedTier selectedTier  = ScenarioSelectedTier::None;
    int                  selectedIndex = -1;   // position in patternScenarios/countScenarios;
                                               // ignored when selectedTier == Default
};

// --- pure helpers, no imgui, testable headless ---
inline std::string NextScenarioName(int existingCount) { return NextUniqueLabel("Scenario", existingCount); }
inline const char* ScenarioRowLabel(const Params::ScenarioBody& body) {
    return body.name.empty() ? "Scenario" : body.name.c_str();
}

// Correction 1 (no `ScenarioSpawnsPolicy` in ratified PARAMS): a Tier 1/2 scenario risks the
// live-documented 2h1ai regression (MAP_SCENARIO_SPEC.md §6) when it has no explicit spawns AND has
// not documented that as intentional in its own authoringNote (the exact mechanism §15.5 ratified
// `authoringNote` FOR). Tier 3 (defaultScenario) is NEVER flagged by this function — its own empty
// `spawns` correctly means "inherit the .sanmap baseline" — callers skip it explicitly (Fix §4).
inline bool ScenarioNeedsSpawnsAcknowledgment(const Params::ScenarioBody& body) {
    return body.spawns.empty() && body.authoringNote.empty();
}

// Fix §4's export-time gate predicate, exported publicly so STEP77 calls it rather than duplicating
// the rule: true when ANY Tier 1/2 scenario needs acknowledgment (Tier 3 excluded, see above). Not
// wired to any dialog here.
inline bool AnyScenarioNeedsSpawnsAcknowledgment(const Params::Scenarios& scenarios) {
    for (const Params::PatternScenario& scenario : scenarios.patternScenarios)
        if (ScenarioNeedsSpawnsAcknowledgment(scenario.body)) return true;
    for (const Params::CountScenario& scenario : scenarios.countScenarios)
        if (ScenarioNeedsSpawnsAcknowledgment(scenario.body)) return true;
    return false;
}

// Correction 2: names armies at 1-based roster position > maxArmySlotCount. 1-based roster position
// == alphabetical name order, guaranteed by STEP76's machine-minted `ARMY_XX` identity (STEP76
// ruling 4) — so this positional walk and STEP73's BuildArmyIdToNameTable's alphabetical sort
// provably return the same answer; the two tickets reconcile with no code change here.
inline std::vector<std::string> ArmiesExceedingSlotCount(const std::vector<Params::Army>& armies,
                                                         int maxArmySlotCount) {
    std::vector<std::string> affected;
    for (std::size_t index = static_cast<std::size_t>(maxArmySlotCount < 0 ? 0 : maxArmySlotCount);
         index < armies.size(); ++index)
        affected.push_back(armies[index].name.empty() ? "(unnamed army)" : armies[index].name);
    return affected;
}

// Fix §2 "clicking inside the fixed Default panel sets selectedTier = Default directly" — there is
// no per-row Select signal for a fixed single-item panel, so entering it selects it outright.
// Deliberately does NOT touch `selectedIndex` (unlike a tier-1/2 Select, which writes it) — the
// field is simply never consulted for Default, and this function proves that structurally rather
// than by inspection.
inline void SelectScenarioDefaultTier(ScenariosTabState& state) {
    state.selectedTier = ScenarioSelectedTier::Default;
}

// --- shared pure/draw API across the ScenariosTab_*_UI.cpp translation units ---
std::string ScenarioPriorityBadge(int zeroBasedRowIndex);                            // Reachability.cpp

std::string BuildSlotPatternFromToggles(const std::vector<char>& toggles);           // MatchRules.cpp
std::vector<char> ParseSlotPatternToToggles(const std::string& pattern, int maxArmySlotCount);
bool MatchesScenarioConditions(const std::vector<Params::ScenarioCountCondition>& conditions,
                               int total, int human, int ai);
bool IsCountScenarioReachable(const Params::Scenarios& scenarios, int countScenarioIndex);
bool IsDefaultScenarioReachable(const Params::Scenarios& scenarios);
// "" when reachable; " \xE2\x9A\xA0 Unreachable (shadowed by <name>)" otherwise.
// countScenarioIndex == -1 asks about defaultScenario.
std::string ScenarioReachabilityBadgeSuffix(const Params::Scenarios& scenarios, int countScenarioIndex);
void DrawSlotPatternToggleRow(std::string& slotPattern, const std::vector<Params::Army>& armies,
                              int maxArmySlotCount);
void DrawScenarioCountConditionsEditor(std::vector<Params::ScenarioCountCondition>& conditions);

void DrawScenarioBodyFields(Params::ScenarioBody& body,
                            const std::vector<Params::Army>& armies);                // Detail_UI.cpp
// alloys/alloysToAdd/alloysToRemove/navalFleet — split out of DrawScenarioBodyFields for the ARCH
// §1.5 file-size ceiling (called by it, never directly by Lists.cpp).
void DrawScenarioBodyExtendedFields(Params::ScenarioBody& body,
                                    const std::vector<Params::Army>& armies);         // DetailAlloys.cpp
// fleet / pondSideByArmy / sideBiasDistance — split out of DetailAlloys.cpp for the same ceiling.
void DrawScenarioNavalFleetFields(Params::ScenarioNavalFleet& navalFleet,
                                  const std::vector<Params::Army>& armies);           // DetailNaval.cpp

void DrawScenarioSpawnsWarningBanner(Params::ScenarioBody& body,
                                     const std::vector<Params::Army>& armies);        // SpawnsWarning.cpp

void DrawScenarioMatrix(const Params::Scenarios& scenarios, SectionState& matrixSection); // Matrix.cpp

void DrawScenarioSettings(Params::Scenarios& scenarios, SectionState& settingsSection,
                          const std::vector<Params::Army>& armies);                  // Settings.cpp

// STEP77 Fix §2 — the file-based Runtime Script editor. Reads `state.scenarioRuntimeOverridePath`/
// `scenarioRuntimeResourceDirectory` only; touches no PARAMS field (RuntimeScript.cpp).
void DrawScenarioRuntimeScriptSection(ScenariosTabState& state);
// The simplified bundled-vs-override diff banner — split out of RuntimeScript.cpp for the ARCH
// §1.5 ceiling (RuntimeScriptDiff.cpp). Called only while an override is active.
void DrawScenarioRuntimeScriptDiffBanner(ScenariosTabState& state);

// `recipe.scenarios` is edited directly; see the previewDriver note above.
void DrawScenariosTab(Params::MapRecipe& recipe, ScenariosTabState& state,
                      Pipeline::PreviewDriver* previewDriver);                       // Lists.cpp

} // namespace Ui
} // namespace SanmapGen
