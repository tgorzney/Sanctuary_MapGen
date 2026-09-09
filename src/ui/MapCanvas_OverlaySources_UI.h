// MapCanvas_OverlaySources_UI.h — the screen-space overlay-icon-draw-pass's injected sources +
// its own owned per-canvas caches, consolidated out of MapCanvas_UI.h (ARCH §21.7 remediation,
// STEP255) so this cluster is one field instead of ten scattered ones — mirrors
// MapCanvas_ManualDragSources_UI.h's ManualPropDragSources_UI/ManualDecalDragSources_UI shape.
// Layer: UI. Pure data, no logic of its own.
#pragma once
#include "MapCanvas_IconLayer_UI.h"       // OverlayRenderingSettings, IconLayerAabbCache_UI, IconLayerFrameCache
#include "OverlayLayer_Settings_UI.h"     // OverlayLayerSettings
#include "../io/WorldFootprintSizeTable_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Data { struct PlacementResults; struct RuleBucketIndexSet; }
namespace Ui {
struct IconAtlasManifest;
class IconAtlasPairingLookup;   // AMENDMENT — real declaration is `class`, not `struct`; `struct`
                                 // compiles but trips an avoidable C4099 mismatch warning

struct OverlayIconPassSources_UI {
    const OverlayLayerSettings*         layerSettings      = nullptr;
    const OverlayRenderingSettings*     renderingSettings  = nullptr;
    const Data::PlacementResults*       placements         = nullptr;
    const Data::RuleBucketIndexSet*     ruleBucketIndex    = nullptr;
    const Params::MapRecipe*            recipe             = nullptr;
    const IconAtlasPairingLookup*       pairingLookup      = nullptr;
    const IconAtlasManifest*            atlasManifest      = nullptr;
    const Io::WorldFootprintSizeTable*  footprintSizeTable = nullptr;
    IconLayerAabbCache_UI               layerAabbCache;
    IconLayerFrameCache                 iconLayerFrameCache;
};

} // namespace Ui
} // namespace SanmapGen
