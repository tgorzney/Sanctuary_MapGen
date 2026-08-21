# STEP47 — World <-> screen coordinate projection, one canonical copy each direction

**Layer:** UI. **Domain:** `MapCanvasView`, `PreviewComposite`. **Sequence:** Phase 1.1,
`work_orders/SEQUENCE_PreviewOverlayLayering.md`. Prerequisite for STEP48 (picking migration) and
Phase 3's screen-space icon draw pass.

## Problem
Two coordinate mappings exist today, each with exactly one copy — correct so far — but neither
has an inverse, and no function composes them:
1. **World -> preview-pixel ("texel"):** `PreviewComposite::BuildEntityPoints()`
   (`src/ui/PreviewComposite_Prepare_UI.cpp:90-93`), inline, private to `PreviewComposite`, used
   only to bake the overlay pass's flat marks.
2. **Region-local screen -> preview-pixel:** `MapCanvasView::ResolvePreviewPixel()`
   (`src/ui/MapCanvasView_UI.h:77-91`), public, pure, imgui-free.

ARCH §14's overlay redesign needs the **composition of both, in both directions**:
world -> screen (to place an icon) and screen -> world (to pick one, STEP48). Building that
composition without extracting shared functions first would create a second copy of each mapping
— exactly the class of drift `SpatialGrid_DATA.h`'s own header already warns against
("a second copy... is exactly how a picker drifts from its index").

## Fix — two small pure additions, no existing call site changes behavior

