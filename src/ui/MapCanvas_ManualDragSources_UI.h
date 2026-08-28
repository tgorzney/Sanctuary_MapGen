// MapCanvas_ManualDragSources_UI.h — the Props/Decals manual-drag injected-pointer bundles + their
// own live gesture state, split out of MapCanvas_UI.h (ARCH §21.7's own flagged file-size ceiling)
// so this ticket's net-new Props/Decals fields land as two small struct members instead of eight
// more scattered fields on an already-over-ceiling class. Markers' own equivalent fields predate
// this split and stay where they are (manualMarkerDragMarkers/.../manualMarkerDragState,
// MapCanvas_UI.h) — not moved here, to keep this ticket's diff additive rather than a Markers-side
// rename churn. Layer: UI. Pure data, no logic of its own.
#pragma once
#include <vector>
#include "InstanceDragGesture_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ManualPropDragSources_UI {
    std::vector<Params::PropInstanceGroup>*       props    = nullptr;
    const std::vector<Params::PropInstanceLayer>* layers   = nullptr;
    const Params::Geometry*                       geometry = nullptr;
    const Params::MapRecipe*                       recipe   = nullptr;
    InstanceDragGestureState                       state;
};

struct ManualDecalDragSources_UI {
    std::vector<Params::DecalInstanceGroup>*       decals   = nullptr;
    const std::vector<Params::DecalInstanceLayer>* layers   = nullptr;
    const Params::Geometry*                        geometry = nullptr;
    const Params::MapRecipe*                        recipe   = nullptr;
    InstanceDragGestureState                        state;
};

} // namespace Ui
} // namespace SanmapGen
