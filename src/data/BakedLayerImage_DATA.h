// BakedLayerImage_DATA.h — the FROZEN pixels of one baked `Params::Layer` (STEP99/100).
// Layer: DATA. Same precedent as StratumArt_DATA.h (MASKING_SPEC §1.7 / this ticket's own
// "Architecture call"): a baked layer's height contribution is loaded/frozen input, never
// part of the recipe, so it lives here rather than on `Params::Layer` itself.
// Keyed by `Params::Layer::layerIdentifier` (stable across reorder/copy/move), NOT by flat
// stack position — a dynamic, reorderable `std::vector<Layer>` makes a position-indexed cache
// silently reattach to the wrong layer after a reorder (STEP100's load-bearing design call).
// Plain data + accessors; no behavior, no GPU handles (ARCH §3.2).
#pragma once
#include <vector>
#include "FloatField_DATA.h"

namespace SanmapGen {
namespace Data {

struct BakedLayerImage {
    int        layerIdentifier = -1;   // Params::Layer::layerIdentifier this belongs to
    FloatField image;                  // 0..1 normalized height contribution
};

// Linear scan — layer counts are tens, not thousands.
inline const FloatField* FindBakedLayerImage(const std::vector<BakedLayerImage>& images,
                                             int layerIdentifier) {
    if (layerIdentifier < 0) return nullptr;
    for (const BakedLayerImage& entry : images)
        if (entry.layerIdentifier == layerIdentifier) return &entry.image;
    return nullptr;
}

} // namespace Data
} // namespace SanmapGen
