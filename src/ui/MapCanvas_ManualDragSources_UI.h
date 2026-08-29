// MapCanvas_ManualDragSources_UI.h — the Props/Decals manual-drag injected-pointer bundles + their
// own live gesture state, split out of MapCanvas_UI.h (ARCH §21.7's own flagged file-size ceiling)
// so this ticket's net-new Props/Decals fields land as two small struct members instead of eight
// more scattered fields on an already-over-ceiling class. Markers' own equivalent fields predate
// this split and stay where they are (manualMarkerDragMarkers/.../manualMarkerDragState,
// MapCanvas_UI.h) — not moved here, to keep this ticket's diff additive rather than a Markers-side
// rename churn. Layer: UI. Pure data, no logic of its own.
#pragma once
#include <vector>
#include "AreaColorTable_UI.h"         // AreaColorEntry — ARCH §14.17 item 9's retarget
#include "AreaDragGesture_UI.h"
#include "AreaLockTable_UI.h"          // AreaLockEntry — STEP212's new per-area lock side table
#include "InstanceDragGesture_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MapArea_PARAMS.h"
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

// ARCH §21.8 — the Area canvas gesture's own injected-pointer bundle. `recipe.areas` is a flat
// vector with no group/transform/lock shape at all (§21.8 correction 1), so this does NOT mirror
// ManualPropDragSources_UI/ManualDecalDragSources_UI's own `InstanceDragGestureState` — it carries
// the standalone AreaDragGestureState instead.
struct ManualAreaDragSources_UI {
    std::vector<Params::MapArea>* areas             = nullptr;   // mutable: canvas creates/moves/resizes
    std::vector<AreaColorEntry>*  areaColors         = nullptr;   // mutable: ResolveAreaColor lazily
                                                                    // appends a default entry for a
                                                                    // freshly canvas-created area
    // STEP212 — replaces the retired `const bool* bAreasLocked`: one lock bit PER AREA, the exact
    // same UI-only name-keyed side-table shape as `areaColors` above (AreaLockTable_UI.h's own
    // AreaLockEntry/ResolveAreaLocked, mirroring AreaColorTable_UI.h's AreaColorEntry/
    // ResolveAreaColor). Mutable (not read-only like the field it replaces) because
    // ResolveAreaLocked lazily appends a default-LOCKED entry on first touch, exactly as
    // ResolveAreaColor already does for areaColors, AND because CreateAreaFromDrag must insert a
    // freshly created area's own entry as UNLOCKED (STEP212 Fix 1) — the canvas now legitimately
    // writes into this table, unlike the plain bool it replaces. Unlike areaColors, this table has
    // NO composite-side reader at all (lock never affects what the GPU composite draws — only
    // whether the canvas gesture accepts input) — its single owner stays `AreasTabState::areaLocks`,
    // never `PreviewCompositeSettings`.
    std::vector<AreaLockEntry>*   areaLocks          = nullptr;
    int*                          selectedAreaIndex  = nullptr;   // mutable: auto-select-on-touch/deselect
    // ARCH §14.17 item 11 — mutable: the canvas sets/clears this to omit the dragged area from the
    // composite input for the duration of a gesture. Points at
    // `PreviewCompositeSettings::mapAreaSuppressedIndex` — one source of truth, never a second copy.
    // STEP212 — untouched; this field's own plumbing is STEP211 territory.
    int*                          mapAreaSuppressedIndex = nullptr;
    AreaDragGestureState           state;
};

} // namespace Ui
} // namespace SanmapGen
