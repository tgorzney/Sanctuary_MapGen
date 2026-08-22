# STEP87 — `TemplateDialect_IO`: classify the five dialects, extract footprint/tpId/tags, detect collisions

**Layer:** IO. **Domain:** `.santp`/`.sanprop` template parsing/classification. **Sequence:** ticket 3
of 8 (85–92). **Real dependency:** ticket 85 (`Sys::LuaTableValue`/`Sys::LuaTableEvaluateResult` types).
Does **not** `#include` ticket 86's header — `ParseTemplateSource` takes plain scalars, not a
`TemplateSourceFile`, deliberately decoupling the two (ticket 89 is what positionally glues 86's
output into this function's input). No dependency on 88/89 (ticket 89 depends on this, not the
reverse).

**⚠️ Correction 2026-08-22 — real compile-blocking error in an earlier draft of this ticket, fixed.**
An earlier draft claimed `TemplateDialect_IO.h` "`#include`s `LuaTableValue_SYS.h` only, never
`LuaTableEvaluate_SYS.h`/`.cpp`." That is false and does not compile: `LuaTableEvaluateResult` (with
its `bSucceeded`/`errorMessage`/`globals` members, which `ParseTemplateSource`'s own implementation
contract below dereferences directly) is defined in `LuaTableEvaluate_SYS.h`, NOT in
`LuaTableValue_SYS.h` (which STEP85 scopes to `LuaTableValueKind`/`LuaTableValue` only). A forward
declaration — what the `.h` below correctly uses for its function *signature* — is not sufficient for
the `.cpp`'s member access. **`TemplateDialect_IO.cpp` MUST `#include "../sys/LuaTableEvaluate_SYS.h"`**
— the header stays forward-declaration-only (no change needed there, and it still never calls
`Sys::EvaluateLuaTableSource` itself, per this ticket's actual scope), but the source file needs the
complete type. Do not remove that include thinking it contradicts this ticket's "never calls the
evaluator" framing — *calling* the evaluator and *including its header for the complete result type*
are different things; this ticket does the latter, not the former.

## Root problem
`DESIGN_SantpFootprintIngestion_R1.md` §1.2 (re-verified this session, full document read) establishes
**five** root-table dialects across the corpus, not two — `UnitTemplate` (283 files),
`propTemplate` lowercase / Dialect A (145), `PropTemplate` capital / Dialect B (4),
`ProjectileTemplate` (62, no `footprint`), `MarkerTemplate` (6, no `footprint`). §1.2's own footnote
is load-bearing: the root assignment is **not reliably on line 1 or 2** (12 real `UnitTemplate` files
carry a multi-line comment header before it) — a line-anchored or regex-based reader is wrong from the
start, which is exactly why ticket 85 hands this ticket a fully-evaluated `Sys::LuaTableValue` tree
rather than raw text.

## ⚠️ Scope correction versus the design doc's own §7 one-liner
`DESIGN_SantpFootprintIngestion_R1.md` §7's ticket-87 row says "extract footprint/tpId/tags/collision
box." **`ARCH_18_03_CatalogDataOwnership.md` §18.3 supersedes this**: `economy.harvest`,
`collisionInfo`/`collider`, and `general.displayName` are explicitly deferred to the not-yet-scoped
texture/asset importer, ruled OUT OF SCOPE for tickets 89 and 92 (and therefore for this ticket, their
shared upstream parser). This ticket extracts **footprint + tpId + tags only** — no collision box.

## Fix

### 1. New file: `src/io/TemplateDialect_IO.h`
```cpp
// TemplateDialect_IO.h — classifies a Sys::LuaTableValue evaluation result (ticket 85's output) by
// its root-table global name across the FIVE real dialects
// (DESIGN_SantpFootprintIngestion_R1.md §1.2), extracts footprint/tpId/tags for the three that carry
// them, and detects (never silently resolves) tpId collisions across a whole ingestion run.
// Layer: IO.
//
// OUT OF SCOPE, per ARCH_18_03_CatalogDataOwnership.md's explicit deferral: economy.harvest,
// collisionInfo/collider, general.displayName — narrower than the design doc's own §7 one-line
// summary, which this ARCH ruling supersedes (see this ticket's own "Scope correction" section).
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
// collision; this function only reports one occurred — see the Q7 resolution below). A tpId
// appearing in >1 record is reported regardless of dialect kind: an unexpected cross-kind collision
// is exactly the kind of thing that must surface, not be assumed away.
TpIdCollisionReport DetectTpIdCollisions(const std::vector<TemplateRecord>& records);

} // namespace Io
} // namespace SanmapGen
```

### 2. New file: `src/io/TemplateDialect_IO.cpp` — implementation contract
1. **Root-table detection.** If `!evaluated.bSucceeded` → `{bRecognized=false, diagnosticMessage =
   "evaluation failed: " + evaluated.errorMessage}`. Else check, in order,
   `evaluated.globals.Find("UnitTemplate")`, `Find("propTemplate")`, `Find("PropTemplate")`,
   `Find("ProjectileTemplate")`, `Find("MarkerTemplate")` — first hit sets `dialectKind`
   accordingly. None found → `{bRecognized=false, diagnosticMessage = "no recognized root table
   (possibly non-Lua content, e.g. a JSON .sanprop)"}`.
2. **`ProjectileTemplate`/`MarkerTemplate`** → `bRecognized = true`, `record.dialectKind` set,
   `bHasFootprint = false`, tpId resolved per step 3 below (still useful for diagnostics/counting even
   though these are skipped by design downstream), no footprint/tags extraction attempted.
3. **tpId resolution** (`UnitTemplate`/`PropTemplateLowercase`/`PropTemplateUppercase` — also applied
   to Projectile/Marker for consistency): look up `rootTable->Find("general")` →
   `Find("tpId")`; if that value is `Text`-kind and non-empty, use it verbatim
   (`bTpIdWasDeclared = true`). Otherwise `DeriveTemplateIdentifierFromPath(sourceLogicalPath)`
   (`bTpIdWasDeclared = false`).
4. **Footprint extraction** (the three carrying kinds only): `rootTable->Find("footprint")` →
   `Find("x")`/`Find("y")`, each `AsNumber(0.0)` cast to `float`. If `footprint` itself is absent
   entirely — an anomaly for these three dialects per the design doc's own "every UnitTemplate/
   propTemplate/PropTemplate file carries footprint" finding — `bHasFootprint = false`,
   `diagnosticMessage = "recognized dialect but footprint field is missing"` (still
   `bRecognized = true`; ticket 89 counts this separately, never silently treated as 0×0).
5. **Tags extraction** (the three carrying kinds only): `rootTable->Find("tags")` → if `Array`-kind,
   append each `Text`-kind element's `text` to `record.tags`, skipping any non-text element silently
   (Constitution §6 — never crash on a malformed field). Absent `tags` → empty vector, not an error.
6. `DeriveTemplateIdentifierFromPath`: take the text after the last `/` or `\`, strip a trailing
   `.santp`/`.sanprop` (case-insensitive), return the remainder — implementable via
   `std::filesystem::path(path).stem().string()`, which correctly ignores everything before the last
   path separator (works uniformly for both real filesystem paths and ticket 86's
   `"<sanpackPath>!<entryName>"` synthetic logical paths, since `!` is ordinary text preceding the
   final `/`-delimited component).
7. `DetectTpIdCollisions`: group `records` by `templateIdentifier`; any group with `size() > 1`
   becomes a `TpIdCollision` naming every `sourceLogicalPath` in that group, in the input vector's
   own order.

### 3. Q7 resolution (tpId collision priority) — Format Expert's own call, not an ARCH ruling
`DESIGN_SantpFootprintIngestion_R1.md` §7 Q7 flagged this as needing an ARCH ruling but
`ARCH_18_01/02/03` do not rule on it — `ARCH_18_03` explicitly delegates ticket-92's tags-table
*shape* to this expert without addressing collision *priority* at all. As the Format Expert, I resolve
this within my own domain: **option (a) from the design doc's own recommendation** — first-write-wins
by deterministic source priority (`TemplateSourceRank::LooseFile` > `UnzippedPackTree` >
`CompressedSanpack`), every collision reported loudly regardless of which entry wins. Mechanically:
`DetectTpIdCollisions` only REPORTS; **ticket 89** is what actually resolves a winner, by stable-
sorting parsed records by `sourcePriorityRank` before folding them into its own map (see STEP89).
This keeps `DetectTpIdCollisions` pure/stateless and keeps the winner-selection logic in exactly one
place. Flagged here plainly so a future reader does not mistake this for an ARCH ruling.

## Files touched
- NEW `src/io/TemplateDialect_IO.h`
- NEW `src/io/TemplateDialect_IO.cpp`
- NEW `src/io/TemplateDialect_IO_Test.cpp`
- `CMakeLists.txt` — one new `add_sangen_test(TemplateDialect_IO_Test src/io/TemplateDialect_IO_Test.cpp)`.

## Backend policy
CPU only, pure functions, no `Dispatch_SYS` involvement, no I/O of its own (operates entirely on an
already-in-memory `Sys::LuaTableEvaluateResult`). O(fields) per file, O(n log n) for the collision
sort a caller performs — trivial at this corpus size (Constitution §7, direct algorithmic inspection,
same basis STEP58/STEP96 already used).

## ARCH rules invoked
- `ARCH_18_03_CatalogDataOwnership.md` §18.3 — the explicit deferral of `economy.harvest`/
  `collisionInfo`/`collider`/`general.displayName`, which narrows this ticket's extraction scope
  below the design doc's own §7 summary (see "Scope correction" above); also the explicit delegation
  of the tags-table shape to this expert, exercised here.
- `ARCH_18_01_SandboxedExecutionPrimitive.md` §18.1 — this ticket consumes `Sys::LuaTableValue`
  exactly as that ruling specifies it crosses the SYS/IO boundary (an owned tree, no LuaJIT type).
- Constitution §6 — every extraction step degrades gracefully on a missing/malformed field; a tpId
  collision is DETECTED AND REPORTED, never silently overwritten (the design doc's own §1.5 trap 2
  and `UNIT_PROP_MARKER_DATA_SPEC.md:73-77`'s documented Pandemonium collisions, both real).
- §1 naming law — `templateIdentifier` fully spelled (not `tpId`, except where citing the game's own
  scheme in comments), `_IO` suffix.

## Explicit out-of-scope
- **`economy.harvest`, `collisionInfo`/`collider`, `general.displayName`** — `ARCH_18_03`'s explicit
  deferral to the not-yet-scoped texture/asset importer.
- **Calling `Sys::EvaluateLuaTableSource` itself** — this file only consumes an already-produced
  `Sys::LuaTableEvaluateResult`; the call site is ticket 89's orchestrator.
- **File discovery** — ticket 86; this file has no dependency on its header (see this ticket's header
  note).
- **Deciding which colliding record wins** — `DetectTpIdCollisions` reports only; ticket 89 resolves
  (see the Q7 resolution above).
- **Caching** — ticket 88.
- **The load-bearing misspellings `maxVerrtices`/`positonOffset`** (`UNIT_PROP_MARKER_DATA_SPEC.md:70-71`)
  — irrelevant to footprint/tpId/tags extraction; never "corrected" anywhere in this file, noted so a
  future reader does not "fix" a key lookup this ticket never touches.

## Acceptance test
New `src/io/TemplateDialect_IO_Test.cpp` (registered in `CMakeLists.txt`), building
`Sys::LuaTableValue` fixtures directly (no live Lua evaluation needed — this file's own
"no dependency on ticket 85's `.cpp`" boundary, proven by the test needing only the `.h`):
- A fixture shaped like a real `UnitTemplate` (`general.tpId = "uca1001"`, `footprint = {x=1.2,y=1.2}`)
  parses to `bRecognized == true`, `dialectKind == UnitTemplate`, `templateIdentifier == "uca1001"`,
  `bTpIdWasDeclared == true`, `bHasFootprint == true`, `{1.2f, 1.2f}`.
- A fixture with NO `general.tpId` field, `sourceLogicalPath = ".../edbm0149/edbm0149.santp"` →
  `templateIdentifier == "edbm0149"`, `bTpIdWasDeclared == false`.
- Dialect A (`propTemplate`, lowercase) and Dialect B (`PropTemplate`, capital) fixtures both parse
  with the correct `dialectKind` and `bHasFootprint == true` — proves the reader keys off the root
  table NAME, not the presence/absence of `collider`/`collisionInfo` (which are never inspected).
- `ProjectileTemplate`/`MarkerTemplate` fixtures parse to `bRecognized == true`,
  `bHasFootprint == false`, correct `dialectKind`.
- A fixture whose evaluation itself failed (`Sys::LuaTableEvaluateResult{bSucceeded=false, ...}`) →
  `bRecognized == false`, `diagnosticMessage` mentions "evaluation failed."
- A fixture whose globals contain no recognized root-table name → `bRecognized == false`,
  `diagnosticMessage` mentions "no recognized root table."
- A recognized fixture missing its `footprint` field entirely → `bRecognized == true`,
  `bHasFootprint == false`.
- A fixture with `tags = {"HARVESTABLE", "FLAMMABLE"}` → `record.tags == {"HARVESTABLE", "FLAMMABLE"}`;
  a fixture with an `effects[].tag = "HARVESTABLE"` entry but NO top-level `tags` → `record.tags`
  empty (proves "key off top-level tags only," never the redundant `effects[]` entry).
- `DeriveTemplateIdentifierFromPath("Environment/01_Highlands/Props/edbm0149/edbm0149.santp") ==
  "edbm0149"`; the same against a synthetic `"C:/.../Environment.sanpack!Environment/01_Highlands/
  Props/edbm0149/edbm0149.santp"` also yields `"edbm0149"`.
- `DetectTpIdCollisions` over three records where two share `templateIdentifier == "Cliff_02"` (one
  declared, one derived from a differently-named file — mirroring the real `Cliff_03.sanprop`
  declaring `tpId = "Cliff_02"` case) returns exactly one `TpIdCollision` naming both source paths; a
  fourth, unrelated tpId in the same input produces zero additional collisions.
- Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; the new target passes.

## Verify
- New `src/io/TemplateDialect_IO_Test.cpp` passes.
- Grep this ticket's `.cpp`/`.h` for `economy`, `collisionInfo`, `collider`, `displayName` — must have
  zero matches (confirms the ARCH_18_03 scope correction was actually honored, not just stated).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
