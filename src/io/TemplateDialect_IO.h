// TemplateDialect_IO.h — classifies a Sys::LuaTableValue evaluation result (ticket 85's output) by
// its root-table global name across the FIVE real dialects
// (DESIGN_SantpFootprintIngestion_R1.md §1.2), extracts footprint/tpId/tags for the three that carry
// them, and detects (never silently resolves) tpId collisions across a whole ingestion run.
// Layer: IO.
//
// OUT OF SCOPE, per ARCH_18_03_CatalogDataOwnership.md's explicit deferral: economy.harvest,
// collisionInfo/collider, general.displayName — narrower than the design doc's own §7 one-line
// summary, which this ARCH ruling supersedes (see STEP87's own "Scope correction" section).
#pragma once
#include "../sys/LuaTableValue_SYS.h"
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Sys { struct LuaTableEvaluateResult; }
namespace Io {

enum class TemplateDialectKind {
    Unrecognized,           // evaluation failed, or no known root-table global was set
    UnitTemplate,            // root table `UnitTemplate`             — carries footprint
    PropTemplateLowercase,   // root table `propTemplate` (Dialect A) — carries footprint
    PropTemplateUppercase,   // root table `PropTemplate` (Dialect B) — carries footprint
    ProjectileTemplate,       // root table `ProjectileTemplate`       — NO footprint, skipped by design
    MarkerTemplate            // root table `MarkerTemplate`           — NO footprint, skipped by design
};

struct TemplateRecord {
    TemplateDialectKind dialectKind = TemplateDialectKind::Unrecognized;
    std::string         templateIdentifier;        // resolved tpId — see DeriveTemplateIdentifierFromPath
    bool                bTpIdWasDeclared = false;   // true: came from the table's own general.tpId
                                                     // field; false: fell back to the filename stem
    bool                bHasFootprint = false;
    float               baseFootprintWidth = 0.0f;
    float               baseFootprintDepth = 0.0f;
    std::vector<std::string> tags;   // top-level tags[] only — never the redundant effects[].tag
                                      // entries (DESIGN doc §1.4's "key off top-level tags" finding).
                                      // Empty when absent.
    std::string    sourceLogicalPath;
    std::uint64_t   sourceByteSize = 0;
    std::uint64_t   sourceModifiedTime = 0;
    std::uint64_t   sourceContentHash = 0;
    int             sourcePriorityRank = 0;   // ticket 86's TemplateSourceRank, carried through as a
                                               // plain int (this file has no dependency on ticket 86's
                                               // header — see this ticket's own header note)
};

// bRecognized == false covers BOTH "evaluation of the source text failed" (a syntax/runtime error,
// or — the confirmed real case — a .sanprop file that is actually JSON, DESIGN doc §1.5 trap 1) AND
// "evaluation succeeded but no known root-table global was set." diagnosticMessage distinguishes the
// two, but BOTH are ordinary, non-alarming skips (DESIGN doc §4.4), never a hard failure.
struct TemplateParseOutcome {
    bool           bRecognized = false;
    std::string    diagnosticMessage;
    TemplateRecord record;   // meaningful only when bRecognized == true
};

// Derives a tpId from a source path's filename stem (e.g. ".../edbm0149/edbm0149.santp" -> "edbm0149"),
// the SAME operation this file uses internally as its own fallback; exposed publicly so ticket 92 can
// derive a tpId from a PropInstanceGroup::blueprintPath (a manually-authored path this pipeline never
// evaluates) without duplicating this string logic. Reads the LAST path segment only, ignoring
// everything before it — safe for both real filesystem paths and ticket 86's synthetic
// "<sanpackPath>!<entryName>" logical paths.
std::string DeriveTemplateIdentifierFromPath(const std::string& path);

// Root-table detection (STEP87 §2.1), promoted out of this file's own .cpp (Auto-NavMesh
// mesh-ingestion work) so a second consumer (TemplateVisualLod_IO — a new additive sibling file)
// can classify an already-evaluated table by the same five-dialect rule without re-deriving it.
// Checks the five recognized dialect globals in declaration order, first hit wins; returns nullptr
// (dialectKind left Unrecognized) when none match.
const Sys::LuaTableValue* DetectTemplateRootTable(const Sys::LuaTableValue& globals,
                                                  TemplateDialectKind& dialectKind);

// One file's already-evaluated result in, one outcome out — pure, stateless, safe to call from any
// thread (ticket 89's ThreadPool fan-out). sourceLogicalPath/sourcePriorityRank/sourceByteSize/
// sourceModifiedTime/sourceContentHash are carried through verbatim onto the returned record.
TemplateParseOutcome ParseTemplateSource(const Sys::LuaTableEvaluateResult& evaluated,
                                          const std::string& sourceLogicalPath,
                                          int sourcePriorityRank,
                                          std::uint64_t sourceByteSize,
                                          std::uint64_t sourceModifiedTime,
                                          std::uint64_t sourceContentHash);

struct TpIdCollision {
    std::string              templateIdentifier;
    std::vector<std::string> conflictingSourcePaths;   // every source that declared this tpId, in
                                                         // discovery order — never silently dropped
};
struct TpIdCollisionReport {
    std::vector<TpIdCollision> collisions;
    bool AnyCollisions() const { return !collisions.empty(); }
};

// READ-ONLY diagnostic over an already-parsed record list — does NOT mutate `records` and does NOT
// decide a winner (ticket 89's own first-write-wins fold, sorted by sourcePriorityRank, resolves the
// collision; this function only reports one occurred — see STEP87's Q7 resolution). A tpId
// appearing in >1 record is reported regardless of dialect kind: an unexpected cross-kind collision
// is exactly the kind of thing that must surface, not be assumed away.
TpIdCollisionReport DetectTpIdCollisions(const std::vector<TemplateRecord>& records);

} // namespace Io
} // namespace SanmapGen
