// ScenarioSpawnIdValidation_IO.h -- pure, disk-free validation of the spawnId pool and each
// scenario's spawnIds references (ARCH_15_12_ScenarioSpawnIdentity.md §15.12's Validator section).
// Layer: IO. SHARED by the UI (live, per-frame, non-blocking inline warning) and both export legs
// (MapExporter_Scenarios_IO.cpp / ScenarioScript_DataLua_IO.cpp) -- one validator, never two
// independently invented copies of the rule. Modelled directly on the sibling
// MapExporter_ArmySpawnMarkerValidation_IO.h/ScenarioSlotRangeValidation_IO.h: a report struct with
// a one-wording SummaryText(), plus pure Validate*/Resolve* free functions, same tier as
// recipe.IsValid(), never called from inside BuildSanmapJsonText/BuildScenarioDataLuaText.
// Implementation is split across TWO translation units purely for the ARCH §1.5 file-size ceiling:
// ScenarioSpawnIdValidation_IO.cpp (ResolveSpawnId/ValidateScenarioSpawnIds) and
// ScenarioSpawnIdFixup_IO.cpp (ApplyScenarioSpawnIdFixups) — both share this one small header.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct Scenarios; struct ScenarioBody; struct ScenarioSpawnPoint; }
namespace Io {

// Resolves one spawnId against the pool (first-match) then the literal-ARMY_XX fallback. Returns
// false (unresolvable) with outArmyName/outX/Y/Z left untouched — ARCH_15_12's Resolve step 3.
// `armyDefaults` is nullable (export-time validation, and this file's own pure/disk-free validator,
// run with no live GameInfo — pass nullptr and step 2 falls back to a SHAPE-ONLY check: a
// well-formed ARMY_XX-shaped spawnId is still treated as resolving to itself, position left at the
// caller's own zeroed default. STEP76 guarantees that shape really is a live roster identity
// whenever a real one exists to check against, so this never gives a false positive for the
// duplicate-armyName/unresolvable checks below, which only need the resolved armyName, never a real
// position). A real caller (e.g. a future live-map context) may pass a real lookup instead to also
// recover the actual position.
struct ArmyIdentityTransformLookup {
    // Returns true and fills outX/Y/Z if `armyName` names a real, currently-known Spawn transform.
    virtual bool Find(const std::string& armyName, float& outX, float& outY, float& outZ) const = 0;
    virtual ~ArmyIdentityTransformLookup() = default;
};
bool ResolveSpawnId(const std::string& spawnId, const std::vector<Params::ScenarioSpawnPoint>& pool,
                    const ArmyIdentityTransformLookup* armyDefaults,
                    std::string& outArmyName, float& outX, float& outY, float& outZ);

struct ScenarioSpawnIdValidationReport {
    struct Violation {
        std::string descriptor;   // "pool" or the scenario's own name (or a fallback for an unnamed one)
        std::string detail;       // the offending spawnId/armyName, as-authored
        std::string reason;       // one full sentence
    };
    std::vector<Violation> violations;
    bool AllValid() const { return violations.empty(); }
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Walks Scenarios::spawnPoints for: (a) ARMY_XX-shaped spawnId (reserved namespace,
// Io::ResemblesArmyIdentityCaseInsensitive), (b) duplicate spawnId within the pool. Then walks every
// ScenarioBody's spawnIds for: (c) two entries resolving to the same armyName within ONE scenario
// (cross-scenario reuse of the same armyName is explicitly fine, per the human's own ruling — only
// checked per-scenario), (d) an unresolvable spawnId (warn-only, never dropped). Fix-up posture
// (first-authored-wins drop for a/b/c, warn-only leave-alone for d) is applied by
// ApplyScenarioSpawnIdFixups below -- this function only REPORTS, matching
// ArmySpawnMarkerValidationReport's own posture. Pure/read-only, touches no disk, never called from
// inside BuildSanmapJsonText/BuildScenarioDataLuaText.
ScenarioSpawnIdValidationReport ValidateScenarioSpawnIds(const Params::Scenarios& scenarios);

// Produces a COPY of `scenarios` with the report's fix-ups applied: first-authored-wins duplicate/
// reserved-namespace pool rows dropped, and per-scenario spawnIds entries resolving to an armyName
// an earlier entry in the SAME scenario already resolved to are dropped. Unresolvable spawnIds are
// left AS-AUTHORED (warn-only, never dropped) — ARCH_15_12's "never a flat refusal" posture. Shared
// by both export legs (MapExporter_Scenarios_IO.cpp / ScenarioScript_DataLua_IO.cpp), each calling
// this on its own copy of `recipe.scenarios` before rendering, mirroring the STEP76
// AssignArmyIdentities re-mint-defensively pattern (export never trusts the roster is already clean).
Params::Scenarios ApplyScenarioSpawnIdFixups(const Params::Scenarios& scenarios);

} // namespace Io
} // namespace SanmapGen
