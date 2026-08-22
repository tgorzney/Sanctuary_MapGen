# STEP92 — `bReclaimable` bake from ingested tags (closes STEP62's deferred out-of-scope item)

**Layer:** PARAMS + IO (no PARAMS *file* is edited — `bReclaimable` already exists; see below).
**Domain:** the `ARCH_18_03_CatalogDataOwnership.md`-mandated human-triggered bake action for
`Params::PropRule::bReclaimable`/`Params::PropInstanceGroup::bReclaimable`. **Sequence:** ticket 8 of
8 (85–92), the last in this backlog. **Real dependency:** ticket 89 (`Io::TemplateIngestReport`,
specifically its `tagsByTemplateIdentifier` sibling table) and ticket 87
(`Io::DeriveTemplateIdentifierFromPath`, reused rather than duplicated). Also
`STEP62_ReclaimPropFilter_PARAMS.md` (real, shipped — confirmed by reading `src/params/
ScatterRule_PARAMS.h`/`PropInstance_PARAMS.h` this session: `bool bReclaimable = false;` exists on
BOTH `Params::PropRule` and `Params::PropInstanceGroup`, wire key `"Reclaimable"`, already fully
round-tripped in `MapExporter_PropsStack_IO.cpp`/`MapImporter_PropsStack_IO.cpp`/
`MapExporter_Props_IO.cpp`/`MapImporter_Props_IO.cpp`). Confirmed by grep this session: **zero**
references to `bReclaimable`/`Reclaimable` exist anywhere under `src/ui/` — no checkbox/widget has
ever been built for this field, on either type, matching STEP62's own explicit out-of-scope note.

## Root problem
`STEP62_ReclaimPropFilter_PARAMS.md`'s own "Explicit out-of-scope" list already named this exact
follow-up: *"Auto-populating `bReclaimable` from a future blueprint `tags`/`HARVESTABLE` import —
whether the auto-populated value stays overridable or becomes a live derived field is a question for
whoever builds that importer."* `ARCH_18_03_CatalogDataOwnership.md` §18.3 answers that question
directly: **stays overridable**, via the same one-shot, human-triggered bake mechanism
`ARCH_18_02_IngestedDataDeterminism.md` §18.2 mandates for footprint — never a live derived field,
never automatic. §18.3 also delegates the tags-table's exact shape to this expert: ticket 89 already
supplies it (`TemplateIngestReport::tagsByTemplateIdentifier`/`FindTagsByTemplateIdentifier`), so this
ticket needs no new standalone tags-storage file — see "Design note" below.

## Design note: this ticket is PARAMS+IO only — no UI, following STEP62's own precedent
`STEP62_ReclaimPropFilter_PARAMS.md`'s own out-of-scope list explicitly deferred "any UI checkbox/
widget exposing `bReclaimable` for authoring... not requested by this ticket" — a deferral this
ticket continues rather than resolves. This ticket ships a **callable, pure bake function**, mirroring
`STEP58`'s own "ship the reusable piece, defer the call site" precedent
(`Application_AssetBridge_UI.h` wiring "is STEP51's or STEP52's job at their own dispatch time, not
invented here" — applied identically here to a future bReclaimable-authoring ticket). This resolves
the apparent tension with `ARCH_18_03`'s "reads it exactly once, at the same human-triggered bake
action §18.2 already mandates": the CONTRACT this ticket ships (must be called explicitly, never wired
into any automatic/continuous path) is what makes a future click-driven UI call site correct when it
eventually lands — this ticket is not required to build that call site itself.

## Fix

