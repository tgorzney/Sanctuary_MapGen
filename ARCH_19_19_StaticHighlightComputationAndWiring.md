[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.19. **Only the ARCH Expert writes this file.**

### 19.19 Static selection-highlight — sibling-orbit computation and canvas wiring
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s "Symmetric-sibling computation"
and "Render-side wiring" sections, and items 10/11/14 of its ARCH-rulings list.

**Computation — ratified: fresh, one-shot, discarded every frame — `Pipeline::BuildWorldSymmetryOrbit`
plus a small inline nearest-match, NOT `MarkerOrbitCorrespondence_UI.h`.** Confirmed by direct read:
`MarkerOrbitCorrespondence_UI.h` (`MatchCorrespondenceToOrbit`) is a cross-FRAME stability matcher —
global greedy nearest-pair assignment purpose-built to keep a live drag gesture's siblings correctly
identified as the orbit itself grows/shrinks/reindexes across multiple frames (its own header
comment: a raw frozen orbit-slot index "is NOT safe to re-read every later frame"). A static,
click-driven, recomputed-every-frame-from-scratch highlight has none of that cross-frame drift
problem — reusing the heavier, stateful matcher would be solving a problem this feature doesn't
have. Ratified: stays UI-resident logic, calling the existing PIPELINE query
`Pipeline::BuildWorldSymmetryOrbit(geometry, mask, radialRepeatCount, positionX, positionZ, points,
maxPoints)` (`SymmetryOrbitQuery_PIPELINE.h:34`, already called from UI at multiple existing sites —
`MarkerDragGesture_UI.cpp`, `ArmiesTab_Mirror_UI.cpp` — the same already-legal `UI → PIPELINE`
query-passthrough class §16.3 established, not a new exception). **No new PIPELINE surface.**

Shape: locate the selected transform by `instanceIdentifier` (§19.16; linear scan, cheap at
authoring scale); resolve its effective symmetry via the existing `ResolveEffectiveMarkerSymmetry`
(`MarkerDragGesture_UI.h:67`); call `BuildWorldSymmetryOrbit`; if `orbitCount <= 1`, highlight only
the selected instance; else nearest-match each orbit point beyond slot 0 against the OTHER
transforms in the same `MarkerInstanceGroup`.

**Tolerance reuse — ratified, no new field.** The nearest-match epsilon is
`recipe.markerSymmetryFixSettings.distanceTolerance` (confirmed live, `Symmetry_PARAMS.h:77`,
default 0.5 world units; already exposed as a tab tweakable per `MarkersTab_ManualLayers_UI.h`'s
`fixSymmetryToleranceRange`/`fixSymmetryToleranceToggle`). Same semantic meaning — "how close counts
as the same symmetric position" — as its existing use in the Fix Symmetry command; reuse, not
coincidence, and satisfies Constitution §8's tweakable-not-literal rule for free.

Deliberately NOT `symmetryGroupIdentifier`-equality: confirmed by direct read that "Add Marker"
never populates `symmetryGroupIdentifier` (written only by drag-materialize and the Fix Symmetry
repair tool) — a freshly-authored, never-dragged marker under a symmetric layer would show zero
siblings under an equality approach despite geometric siblings existing. Position-driven orbit
matching is the correct, more complete method.

**Render-side wiring — ratified, same null-safe-injection shape as `SetManualMarkerDragSource`, not
a new module-boundary pattern.** Confirmed by direct read of `MapCanvas_UI.h:98-112`:
`SetActivePanelSource`/`SetScenarioEditModeState`/`SetManualMarkerDragSource` all follow one shape —
a caller-injected pointer (or pointer bundle), defaulted to `nullptr`, checked before use, never
defaulting to "permitted"/"active" on null.
```cpp
void SetManualMarkerSelectionSource(const int* selectedInstanceIdentifier);
// ...
const int* manualMarkerSelectedInstanceIdentifier = nullptr;
```
is the same shape at its simplest — a single scalar pointer, closer to `SetActivePanelSource`'s
one-pointer form than `SetManualMarkerDragSource`'s multi-pointer bundle. Ratified as-is —
confirmation, not a new boundary. Every other input the highlight computation needs (`geometry`,
`globalSymmetryMask`, `radialSymmetryRepeatCount`, `markerSymmetryFixSettings.distanceTolerance`,
the new `selectColor*` fields) is already reachable through the existing
`manualMarkerDragGeometry`/`manualMarkerDragRecipe` pointers `DrawManualMarkerDragPass` threads
today (`MapCanvas_MarkerDrag_UI.cpp:194-204`) — no other new plumbing required.
