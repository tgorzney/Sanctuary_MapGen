// MarkerLink_PARAMS.h — Params::MarkerLink: the cross-Marker-Type grouping/master-slave tag.
// ARCH §19.28 ratifies the type; ARCH §19.31 (CORRECTED 2026-08-31, direct human ruling; FURTHER
// AMENDED 2026-08-31 to add `bLocked` as governed field #7) ratifies this struct's full field set —
// see that section for the "why", not re-derived here. Layer: PARAMS. New file, sibling of
// MarkerLayerBundle_PARAMS.h — same "new tier gets its own file" precedent §19.1/§19.3 already
// established for Bundle. A Link is a peer of Bundle in the hierarchy (a cross-cutting concept a
// Bundle/Layer can optionally be tagged into, §19.29), not a member of it — folding it into
// MarkerLayerBundle_PARAMS.h would misstate that relationship.
//
// STEP241 (retracts STEP239's Name-is-a-cascade-write carve-out): direct human ruling — "when a
// group is part of a link, the link is the 'master' and the Section Group associated with the link
// is the 'slave' — the settings on a linked group would be disabled." ONE uniform mechanism,
// read-and-resolve, for EVERY Section/Group-equivalent setting a Link governs, Name included. A
// bound Bundle's/Layer's own field is never independently editable and never written back onto
// while `linkIdentifier >= 0` — see MarkersTab_ManualLayerHelpers_UI.h's/the new resolvers file's
// `Effective*` functions for the read side.
//
// STEP242 (ARCH §19.31's same-day follow-up amendment, "everything should be cascaded down to the
// Groups in the Link"): `bLocked` joins the governed set on the identical basis as the six fields
// above — no Bundle-tier counterpart exists to mirror (ARCH §19.31's own "Bundle-tier lock — ruled:
// no equivalent field" section), so this field lives at the Link tier only, mirroring
// MarkerInstanceLayer::bLocked exactly.
#pragma once
#include <string>
#include "Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Params {

// ARCH §19.31 (corrected). A Link is new source-of-truth state that exists nowhere else in the
// recipe — read-and-resolve, never write-through-and-copy, onto every bound MarkerLayerBundle/
// MarkerInstanceLayer. Every field below is a plain mirror of its MarkerInstanceLayer counterpart's
// own shape/default — NOT gated behind a second "does this field propagate" flag: once linked, the
// Link owns that same substantive state outright (ARCH §19.31's own reasoning for why no such gate
// exists).
struct MarkerLink {
    int identifier              = -1;    // stable, survives reorder/delete — spelled per §1.9,
                                          // matches MarkerLayerBundle::identifier/
                                          // Params::Assembly::identifier's own spelling
    std::string name;                    // STEP241: read-and-resolve like every other field below —
                                          // the Bundle-tier/Layer-tier cascade-write mechanism
                                          // (STEP239's Mechanism B) is RETRACTED, not merely narrowed.

    bool  bColorOverrideEnabled  = false; // mirrors MarkerInstanceLayer::bColorOverrideEnabled
    float color[4]               = {1.0f, 1.0f, 1.0f, 1.0f};   // mirrors MarkerInstanceLayer::color

    bool  bHidden                = false; // STEP241/ARCH §19.31 correction — retracts STEP239's "no
                                          // bHidden field, hide is a cascading action" text; this is
                                          // now a plain read-and-resolve field like color, requiring
                                          // the field to exist here as the resolve-from source.
    float iconScale               = 1.0f; // mirrors MarkerInstanceLayer::iconScale.
    bool  bGridSnapEnabled        = false;// mirrors MarkerInstanceLayer::bGridSnapEnabled.
    float gridSnapSizeWorldUnits  = 1.0f; // mirrors MarkerInstanceLayer::gridSnapSizeWorldUnits.
    bool  bSymmetryEnabled        = true; // mirrors MarkerInstanceLayer::bSymmetryEnabled.
    Params::SymmetrySetting symmetry;     // mirrors MarkerInstanceLayer::symmetry.
    bool  bLocked                 = false; // STEP242, ARCH §19.31 follow-up amendment (governed field
                                          // #7) — mirrors MarkerInstanceLayer::bLocked. No
                                          // MarkerLayerBundle-tier counterpart exists to mirror.
};

} // namespace Params
} // namespace SanmapGen
