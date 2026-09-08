// ScenarioNameValidation_IO.h -- pure, disk-free validation of ScenarioBody::name against the
// filesystem-safety charset + case-insensitive cross-set uniqueness ARCH_15_05_ParamsScenariosType.md's
// "AMENDED 2026-09-03" section rules (name is now substituted directly into a category-4 Import() path,
// ARCH_15_04_ThreeFileOnDiskShape.md's "AMENDED 2026-09-03" section). Layer: IO. Modelled directly on
// the sibling MapExporter_ScenarioAreaNameValidation_IO.h: a report struct with a one-wording
// SummaryText(), plus a pure Validate* free function, same tier as recipe.IsValid(), never called
// from inside BuildSanmapJsonText/BuildScenarioDataLuaText.
//
// SHARED by the UI (live, per-frame, non-blocking inline warning) and IO (the category-4 export gate,
// ScenarioScript_CategoryFourExport_IO) -- one validator, two consumers, never two independently
// invented copies of the rule (same posture as MapExporter_ScenarioAreaNameValidation_IO.h's own
// "SHARED by both export legs" note).
//
// Runs over the WHOLE Scenarios set regardless of spawnsUnits -- see this file's own header comment in
// the work-order that authored it (STEP251 §1) for why validation is never gated on spawnsUnits even
// though the category-4 WRITE it feeds (a separate file) is.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct Scenarios; }
namespace Io {

struct ScenarioNameValidationReport {
    struct Violation {
        std::string scenarioDescriptor;   // body.name if non-empty else a tier+index fallback,
                                          // mirroring ScenarioAreaNameDescriptor's "never blank" idiom
                                          // (MapExporter_ScenarioAreaNameValidation_IO.cpp)
        std::string name;                 // the offending name itself, as-authored, byte-for-byte
        std::string reason;               // one full sentence -- see ValidateScenarioNames' three
                                          // reason texts in the .cpp
    };
    std::vector<Violation> violations;
    bool AllNamesValid() const { return violations.empty(); }
    // Returns the first violation reason recorded for `name`, or nullptr if `name` has none. The
    // per-scenario query ScenarioScript_CategoryFourExport_IO and the UI's inline warning both use
    // this instead of re-deriving the rule.
    const std::string* FindViolationReasonForName(const std::string& name) const;
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Pure/read-only, touches no disk, no Lua. Walks patternScenarios, then countScenarios, then
// defaultScenario (same tier order this file family already establishes elsewhere) -- runs over EVERY
// ScenarioBody regardless of spawnsUnits (see this header's own top note for why).
ScenarioNameValidationReport ValidateScenarioNames(const Params::Scenarios& scenarios);

} // namespace Io
} // namespace SanmapGen
