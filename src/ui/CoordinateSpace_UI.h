// CoordinateSpace_UI.h — the six universal screen<->world<->grid conversions every canvas gesture
// (drag, grid-snap, picking) should compose from instead of re-deriving its own fragment of this
// math inline. Layer: UI. Replaces two prior, more complicated drafts of the manual-instance drag
// fix — the root cause of that bug was that no single, reusable conversion existed anywhere in the
// codebase; this file closes that gap so the drag/snap logic built on top of it stays simple.
//
// Units: 1 world unit = the base unit the game stores positions in. 1 grid cell =
// `Params::Geometry::worldUnitsPerGenerationCell` world units for "the real terrain grid" (a caller may pass
// a whole-number MULTIPLE of it for a coarser snap grid — Part 3 of the work-order this file
// implements). Screen space = region-local pixels (top-left of the canvas widget), the same space
// every existing gesture already works in.
//
// `ScreenToWorld`/`WorldToScreen` are declared here but DEFINED in CoordinateSpace_UI.cpp (not
// inline): both need `PreviewComposite`'s full type to call its own WorldToPreviewPixel/
// PreviewPixelToWorld methods, and forward-declaring it here (mirroring ManualInstanceHitTest_UI.h's
// own identical precedent) keeps a pure-math-only includer (MarkersTab_ManualLayerHelpers_UI.h and
// its Props/Decals siblings, which only ever need WorldToGrid/GridToWorld) from pulling in
// PreviewComposite_UI.h's own GL/Sys::GpuResourceManager dependency for no reason.
//
// `ScreenToWorld` is CONTINUOUS — it never floors/rounds internally. That was the root cause of the
// "large increment" drag bug: the old drag code routed every frame's cursor position through
// `MapCanvasView::ResolvePreviewPixel`, which floors to an integer preview-texel before converting,
// so a whole screen-pixel span of cursor movement could collapse onto the same floored texel (or
// jump texels) depending on zoom. This file recomputes the same region-local -> image-pixel math
// `ResolvePreviewPixel` uses, from that class's own already-public accessors, WITHOUT the floor —
// `ResolvePreviewPixel`'s own int-returning form and every existing caller of it (picking/hit-testing
// still wants the discrete texel) are untouched.
#pragma once
#include <cmath>
#include "MapCanvasView_UI.h"

namespace SanmapGen {
namespace Ui {

class PreviewComposite;

struct WorldPoint  { float worldX = 0.0f;  float worldZ = 0.0f; };
struct ScreenPoint { float screenX = 0.0f; float screenY = 0.0f; };
// float, not int — a mid-cell position stays meaningful (only snapping, via WorldToGrid/GridToWorld
// applied back-to-back, ever rounds it).
struct GridPoint   { float gridX = 0.0f;   float gridZ = 0.0f; };

// screen -> world, continuous (see header comment above). Degenerate view/composite state (the
// same "not yet baked/sized" conditions `MapCanvasView`/`PreviewComposite`'s own accessors already
// guard) answers (0, 0), mirroring `PreviewComposite::PreviewPixelToWorld`'s own zero-when-unbaked
// contract.
WorldPoint ScreenToWorld(const MapCanvasView& view, const PreviewComposite& composite, ScreenPoint screen);
// world -> screen, continuous. Composition of `PreviewComposite::WorldToPreviewPixel` +
// `MapCanvasView::ProjectPreviewPixelToRegionLocal` — both already continuous, no floor to remove.
ScreenPoint WorldToScreen(const MapCanvasView& view, const PreviewComposite& composite, WorldPoint world);

// world -> grid, at a given cell size in world units (pass `Params::Geometry::worldUnitsPerGenerationCell` for
// "the real terrain grid"; pass a whole-number multiple of it for a coarser snap grid). Guards
// `cellSizeWorldUnits <= 0` by returning the input's raw numeric values unchanged (Constitution §6 —
// never a divide by zero; there is no meaningful grid at a non-positive cell size, so this is a
// crash-free fallback, not a claim that the result is a valid grid coordinate).
inline GridPoint WorldToGrid(float cellSizeWorldUnits, WorldPoint world) {
    if (cellSizeWorldUnits <= 0.0f) return GridPoint{world.worldX, world.worldZ};
    return GridPoint{std::floor(world.worldX / cellSizeWorldUnits), std::floor(world.worldZ / cellSizeWorldUnits)};
}
// grid -> world. ALWAYS returns the CENTER of the cell, never a corner/vertex — every downstream
// caller (grid-snap, symmetry-orbit placement) is automatically correct with no separate "+half a
// cell" step of its own. Same non-positive-cell-size guard as WorldToGrid.
inline WorldPoint GridToWorld(float cellSizeWorldUnits, GridPoint grid) {
    if (cellSizeWorldUnits <= 0.0f) return WorldPoint{grid.gridX, grid.gridZ};
    return WorldPoint{(grid.gridX + 0.5f) * cellSizeWorldUnits, (grid.gridZ + 0.5f) * cellSizeWorldUnits};
}

// screen <-> grid — pure composition of the four above, provided for convenience/discoverability
// (a caller never has to hand-compose ScreenToWorld+WorldToGrid itself).
GridPoint   ScreenToGrid(const MapCanvasView& view, const PreviewComposite& composite,
                         float cellSizeWorldUnits, ScreenPoint screen);
ScreenPoint GridToScreen(const MapCanvasView& view, const PreviewComposite& composite,
                         float cellSizeWorldUnits, GridPoint grid);

} // namespace Ui
} // namespace SanmapGen
