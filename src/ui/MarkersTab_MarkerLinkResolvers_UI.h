// MarkersTab_MarkerLinkResolvers_UI.h — the full "STEP241/ARCH §19.31" read-and-resolve surface: one
// getter per Section/Group-equivalent setting a Link governs, at whichever tier already carries that
// field (Bundle: name only; Layer: name/bHidden/iconScale/bGridSnapEnabled+gridSnapSizeWorldUnits/
// bSymmetryEnabled+symmetry/bLocked). Sibling of MarkersTab_ManualLayerHelpers_UI.h, NOT folded into
// it — that file is already close to ARCH §1.5's soft ceiling before this ticket's own eight new
// resolvers; this is a plain "new file for a distinct concern" split, the same reasoning
// MarkerLayerBundle_PARAMS.h's own header comment already gives for MarkerLayerBundleQuery_PARAMS.h.
// `EffectiveManualMarkerLayerColorOverrideEnabled`/`EffectiveManualMarkerLayerColor` (STEP239) stay
// in MarkersTab_ManualLayerHelpers_UI.h, unmoved — this file only adds what STEP241 needs new.
// STEP242 (ARCH §19.31's same-day follow-up amendment, governed field #7) adds
// EffectiveManualMarkerLayerLocked here, same file, same shape — no new file warranted for one getter.
#pragma once
#include <string>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Every resolver below: `linkIdentifier >= 0` AND it resolves to a real entry in `links` -> the
// Link's own field; otherwise (unbound, or a dangling identifier — Constitution §6 soft-degrade)
// -> the Bundle's/Layer's own field, unchanged from today. Mirrors
// EffectiveManualMarkerLayerColor/ColorOverrideEnabled's exact shape (MarkersTab_ManualLayerHelpers_UI.h).

// Bundle tier — the ONLY field MarkerLayerBundle has that ARCH §19.31 governs (no color/hidden/
// iconScale/grid-snap/symmetry field exists on MarkerLayerBundle at all; those are Layer-tier only).
inline const std::string& EffectiveMarkerLayerBundleName(const Params::MarkerLayerBundle& bundle,
                                                          const std::vector<Params::MarkerLink>& links) {
    if (bundle.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == bundle.linkIdentifier) return link.name;
    return bundle.name;
}

// Layer tier — Name, now read-and-resolved exactly like every other governed field (STEP241
// retracts STEP239's Name-is-a-cascade-write carve-out).
inline const std::string& EffectiveManualMarkerLayerName(const Params::MarkerInstanceLayer& layer,
                                                          const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.name;
    return layer.name;
}

// The row label a Manual Layer draws with once its Name is Link-resolved — the two-arg sibling of
// ManualMarkerLayerRowLabel (MarkersTab_ManualLayerHelpers_UI.h), same never-empty fallback
// ("Marker Layer") posture, layered on top of EffectiveManualMarkerLayerName instead of `layer.name`
// directly.
inline const char* ManualMarkerLayerRowLabel(const Params::MarkerInstanceLayer& layer,
                                             const std::vector<Params::MarkerLink>& links) {
    const std::string& effectiveName = EffectiveManualMarkerLayerName(layer, links);
    return effectiveName.empty() ? "Marker Layer" : effectiveName.c_str();
}

inline bool EffectiveManualMarkerLayerHidden(const Params::MarkerInstanceLayer& layer,
                                             const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.bHidden;
    return layer.bHidden;
}

inline float EffectiveManualMarkerLayerIconScale(const Params::MarkerInstanceLayer& layer,
                                                 const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.iconScale;
    return layer.iconScale;
}

// bGridSnapEnabled/gridSnapSizeWorldUnits resolve as a pair (ARCH §19.31: "a size with no enabling
// toggle is meaningless and vice versa") — two separate getters, same resolved linkIdentifier match.
inline bool EffectiveManualMarkerLayerGridSnapEnabled(const Params::MarkerInstanceLayer& layer,
                                                      const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.bGridSnapEnabled;
    return layer.bGridSnapEnabled;
}

inline float EffectiveManualMarkerLayerGridSnapSizeWorldUnits(const Params::MarkerInstanceLayer& layer,
                                                              const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.gridSnapSizeWorldUnits;
    return layer.gridSnapSizeWorldUnits;
}

inline bool EffectiveManualMarkerLayerSymmetryEnabled(const Params::MarkerInstanceLayer& layer,
                                                      const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.bSymmetryEnabled;
    return layer.bSymmetryEnabled;
}

inline const Params::SymmetrySetting& EffectiveManualMarkerLayerSymmetry(
        const Params::MarkerInstanceLayer& layer, const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.symmetry;
    return layer.symmetry;
}

// STEP242, ARCH §19.31 follow-up amendment (governed field #7) — same read-and-resolve shape as
// every getter above, one field over.
inline bool EffectiveManualMarkerLayerLocked(const Params::MarkerInstanceLayer& layer,
                                             const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.bLocked;
    return layer.bLocked;
}

} // namespace Ui
} // namespace SanmapGen