### 1. New file: `src/io/PropReclaimableBake_IO.h`
```cpp
// PropReclaimableBake_IO.h — the ARCH_18_03_CatalogDataOwnership.md-mandated human-triggered bake
// action for Params::PropRule::bReclaimable / Params::PropInstanceGroup::bReclaimable
// (STEP62_ReclaimPropFilter_PARAMS.md, real, shipped), mirroring
// STEP96_FootprintBakeAndStalenessCheck_IO.md's footprint bake mechanism against the SAME
// Io::TemplateIngestReport (ticket 89) but populating a bool from ingested tags instead of a float
// pair. Layer: IO (depends on PARAMS types, never the reverse — Constitution §1).
//
// NO UI CALL SITE SHIPS IN THIS TICKET — see "Design note" in the work-order. Wiring a button that
// calls this is deferred, the same posture STEP58 and STEP62 already established for their own
// consumer wiring. This file changes NOTHING about bReclaimable's field, wire key, or partition
// semantics (STEP62's own field is untouched, unedited, uneditable by this ticket's own design).
#pragma once
#include "TemplateIngest_IO.h"
#include <string>

namespace SanmapGen { namespace Params { struct PropRule; struct PropInstanceGroup; } }

namespace SanmapGen {
namespace Io {

// True iff templateIdentifier's ingested tags (report.FindTagsByTemplateIdentifier) contain the
// literal string "HARVESTABLE" — DESIGN_SantpFootprintIngestion_R1.md §1.4's confirmed real tag
// spelling, keyed off top-level tags[] (never the redundant effects[].tag entries, ticket 87's own
// finding, carried through unchanged). Returns false when templateIdentifier was never ingested
// this session (report has no entry for it) — this is NOT the same as "known false"; see the two
// bake functions below for how that distinction is actually used.
bool TemplateHasHarvestableTag(const TemplateIngestReport& report, const std::string& templateIdentifier);

// Returns false (rule.bReclaimable UNTOUCHED) when templateIdentifier was never ingested this
// session — never clears a hand-authored `true` just because ingestion doesn't (yet) know about it
// (ARCH_18_02_IngestedDataDeterminism.md §18.2 rules 3/4, applied to this sibling field exactly as
// §18.3 mandates). Returns true (rule.bReclaimable OVERWRITTEN, one-shot, on this explicit call)
// otherwise — including overwriting a previously-true value back to false when the ingested tags
// no longer contain HARVESTABLE, since that IS the point of a bake: it reflects what was just found,
// not a one-directional "only ever sets true" ratchet.
bool BakeReclaimableForPropRule(const TemplateIngestReport& report, Params::PropRule& rule);

// PropInstanceGroup has no templateIdentifier field — its identity is blueprintPath (a manually-
// authored path, e.g. "Environment/01_Highlands/Props/edbm0149/edbm0149.santp"). The tpId is
// derived via Io::DeriveTemplateIdentifierFromPath (ticket 87, the SAME filename-stem operation the
// ingestion pipeline itself uses as its own fallback) — NEVER synthesized the other direction (a
// path FROM a tpId), which the design doc's own "prop folder naming is inconsistent across biome
// sets" finding forbids; this only ever reads an ALREADY-AUTHORED path backward to a tpId, the safe
// direction (the same direction this expert's charter names as the one that must never be
// synthesized).
bool BakeReclaimableForPropInstanceGroup(const TemplateIngestReport& report, Params::PropInstanceGroup& group);

} // namespace Io
} // namespace SanmapGen
```

