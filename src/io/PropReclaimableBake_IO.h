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
