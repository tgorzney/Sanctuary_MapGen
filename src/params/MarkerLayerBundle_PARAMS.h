// MarkerLayerBundle_PARAMS.h — Params::MarkerLayerBundle: the Group-above-Layer container ARCH §19
// ratifies (ratifies work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md). Layer: PARAMS. New file,
// sibling of MarkerRule_PARAMS.h/MarkerInstance_PARAMS.h — parent of BOTH
// (ARCH_19_03_FieldSpellings.md), so it does not live inside either. UI display label stays "Group"
// (ARCH §19.1); the C++/wire type is spelled "Bundle" specifically to avoid a 4th collision with the
// word "Group" (MarkerInstanceGroup / the MarkerGroups wire array / the MarkersStack
// Group(MarkerRuleLayer)->Rule wrapper already use it for three different things).
//
// STEP119 line-count split (ARCH §1.5's 150-line hard ceiling, flagged by the ticket itself, not an
// undocumented deviation): the two `Collect*` recursive-membership resolvers plus their shared
// `CollectMarkerLayerBundleDescendantIdentifiers` helper live in the companion
// MarkerLayerBundleQuery_PARAMS.h, mirroring how MapImporter_MarkerLayerReconcile_IO.cpp was split
// out of MapImporter_Markers_IO.cpp for the identical reason (STEP115). This file keeps the struct
// itself plus the cycle-detection predicate, both hand-written per domain per ARCH §19.8
// (Props/Decals get their own independent twins later, NOT a shared template — ARCH §19.2's
// domain-touching-vs-pure-mechanics split), mirroring ResolvePropInstanceLayerId/
// ResolveDecalInstanceLayerId's per-domain-repeated shape (PropInstance_PARAMS.h:37-44).
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params {

// ARCH_19_03_FieldSpellings.md. Additive-only; MapRecipe::markerLayerBundles is a fresh vector.
// Wire array MarkerLayerBundles (Correction 19) spells its stable id `Identifier` in full from day
// one — does NOT repeat MarkerGroups' pre-§1.9 "Id" abbreviation defect (ARCH §1.9).
struct MarkerLayerBundle {
    int identifier             = -1;   // stable, survives reorder/delete — NOT this array's own
                                        // position (array order is not this array's identity,
                                        // unlike MarkerGroups/PropGroups/DecalGroups).
    std::string name;
    int parentBundleIdentifier = -1;   // -1 = root; enables Bundle-in-Bundle nesting.
    std::string markerTypeName;        // single-type scope, free-form string space, same as
                                        // MarkerInstanceGroup::name (e.g. "Alloy"), NOT MarkerCategory.
    int assemblyIdentifier     = -1;   // ARCH §19.5 — Assembly-references-Bundle hook. Inert until
                                        // the separate, still-unbuilt Assembly feature exists; no
                                        // Params::Assembly type exists yet to validate against.
    int linkIdentifier         = -1;   // ARCH §19.29 — organizational: which Params::MarkerLink
                                        // created this Group; drives the Links tier's own
                                        // membership/ungroup walk (ARCH §19.31's Delete-Link
                                        // semantics). -1 = not Link-bound, the shared sentinel this
                                        // whole struct family already uses.
};

// True when reparenting `candidateId` under `newParentId` would create a cycle (including
// `candidateId == newParentId` itself). Walks parentBundleIdentifier up from newParentId; mirrors
// WouldReparentCreateCycle's proposed shape for Params::Assembly (DESIGN_Assembly_R1.md §1),
// confirmed PARAMS-resident and NOT shared/templated with Assembly's own body by ARCH §19.8 (this
// one's signature carries a Params:: type). Bounded to bundles.size()+1 steps so an
// already-corrupt/cyclic table cannot hang the caller — used both to REFUSE a live reparent (Ticket
// B, UI) and to REPAIR a cyclic import (MapImporter_MarkerLayerBundle_IO.cpp, this ticket).
inline bool WouldReparentMarkerLayerBundleCreateCycle(int candidateId, int newParentId,
                                                       const std::vector<MarkerLayerBundle>& bundles) {
    if (candidateId == newParentId) return true;
    int walk = newParentId;
    std::size_t stepsRemaining = bundles.size() + 1;
    while (walk != -1 && stepsRemaining > 0) {
        if (walk == candidateId) return true;
        int nextWalk = -1;
        for (const MarkerLayerBundle& bundle : bundles) {
            if (bundle.identifier == walk) { nextWalk = bundle.parentBundleIdentifier; break; }
        }
        walk = nextWalk;
        --stepsRemaining;
    }
    return false;
}

} // namespace Params
} // namespace SanmapGen
