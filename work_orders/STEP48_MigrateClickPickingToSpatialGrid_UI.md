# STEP48 — Migrate click-picking off the baked entity-id buffer onto SpatialGrid

**Layer:** UI. **Domain:** `MapCanvas`, `Picking_UI`. **Sequence:** Phase 1.2,
`work_orders/SEQUENCE_PreviewOverlayLayering.md`. **Depends on STEP47** (uses
`PreviewComposite::PreviewPixelToWorld()` and `MapCanvasView::PreviewPixelsPerRegionPixel()`).

## Problem
`MapCanvas::ApplyClick()` (`src/ui/MapCanvas_UI.cpp:31-39`) resolves a click by reading
`Data::EntityIdBuffer` — a per-pixel raster baked at composite time, in **texel space** — via
`Picking_UI::PickEntity()`. This ties picking to whatever was last baked into the shared
composite texture, the same texel-space coupling that causes markers to scale with zoom
(ARCH_14_PreviewOverlayLayering.md §14's whole reason for existing). It is also already redundant: `Picking_UI::PickMarker()`
(`src/ui/Picking_UI.h:46-48`) — an O(1) `Data::SpatialGrid` chunk hit-test against
`Data::PlacementInstances` directly, in **world space** — is fully implemented and tested
(`Picking_UI_Test.cpp`) and is never called anywhere in `src/`.

## Why the public API doesn't change
`MapCanvas::SelectedEntityIdentifier()` returns `std::uint32_t`. Today that value is whatever
`PickEntity` read from the id buffer, which the composite bakes as
`static_cast<unsigned int>(instance)` — the **index into the same markers SoA**
(`PreviewComposite_Prepare_UI.cpp`'s `BuildEntityPoints`, pre-STEP47). `Picking_UI::PickMarker`
returns `std::int32_t`, sentinel `kNoMarkerPicked = -1`. Casting `-1` to `std::uint32_t` is
`0xFFFFFFFF` — **identical** to `Data::EntityIdBuffer::emptySentinel`. So the migration changes
*how* the selected index is computed, not *what it means* or its bit pattern for "nothing
selected" — every existing caller of `SelectedEntityIdentifier()`/`HasSelection()` keeps working
unmodified.

## Fix

### 1. Wire the picking source, same pattern as the retiring `SetEntityIdentifierBuffer`
```cpp
// MapCanvas_UI.h
#include "../data/PlacementInstances_DATA.h"
#include "../data/SpatialGrid_DATA.h"
// ...
// What a click is resolved against: the resolved markers and the spatial index over them,
// both read-only, both owned by PIPELINE (Generation_PIPELINE, M4-5). Replaces
// SetEntityIdentifierBuffer — see STEP48.
void SetMarkerPickingSource(const Data::PlacementInstances* markerInstances,
                            const Data::SpatialGrid* markerSpatialGrid) {
    pickMarkerInstances = markerInstances;
    pickMarkerSpatialGrid = markerSpatialGrid;
}
// New: the constant on-screen radius a click must land within to hit a marker icon
// (Constitution §8 — a named setting, not a literal). Matches Phase 3's icon draw radius; the
// two must agree or a click can miss a visibly-hit icon.
void SetMarkerPickRadiusScreenPixels(float radius) { pickRadiusScreenPixels = radius; }
```
Remove `SetEntityIdentifierBuffer()` and the `entityIdentifiers` member — `PickEntity`'s only
caller (confirmed, `src/` grep) was `MapCanvas::ApplyClick`.

⚠️ **Do not remove `Data::EntityIdBuffer` itself, its GPU write pass, or `PickEntity()` in this
work-order.** That is a separate, larger change (`PreviewComposite_Cpu_UI.cpp`,
`PreviewComposite_UI.glsl`, `PreviewComposite_GpuBuffers_UI.cpp`, and STEP46's now-orphaned
entity-id readback line) — scope it as its own follow-up once this lands and the buffer has zero
remaining consumers. This work-order only removes `MapCanvas`'s *read* of it.

### 2. `ApplyClick` composes STEP47's inverse projection with `PickMarker`
```cpp
// MapCanvas_UI.cpp
std::uint32_t MapCanvas::ApplyClick(float regionLocalX, float regionLocalY) {
    lastPickedPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    if (!lastPickedPixel.bInsideImage || pickMarkerInstances == nullptr
        || pickMarkerSpatialGrid == nullptr || composite == nullptr
        || composite->PixelsPerPreviewCell() <= 0.0f) {
        SetSelection(Data::EntityIdBuffer::emptySentinel);
        return selectedEntityIdentifier;
    }
    const PreviewComposite::PreviewWorldPoint worldPoint =
        composite->PreviewPixelToWorld(static_cast<float>(lastPickedPixel.pixelX),
                                       static_cast<float>(lastPickedPixel.pixelY));
    const float pickRadiusWorldUnits = pickRadiusScreenPixels
        * view.PreviewPixelsPerRegionPixel()
        * composite->Settings().worldUnitsPerCell / composite->PixelsPerPreviewCell();
    const std::int32_t pickedIndex = PickMarker(*pickMarkerSpatialGrid, *pickMarkerInstances,
                                                worldPoint.worldX, worldPoint.worldZ,
                                                pickRadiusWorldUnits);
    SetSelection(static_cast<std::uint32_t>(pickedIndex));   // kNoMarkerPicked(-1) == emptySentinel
    return selectedEntityIdentifier;
}
```
**RESOLVED — ARCH ruling, this introduces `MapCanvas`'s first dependency on `PreviewComposite`, and
that's ratified as correct.** `MapCanvas` gets a `const PreviewComposite*` member, injected the
same way it already takes `Sys::GpuResourceManager*`:
```cpp
// MapCanvas_UI.h
void SetPreviewComposite(const PreviewComposite* previewComposite) { composite = previewComposite; }
```
wired in `Application_UI.cpp`'s `WireCallbacks()` alongside `SetMarkerPickingSource`. Ruling
reasoning: `MapCanvas_UI` and `PreviewComposite_UI` are both `UI` layer — this is an intra-layer
edge, not a dependency-table violation. The rejected alternative (`Application` re-pushing derived
numbers via a callback) would create a second copy of state that must be kept in sync with
`PreviewComposite`'s live baked state — exactly the "second copy... is exactly how a picker drifts
from its index" anti-pattern `SpatialGrid_DATA.h`'s own header warns against, which this ticket's
own Problem section already cites. `MapCanvas_UI.h`'s "never a `_UI` sibling module" line was a
description of pre-STEP47/48 code, not a standing rule — update that header comment to drop it and
document the new `PreviewComposite*` dependency plainly, same as the file documents its other
injected pointers.

### 3. Wire the new source at the `Application` level, alongside where `SetEntityIdentifierBuffer` is removed
```cpp
// Application_UI.cpp, WireCallbacks()
canvas.SetMarkerPickingSource(&assembler.Placements().markers, &assembler.MarkerSpatialGrid());
canvas.SetMarkerPickRadiusScreenPixels(settings.markerIconRadiusPixels);  // new named setting,
                                                                           // shared with Phase 3's
                                                                           // icon draw radius
```
`assembler.Placements().markers` (`Data::PlacementInstances`) and `assembler.MarkerSpatialGrid()`
(`Data::SpatialGrid`) already exist and are already built after Placement
(`GenerationAssembler_PIPELINE.h:63,70`, `GenerationAssembler_Stages_PIPELINE.cpp`'s
`BuildMarkerSpatialGrid()`) — no new PIPELINE wiring needed, only a new consumer of data that
already exists.

## Files touched
- `src/ui/MapCanvas_UI.h` — replace `SetEntityIdentifierBuffer`/`entityIdentifiers` with
  `SetMarkerPickingSource`/`SetMarkerPickRadiusScreenPixels` and their backing members
- `src/ui/MapCanvas_UI.cpp` — `ApplyClick()` rewritten per above
- `src/ui/Application_UI.cpp` — `WireCallbacks()` wires the new source instead of the old buffer
- `src/ui/Application_Settings_UI.h` (or wherever `ApplicationSettings` lives) — new
  `markerIconRadiusPixels` setting (Constitution §8), shared with Phase 3

## Verify
- `Picking_UI_Test.cpp`'s existing `PickMarker` tests are unaffected (function itself unchanged).
- Update `PreviewIntegration_Picking_UI_Test.cpp` (currently exercises the old
  `EntityIdBuffer`-backed click path, per its name) to exercise the new source — this is the one
  test file expected to need real edits, not a red flag like STEP46's zero-edits bar.
- New test: click on a marker's exact world position at several zoom levels selects the same
  marker index regardless of zoom (proves the texel-space coupling is actually gone).
- New test: click just outside `pickRadiusScreenPixels` of every marker selects nothing
  (`emptySentinel`), at more than one zoom level (proves the screen-space radius conversion, not
  just the world-space math, is correct).
