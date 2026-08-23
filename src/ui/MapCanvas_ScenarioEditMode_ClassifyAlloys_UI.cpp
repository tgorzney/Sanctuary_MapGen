// MapCanvas_ScenarioEditMode_ClassifyAlloys_UI.cpp — the baseline-alloy classification
// (Kept/Deleted/RemovedGhost) plus the authored-override candidates (Added). Layer: UI. Pure,
// imgui-free, headless-testable.
//
// alloyMode-scoped, per Fix section's own state table and MAP_SCENARIO_SPEC §5:
//   - Delta: a baseline instance whose synthesized markerName matches a `body.alloysToRemove` row
//     renders AlloyRemovedGhost instead of AlloyKept; every other baseline instance stays Kept
//     (spec: "silence is NOT a delete instruction" under Delta). `body.alloysToAdd` renders as
//     AlloyAdded, its own position, independent of any baseline instance (§0).
//   - Occupancy: REASONED SIMPLIFICATION, flagged (not gameplay-verified) — SanGen bakes no
//     per-instance army identity for Alloy markers (§0), so "delete markers for armies with no
//     player" cannot be tested per real instance directly. This treats each baseline instance's
//     own rule-relative bucket position as a stand-in "seat" and compares it against the
//     Preview-As pattern's filled-slot count: a seat at or past that count renders AlloyDeleted.
//   - Explicit/KeepAll: baseline always renders Kept (context only) — Explicit's delete/keep
//     semantics are army-roster-scoped (MAP_SCENARIO_SPEC §5), not baked-instance-scoped, so this
//     module cannot honestly attribute a "deleted" state to one specific baseline instance under
//     Explicit; `body.alloys` (Explicit's own override list) renders as AlloyAdded instead.
#include "MapCanvas_ScenarioEditMode_UI.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

int FindAlloyRemovalRow(const std::vector<Params::ScenarioAlloyRemoval>& removals, const std::string& markerName) {
    for (std::size_t index = 0; index < removals.size(); ++index)
        if (removals[index].markerName == markerName) return static_cast<int>(index);
    return kScenarioEditModeNoIndex;
}

int FilledSlotCountOf(const std::string& previewAsSlotPattern) {
    int count = 0;
    for (const char slotCharacter : previewAsSlotPattern)
        if (slotCharacter == 'h' || slotCharacter == 'A') ++count;
    return count;
}

int ResolveArmyIndexByName(const std::vector<Params::Army>* armies, const std::string& armyName) {
    if (armies == nullptr) return kScenarioEditModeNoIndex;
    for (std::size_t index = 0; index < armies->size(); ++index)
        if ((*armies)[index].name == armyName) return static_cast<int>(index);
    return kScenarioEditModeNoIndex;
}

void AppendBaselineAlloyCandidates(const ScenarioEditModeResolveInput& input, const Params::ScenarioBody& body,
                                   const std::string& previewAsSlotPattern,
                                   std::vector<ScenarioEditMarkerCandidate_UI>& outCandidates) {
    std::vector<ScenarioEditModeBaselineInstance_UI> baseline;
    ResolveScenarioEditModeBaselineAlloys(input, baseline);
    const int filledSlotCount = FilledSlotCountOf(previewAsSlotPattern);

    for (std::size_t index = 0; index < baseline.size(); ++index) {
        const ScenarioEditModeBaselineInstance_UI& instance = baseline[index];
        ScenarioEditMarkerCandidate_UI candidate;
        candidate.kind = ScenarioMarkerKind_UI::Alloy;
        candidate.worldX = instance.worldX; candidate.worldY = instance.worldY; candidate.worldZ = instance.worldZ;
        candidate.templateIdentifier = instance.templateIdentifier;
        candidate.markerName = instance.markerName;

        const int removalRow = FindAlloyRemovalRow(body.alloysToRemove, instance.markerName);
        if (body.alloyMode == Params::ScenarioAlloyMode::Delta && removalRow != kScenarioEditModeNoIndex) {
            candidate.state = ScenarioMarkerVisualState_UI::AlloyRemovedGhost;
            candidate.alloyRemovalRowIndex = removalRow;
        } else if (body.alloyMode == Params::ScenarioAlloyMode::Occupancy
                   && static_cast<int>(index) >= filledSlotCount) {
            candidate.state = ScenarioMarkerVisualState_UI::AlloyDeleted;
        } else {
            candidate.state = ScenarioMarkerVisualState_UI::AlloyKept;
        }
        outCandidates.push_back(candidate);
    }
}

void AppendAlloyOverrideCandidates(const std::vector<Params::ScenarioAlloyOverride>& overrides, bool bIsAdd,
                                   const std::vector<Params::Army>* armies,
                                   std::vector<ScenarioEditMarkerCandidate_UI>& outCandidates) {
    for (std::size_t index = 0; index < overrides.size(); ++index) {
        const Params::ScenarioAlloyOverride& entry = overrides[index];
        ScenarioEditMarkerCandidate_UI candidate;
        candidate.kind = ScenarioMarkerKind_UI::Alloy;
        candidate.state = ScenarioMarkerVisualState_UI::AlloyAdded;
        candidate.worldX = entry.positionX; candidate.worldY = entry.positionY; candidate.worldZ = entry.positionZ;
        candidate.markerName = entry.markerName;
        candidate.armyIndex = ResolveArmyIndexByName(armies, entry.armyName);
        candidate.alloyOverrideRowIndex = static_cast<int>(index);
        candidate.bAlloyOverrideIsAdd = bIsAdd;
        outCandidates.push_back(candidate);
    }
}

} // namespace

void AppendScenarioEditModeAlloyCandidates(const ScenarioEditModeResolveInput& input,
                                           const Params::ScenarioBody& body,
                                           const std::string& previewAsSlotPattern,
                                           std::vector<ScenarioEditMarkerCandidate_UI>& outCandidates) {
    AppendBaselineAlloyCandidates(input, body, previewAsSlotPattern, outCandidates);
    if (body.alloyMode == Params::ScenarioAlloyMode::Explicit)
        AppendAlloyOverrideCandidates(body.alloys, false, input.armies, outCandidates);
    else if (body.alloyMode == Params::ScenarioAlloyMode::Delta)
        AppendAlloyOverrideCandidates(body.alloysToAdd, true, input.armies, outCandidates);
}

} // namespace Ui
} // namespace SanmapGen