### 2. New file: `src/io/PropReclaimableBake_IO.cpp`
```cpp
#include "PropReclaimableBake_IO.h"
#include "TemplateDialect_IO.h"                 // Io::DeriveTemplateIdentifierFromPath
#include "../params/ScatterRule_PARAMS.h"       // Params::PropRule
#include "../params/PropInstance_PARAMS.h"      // Params::PropInstanceGroup

namespace SanmapGen {
namespace Io {

bool TemplateHasHarvestableTag(const TemplateIngestReport& report, const std::string& templateIdentifier) {
    const std::vector<std::string>* tags = report.FindTagsByTemplateIdentifier(templateIdentifier);
    if (tags == nullptr) return false;
    for (const std::string& tag : *tags) if (tag == "HARVESTABLE") return true;
    return false;
}

bool BakeReclaimableForPropRule(const TemplateIngestReport& report, Params::PropRule& rule) {
    const std::string templateIdentifier(rule.transform.templateIdentifier);
    if (report.FindTagsByTemplateIdentifier(templateIdentifier) == nullptr) return false;
    rule.bReclaimable = TemplateHasHarvestableTag(report, templateIdentifier);
    return true;
}

bool BakeReclaimableForPropInstanceGroup(const TemplateIngestReport& report, Params::PropInstanceGroup& group) {
    const std::string templateIdentifier = DeriveTemplateIdentifierFromPath(group.blueprintPath);
    if (report.FindTagsByTemplateIdentifier(templateIdentifier) == nullptr) return false;
    group.bReclaimable = TemplateHasHarvestableTag(report, templateIdentifier);
    return true;
}

} // namespace Io
} // namespace SanmapGen
```
(`rule.transform.templateIdentifier` is a fixed `char[8]` — `ScatterTransform_PARAMS.h`, confirmed
real, read this session — the `std::string` construction above is total and safe: a shorter
identifier's trailing NULs terminate the `std::string` constructor normally.)

## Files touched
- NEW `src/io/PropReclaimableBake_IO.h`
- NEW `src/io/PropReclaimableBake_IO.cpp`
- NEW `src/io/PropReclaimableBake_IO_Test.cpp`
- `CMakeLists.txt` — one new `add_sangen_test(PropReclaimableBake_IO_Test src/io/PropReclaimableBake_IO_Test.cpp)`.
- **Zero edits** to `src/params/ScatterRule_PARAMS.h`, `src/params/PropInstance_PARAMS.h`,
  `src/io/MapExporter_PropsStack_IO.cpp`, `src/io/MapImporter_PropsStack_IO.cpp`,
  `src/io/MapExporter_Props_IO.cpp`, `src/io/MapImporter_Props_IO.cpp` — `bReclaimable`'s field, wire
  key, and round trip are already correct (STEP62) and are not touched by this ticket, per the task's
  own explicit requirement.

## Backend policy
CPU only. One hash-map lookup (`FindTagsByTemplateIdentifier`) plus a short linear scan (tags list,
typically 1-3 entries) per call, human-triggered, at most a few times per authoring session — no
compute dispatch, no SIMD, no GPU handle, same scale/posture STEP62's own "two bool fields" cost
estimate already established.

## Layer & accuracy class
IO (bake functions) — no accuracy class of its own (Constitution §4 classes computations, not data).
`bReclaimable`'s OWN consumer remains UI-only (the `Reclaim`/`Props` overlay-domain partition,
`ARCH_14_02_DataModel.md` §14.2/§14.6, per STEP62's own already-shipped exclusion of `bReclaimable`
from `Placement_Hash_PROC.cpp`'s dirty-hash — confirmed by reading STEP62 this session: "does not
affect where/how a prop is scattered... only its overlay-domain classification"). This ticket changes
nothing about that — `bReclaimable` still never reaches PROC, baked or not.

## ARCH rules invoked
- `ARCH_18_03_CatalogDataOwnership.md` §18.3 — the ruling this ticket implements verbatim: tags stay
  IO-owned (ticket 89's sibling table, reused not reinvented here), the bake mechanism mirrors §18.2's
  footprint mechanism exactly, and `economy.harvest`/`collisionInfo`/`collider`/`displayName` remain
  irrelevant (this ticket never touches them, consistent with ticket 87's own scope correction).
- `ARCH_18_02_IngestedDataDeterminism.md` §18.2 rules 1/3/4 — discrete human-triggered action (this
  ticket ships the callable contract, not an automatic trigger); the baked field stays ordinary and
  hand-overridable after baking (no read-only mirror is introduced — `rule.bReclaimable` remains a
  plain `bool`, unchanged); an un-baked/never-ingested identifier is not an error state
  (`BakeReclaimableForPropRule` returning `false` on a miss is an ordinary "nothing to do here" signal,
  not a failure).
- `STEP62_ReclaimPropFilter_PARAMS.md` — the field/wire-key/partition semantics this ticket closes the
  deferred item on WITHOUT changing any of them, per the task's explicit requirement.
- Constitution §1 layering — `PARAMS` never includes `IO`; this file is IO depending on PARAMS
  (`ScatterRule_PARAMS.h`/`PropInstance_PARAMS.h`), the legal direction, not the reverse.

## Explicit out-of-scope
- **Any UI checkbox/button** exposing `bReclaimable` or triggering these bake functions —
  `PropsTab_Rules_UI.cpp`/`PropsTab_Manual_UI.cpp` remain untouched, continuing STEP62's own explicit
  deferral (see "Design note" above).
- **Staleness detection** for `bReclaimable` (a `STEP96`-style "has the ingested tag changed since
  baking" check) — not built; §18.3 does not mandate it, and STEP62's field carries no fingerprint
  concept today. A future ticket's own call if ever needed, not invented here.
- **`economy.harvest`, `collisionInfo`/`collider`, `general.displayName`** — deferred by
  `ARCH_18_03`, irrelevant to this ticket, never read.
- **`PropTransform`/`DecalTransform`/`DecalRule`/`DecalInstanceGroup`** — this ticket is
  `PropRule`/`PropInstanceGroup` only, matching STEP62's own scope exactly (Decals were never named
  in STEP62's ARCH ruling).
- **`Placement_Hash_PROC.cpp`'s dirty-hash** — STEP62 already deliberately excludes `bReclaimable`;
  this ticket adds nothing that would change that (bReclaimable is never written by a PROC-adjacent
  path this ticket introduces).
- **Wiring these functions into `TemplateIngest_IO`/ticket 89 itself** — ticket 89's own scope is
  footprint-and-tags ingestion only; the BAKE step (this ticket) is a separate, later, human-triggered
  action over its already-produced report, exactly mirroring how STEP96's footprint bake is also
  separate from ticket 89.

## Acceptance test
New `src/io/PropReclaimableBake_IO_Test.cpp` (registered in `CMakeLists.txt`):
1. A `TemplateIngestReport` with `tagsByTemplateIdentifier["edbm0101"] = {"HARVESTABLE", "FLAMMABLE"}`
   → `TemplateHasHarvestableTag(report, "edbm0101") == true`;
   `tagsByTemplateIdentifier["edbm0102"] = {"FLAMMABLE"}` (no HARVESTABLE) → `false` for that id.
2. `TemplateHasHarvestableTag(report, "unknown_id")` (no entry at all) → `false` (distinct code path
   from case 1's "known, present" — both currently return `false`, but the NEXT tests below prove the
   two are handled differently by the bake functions themselves via the `Find...` nullptr check).
3. **Bake, found, sets true.** A `PropRule` with `transform.templateIdentifier = "edbm0101"`,
   `bReclaimable` starting `false`, and a report where `"edbm0101"` maps to `{"HARVESTABLE"}` →
   `BakeReclaimableForPropRule` returns `true`, `rule.bReclaimable == true` afterward.
4. **Bake, found, sets false.** Same rule but the report's tags for `"edbm0101"` do NOT contain
   `HARVESTABLE` and `rule.bReclaimable` started `true` → returns `true` (a bake happened),
   `rule.bReclaimable == false` afterward — proves the bake is a real overwrite in both directions,
   not a one-way "only ever sets true" ratchet.
5. **Bake, not found, untouched.** A rule whose `templateIdentifier` has no entry in
   `tagsByTemplateIdentifier` at all, `bReclaimable` starting `true` → `BakeReclaimableForPropRule`
   returns `false`, `rule.bReclaimable` STILL `true` — proves a miss never clears a hand-authored value.
6. **`PropInstanceGroup`, tpId derived from blueprintPath.** A group with
   `blueprintPath = "Environment/01_Highlands/Props/edbm0101/edbm0101.santp"` and a report entry
   keyed `"edbm0101"` → `BakeReclaimableForPropInstanceGroup` derives the same tpId
   (`Io::DeriveTemplateIdentifierFromPath`) and bakes correctly, mirroring test 3's assertion shape.
7. **Empty `templateIdentifier`/`blueprintPath`.** Both bake functions return `false`, target field
   untouched, no crash — an empty identifier can never legitimately be "found" in the report.
8. Full solo rebuild + `ctest -C Debug`: previously-passing suite (including
   `PlacementRules_PARAMS_Test`'s existing `bReclaimable` default check, `MapImporter_IO_Test`'s
   round-trip check, `MapImporter_PropsDecals_IO_Test`'s round-trip check — all from STEP62, unedited)
   stays green; the new target passes.

## Verify
- New `src/io/PropReclaimableBake_IO_Test.cpp` passes, especially tests 4 (bidirectional overwrite)
  and 5 (miss leaves the value untouched).
- Grep `src/params/ScatterRule_PARAMS.h`, `src/params/PropInstance_PARAMS.h`,
  `src/io/MapExporter_PropsStack_IO.cpp`, `src/io/MapImporter_PropsStack_IO.cpp`,
  `src/io/MapExporter_Props_IO.cpp`, `src/io/MapImporter_Props_IO.cpp` — confirm all six are
  byte-for-byte unedited by this ticket (the task's own explicit "without changing STEP62's field,
  wire-key, or partition semantics" requirement).
- Grep `src/ui/` for any new reference to `bReclaimable`/`Reclaimable`/`PropReclaimableBake_IO`
  introduced by this ticket — must be zero (confirms the "no UI call site" design note was honored).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