### 1. `MapCanvasView` gets the inverse of its existing mapping
`MapCanvasView_UI.h` is deliberately imgui-free and DATA-free (its own header comment: "no imgui,
no GL, no DATA... exactly one copy"). Its job stops at region-local-screen <-> preview-pixel; add
the missing direction next to the existing one:

```cpp
// MapCanvasView_UI.h — next to PreviewPixelCoordinate
struct RegionLocalPoint { float regionLocalX = 0.0f; float regionLocalY = 0.0f; };
```
```cpp
// MapCanvasView — public, next to ResolvePreviewPixel()
// Preview pixel -> region-local point (inverse of ResolvePreviewPixel). Degenerate view state
// (non-positive previewResolution/regionSidePixels/zoomScale) answers (0,0), mirroring
// ResolvePreviewPixel's own early-return-zeroed contract — callers must have valid state already.
RegionLocalPoint ProjectPreviewPixelToRegionLocal(float pixelX, float pixelY) const {
    RegionLocalPoint point;
    if (previewResolution <= 0 || regionSidePixels <= 0.0f) return point;
    const float span = VisibleSpanPixels();
    if (span <= 0.0f) return point;
    const float spanReciprocal = 1.0f / span;
    point.regionLocalX = regionSidePixels * (0.5f + (pixelX - viewCenterPixelX) * spanReciprocal);
    point.regionLocalY = regionSidePixels * (0.5f + (pixelY - viewCenterPixelY) * spanReciprocal);
    return point;
}
```
Derivation: `ResolvePreviewPixel` computes
`imagePoint = viewCenter + (regionLocal/regionSide - 0.5) * span`; solving for `regionLocal` gives
exactly the formula above. Add a unit test asserting round-trip identity
(`ProjectPreviewPixelToRegionLocal(ResolvePreviewPixel(x,y).pixelX, ...) ~= (x,y)` within one
pixel, accounting for `ResolvePreviewPixel`'s floor) alongside the existing `MapCanvasView` tests.

This function returns a **region-local** point, not an absolute screen coordinate — consistent
with `ResolvePreviewPixel`'s own contract (it takes region-local input, not absolute screen
input). The caller adds the region's screen origin, exactly as `MapCanvas_Draw_UI.cpp` already
subtracts it before calling `ResolvePreviewPixel` today (`ApplyPointerInput`'s existing pattern —
do not invert that convention here).

### 2. `PreviewComposite` gets a public, bidirectional world <-> preview-pixel mapping
This mapping depends on composite-owned config (`settings.worldUnitsPerCell`, the field grid's
vertex size) that `MapCanvasView` must not know about (its header's own "no DATA" boundary) — it
belongs where `BuildEntityPoints` already lives, not in `MapCanvasView`.

First, expose the one missing input `BuildEntityPoints` currently computes inline and nothing
outside `PreviewComposite` can reach:
```cpp
// PreviewComposite_UI.h — public, next to Resolution()
// Preview pixels per one world-space "cell" of the baked field grid — the scale factor world
// positions are mapped through. Zero if no field grid is baked yet (mirrors Resolution()'s own
// zero-when-unbaked contract).
float PixelsPerPreviewCell() const;   // PreviewComposite_Prepare_UI.cpp — mapFields.VertexSize()-derived
```

Then extract `BuildEntityPoints`'s inline math into a named, public, pure pair and repoint
`BuildEntityPoints` to call the forward half — no second copy on day one:
```cpp
// PreviewComposite_UI.h — a small pure value type + public methods, next to PixelsPerPreviewCell()
struct PreviewWorldPoint  { float worldX = 0.0f; float worldZ = 0.0f; };
struct PreviewPixelPoint  { float pixelX = 0.0f; float pixelY = 0.0f; };

// World (positionX/positionZ — the horizontal plane; positionY is height, PlacementInstance_DATA)
// -> preview pixel. The exact mapping BuildEntityPoints already bakes marks through; extracted so
// there is exactly one copy (ARCH §8.3's "one copy" principle, same class of rule as
// Data::SpatialGrid::CellIndexAt).
PreviewPixelPoint WorldToPreviewPixel(float worldX, float worldZ) const;
// Inverse — preview pixel -> world. New; BuildEntityPoints never needed this direction, STEP48's
// picking migration does.
PreviewWorldPoint PreviewPixelToWorld(float pixelX, float pixelY) const;
```
```cpp
// PreviewComposite_Prepare_UI.cpp
float PreviewComposite::PixelsPerPreviewCell() const {
    const int vertexSize = mapFields.VertexSize();
    if (vertexSize < 2) return 0.0f;
    return static_cast<float>(configuration.previewResolution) / static_cast<float>(vertexSize - 1);
}

PreviewComposite::PreviewPixelPoint PreviewComposite::WorldToPreviewPixel(float worldX, float worldZ) const {
    const float cellsPerWorldUnit = ReciprocalOrZero(settings.worldUnitsPerCell);
    const float pixelsPerCell = PixelsPerPreviewCell();
    PreviewPixelPoint point;
    point.pixelX = worldX * cellsPerWorldUnit * pixelsPerCell - 0.5f;
    point.pixelY = worldZ * cellsPerWorldUnit * pixelsPerCell - 0.5f;
    return point;
}

PreviewComposite::PreviewWorldPoint PreviewComposite::PreviewPixelToWorld(float pixelX, float pixelY) const {
    const float pixelsPerCell = PixelsPerPreviewCell();
    const float cellReciprocal = ReciprocalOrZero(pixelsPerCell);
    PreviewWorldPoint point;
    point.worldX = (pixelX + 0.5f) * settings.worldUnitsPerCell * cellReciprocal;
    point.worldZ = (pixelY + 0.5f) * settings.worldUnitsPerCell * cellReciprocal;
    return point;
}

void PreviewComposite::BuildEntityPoints() {
    entityPoints.clear();
    configuration.entityCount = 0;
    if (!settings.bEntitiesEnabled || PixelsPerPreviewCell() <= 0.0f || instances.IsEmpty()) return;
    entityPoints.reserve(instances.Count());
    for (std::size_t instance = 0; instance < instances.Count(); ++instance) {
        PreviewEntityPoint point;
        const PreviewPixelPoint pixel = WorldToPreviewPixel(instances.positionX[instance],
                                                             instances.positionZ[instance]);
        point.pixelX = pixel.pixelX;
        point.pixelY = pixel.pixelY;
        point.entityIdentifier = static_cast<unsigned int>(instance);
        entityPoints.push_back(point);
    }
    configuration.entityCount = static_cast<int>(entityPoints.size());
}
```
⚠️ `WorldToPreviewPixel`/`PreviewPixelToWorld` are exact inverses only when `PixelsPerPreviewCell()
> 0`; `PreviewPixelToWorld` on an unbaked composite (`PixelsPerPreviewCell() == 0`) answers
`(0, 0)` via `ReciprocalOrZero` — callers (STEP48) must check `PixelsPerPreviewCell() > 0` (or
equivalently that a composite has been baked at least once) before trusting a picked world
position, same discipline `ResolvePreviewPixel`'s `bInsideImage` already requires callers to
observe.

### 3. `MapCanvasView` exposes its existing screen<->texel ratio (needed by STEP48's pick radius)
`PanByRegionPixels()` already computes `VisibleSpanPixels() / regionSidePixels` privately
(`MapCanvasView_UI.h:96`) — the number of preview texels one screen pixel currently covers.
STEP48's world-space pick radius (icon is a constant SCREEN size; `PickMarker` wants a WORLD
radius) needs this exact ratio too. Expose it instead of recomputing a second copy:
```cpp
// MapCanvasView — public, next to VisibleSpanPixels()
// Preview texels covered by one screen pixel at the current zoom/region size. Zero if
// regionSidePixels is not yet set (mirrors VisibleSpanPixels()'s own zero-when-unset contract).
float PreviewPixelsPerRegionPixel() const {
    return regionSidePixels > 0.0f ? VisibleSpanPixels() / regionSidePixels : 0.0f;
}
```
Repoint `PanByRegionPixels()`'s existing `pixelsPerRegionPixel` local to call this instead of
recomputing the same division inline — one copy, per this file's own stated design principle.

STEP48 composes it with STEP47 item 2's `PixelsPerPreviewCell()`/`settings.worldUnitsPerCell` to
convert a constant on-screen icon pick radius into world units at the current zoom:
`pickRadiusWorldUnits = iconScreenRadiusPixels * view.PreviewPixelsPerRegionPixel() *
settings.worldUnitsPerCell / composite.PixelsPerPreviewCell()`.

## Files touched
- `src/ui/MapCanvasView_UI.h` — `RegionLocalPoint` struct, `ProjectPreviewPixelToRegionLocal()`,
  `PreviewPixelsPerRegionPixel()`; `PanByRegionPixels()` repointed to the new accessor
- `src/ui/PreviewComposite_UI.h` — `PreviewWorldPoint`/`PreviewPixelPoint` structs,
  `PixelsPerPreviewCell()`, `WorldToPreviewPixel()`, `PreviewPixelToWorld()` declarations
- `src/ui/PreviewComposite_Prepare_UI.cpp` — the four new definitions; `BuildEntityPoints()`
  repointed to call `WorldToPreviewPixel()` instead of its old inline math

## Verify
- New unit tests: `MapCanvasView` round-trip (`ResolvePreviewPixel` <-> `ProjectPreviewPixelToRegionLocal`)
  and `PreviewComposite` round-trip (`WorldToPreviewPixel` <-> `PreviewPixelToWorld`), both within
  one pixel/one epsilon.
- Existing `PreviewComposite_*_Test.cpp` suite green with zero edits — `BuildEntityPoints`'s
  observable output (baked mark positions) must be byte-identical before/after the extraction; if
  any existing test needed changing, the refactor was not behavior-preserving.
