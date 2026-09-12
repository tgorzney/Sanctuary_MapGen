// ScenarioUnitPlacementValidation_IO.h -- pure, disk-free validation of
// ScenarioBody::unitPlacements (ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md §15.14's
// new declarative type, STEP260). Layer: IO. Modelled directly on the sibling
// ScenarioSpawnIdValidation_IO.h: a report struct with a one-wording SummaryText(), plus a pure
// Validate* free function, same tier as recipe.IsValid(), never called from inside
// BuildSanmapJsonText/BuildScenarioDataLuaText. Warn-only throughout -- Constitution's never-abort-
// parsing posture -- so this file never drops a placement row; there is no companion "Fixup" unit
// (unlike ScenarioSpawnIdValidation_IO's own pool/duplicate fix-ups) because nothing here is ever
// dropped or rewritten.
#pragma once
#include <string>
#include <vector>
#include "ScenarioSpawnIdValidation_IO.h"   // reuses Io::ArmyIdentityTransformLookup -- do not
                                             // invent a second interface (STEP260's own explicit rule)

namespace SanmapGen {
namespace Params { struct Scenarios; struct ScenarioBody; struct ScenarioUnitPlacement; }
namespace Io {

struct ScenarioUnitPlacementValidationReport {
    struct Violation {
        std::string descriptor;   // the scenario's own name (or a fallback for an unnamed one)
        std::string detail;       // the offending templateIdentifier/armyName, as-authored
        std::string reason;       // one full sentence
    };
    std::vector<Violation> violations;
    bool AllValid() const { return violations.empty(); }
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Walks every ScenarioBody's unitPlacements (PatternScenarios, CountScenarios, DefaultScenario) for:
// (a) an empty templateIdentifier, (b) an armyName that does not resolve against the live roster
// (via `armyDefaults`, or a shape-only ARMY_XX check when `armyDefaults` is nullptr). Both are
// warn-only -- a violation is REPORTED but the placement row is never dropped, mirroring the props
// `blueprintPath` lesson (an unresolved tpId must never abort parsing; the runtime already handles
// it gracefully via `pcall(CreateUnit, ...)`). No duplicate/uniqueness check -- placement rows are
// independent; two placements at the same or different position are both legal (multiple units
// spawn at once in the real reference file). Pure/read-only, touches no disk, never called from
// inside BuildSanmapJsonText/BuildScenarioDataLuaText.
ScenarioUnitPlacementValidationReport ValidateScenarioUnitPlacements(
    const Params::Scenarios& scenarios, const ArmyIdentityTransformLookup* armyDefaults);

} // namespace Io
} // namespace SanmapGen
