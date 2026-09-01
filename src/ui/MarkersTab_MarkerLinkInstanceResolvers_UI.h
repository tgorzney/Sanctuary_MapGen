// MarkersTab_MarkerLinkInstanceResolvers_UI.h — STEP246, ARCH §19.33/§21.9: the remaining 4
// instance-tier resolver pairs (Hidden, IconScale, ColorOverrideEnabled+Color, and the
// GridSnapEnabled/SymmetryEnabled BOOLEAN gates — distinct from QuantizeMarkerPositionToLayerGrid/
// ResolveEffectiveMarkerSymmetry, MarkersTab_ManualLayerHelpers_UI.h, which resolve the actual
// grid-size/mask VALUES). The sixth governed field (`bLocked`) lives in that same header instead
// (`EffectiveManualMarkerInstanceLocked`/`IsMarkerInstanceLocked`), per §21.9's own ruling.
//
// New file rather than an addition to MarkersTab_MarkerLinkResolvers_UI.h (STEP241/242's Layer-tier
// resolvers) — that file is already near ARCH §1.5's soft ceiling; adding 4 more pairs in place
// would blow well past it, the same "new file for a distinct concern" precedent it already cites.
//
// No instance-tier resolver for `name` — ARCH §19.33's explicit, deliberate exclusion: a
// MarkerTransform's own name (e.g. "Mex 0") is its own proper identity, not a label a Link
// overwrites. Do not add an `EffectiveMarkerTransformName` here or anywhere else.
#pragma once
#include <vector>
#include "MarkersTab_ManualLayerHelpers_UI.h"      // EffectiveManualMarkerLayerColorOverrideEnabled/Color
#include "MarkersTab_MarkerLinkResolvers_UI.h"     // the Layer-tier fallback resolvers
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Every resolver below: (1) `transform.linkIdentifier >= 0` and resolves -> that Link's own field;
// (2) else -> the existing 2-parameter Layer-tier resolver (itself already Link-aware, §19.31),
// which itself falls through to the Layer's own stored field. A dangling identifier at either tier
// soft-degrades to the next step (Constitution §6), never a refusal. Mirrors
// EffectiveManualMarkerInstanceLocked's exact shape (MarkersTab_ManualLayerHelpers_UI.h).

inline bool EffectiveManualMarkerInstanceHidden(const Params::MarkerTransform& transform,
                                                const Params::MarkerInstanceLayer& layer,
                                                const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.bHidden;
    return EffectiveManualMarkerLayerHidden(layer, links);
}

inline float EffectiveManualMarkerInstanceIconScale(const Params::MarkerTransform& transform,
                                                     const Params::MarkerInstanceLayer& layer,
                                                     const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.iconScale;
    return EffectiveManualMarkerLayerIconScale(layer, links);
}

inline bool EffectiveManualMarkerInstanceColorOverrideEnabled(const Params::MarkerTransform& transform,
                                                              const Params::MarkerInstanceLayer& layer,
                                                              const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.bColorOverrideEnabled;
    return EffectiveManualMarkerLayerColorOverrideEnabled(layer, links);
}

inline const float* EffectiveManualMarkerInstanceColor(const Params::MarkerTransform& transform,
                                                        const Params::MarkerInstanceLayer& layer,
                                                        const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.color;
    return EffectiveManualMarkerLayerColor(layer, links);
}

// Boolean GATES only — whether grid-snap/symmetry is toggled on at all, mirroring the existing
// 2-parameter EffectiveManualMarkerLayerGridSnapEnabled/SymmetryEnabled split. The actual grid-size/
// mask VALUE resolution is QuantizeMarkerPositionToLayerGrid/ResolveEffectiveMarkerSymmetry's own
// job (MarkersTab_ManualLayerHelpers_UI.h) — not duplicated here.
inline bool EffectiveManualMarkerInstanceGridSnapEnabled(const Params::MarkerTransform& transform,
                                                         const Params::MarkerInstanceLayer& layer,
                                                         const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.bGridSnapEnabled;
    return EffectiveManualMarkerLayerGridSnapEnabled(layer, links);
}

inline bool EffectiveManualMarkerInstanceSymmetryEnabled(const Params::MarkerTransform& transform,
                                                         const Params::MarkerInstanceLayer& layer,
                                                         const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.bSymmetryEnabled;
    return EffectiveManualMarkerLayerSymmetryEnabled(layer, links);
}

} // namespace Ui
} // namespace SanmapGen
