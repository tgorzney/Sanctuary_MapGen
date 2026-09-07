# Fix overlay-visibility toggle (dead stopgap draw pass) and missing prop icons (unresolved asset fallback)

**Layer:** UI. **Executor:** SanGen Coder.

## Context

Reproduced on a real (non-SanGen-authored) imported map with populated "Alloys" and "Spawns" marker
groups and ~thousands of props referencing an external game install's `.santp` blueprints. Two
independent defects found by static trace, confirmed by the human's live repro:

1. The View toolbar's "Overlays (screen-space)" popup visibility toggle (`[o]`/`[-]`,
   `DrawVisibilityIcon`, `DraggableListWidget_RowAffordances_UI.h:34-37`) correctly flips
   `OverlayLayer_UI::bEnabled` per marker-type row (Alloy, Spawns each have their own row and own
   `bEnabled` — not aliased) and the gated draw pass (`DrawOverlayIconLayerPass` →
   `MapCanvas_IconLayer_Cull_UI.cpp:90`) correctly honors it. But a second, **entirely separate and
   unconditional** draw pass paints the same markers on top every frame regardless of the toggle.
2. Props never render at all, independent of any toggle state, because their blueprint names never
   resolve to an icon in SanGen's own atlas — this is a real, external-map limitation, not a bug to
   "fix" outright, but it currently fails **silently** with no indication to the user why nothing
   appears.

## Part 1 — retire the dead "STEP94 stopgap" marker draw pass

`MapCanvas_Draw_UI.cpp:44-47`:
```cpp
DrawOverlayIconLayerPass(regionOrigin.x, regionOrigin.y, regionSidePixels);   // gated, correct
// STEP94 — Gap 6's minimal stopgap manual-marker draw, on top of the terrain/overlay stack.
DrawManualMarkerDragPass(regionOrigin.x, regionOrigin.y);                    // NOT gated — bug
```
`DrawManualMarkerDragPass` (`MapCanvas_MarkerDrag_UI.cpp:18-56`) → `DrawManualMarkerRoster`
(`MapCanvas_MarkerRosterDraw_UI.cpp:94-165`) loops every `Params::MarkerInstanceGroup` in
`recipe.markers` unconditionally — no reference to `OverlayLayerSettings`/`bEnabled` anywhere in it.
This was a temporary measure (its own comment says "minimal stopgap") superseded by the real per-layer
overlay pipeline (STEP53) but never retired.

**Fix:** `DrawManualMarkerRoster` must consult the same `OverlayLayerSettings`/`bEnabled` state
`DrawOverlayIconLayerPass` already reads, per marker-type/domain, and skip drawing instances whose
domain is currently disabled — OR, if `DrawManualMarkerDragPass`'s only real remaining purpose is
supporting an active in-progress drag gesture (not a general-purpose marker roster draw), narrow it to
draw *only* the instance(s) actively being dragged (via the existing `manualMarkerDragMarkers` state
already wired at `Application_UI.cpp:181`) rather than the entire roster every frame. **This is a design
choice for the SanGen Coder to confirm against current drag-gesture code before implementing** — read
`InstanceDragGesture_UI.h` and confirm whether a live drag needs the whole-roster draw or just the
dragged instance(s); prefer the narrower fix if sufficient, since it removes the redundant full-roster
iteration entirely rather than adding a second visibility check next to the one that already exists in
the gated pass.

**Note for coordination:** `MapCanvas_MarkerRosterDraw_UI.cpp` is also being touched by a concurrent,
unrelated work-order (`BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI.md`, which changes
`ProjectWorldToScreen` in this same file to call the new shared `WorldToScreen`). Check that work-order's
landed state before editing `DrawManualMarkerRoster`/`DrawManualMarkerDragPass` in this file to avoid a
merge conflict — different functions, but same file.

## Part 2 — surface the prop-icon-resolution miss instead of failing silently

Root cause (confirmed, not a bug in the visibility/layer-default sense — `Props` layer is enabled by
default, `OverlayLayer_Settings_UI.h:35`): `EmitCandidateIfVisible`
(`MapCanvas_IconLayer_CullEmit_UI.cpp:91-96`) drops any candidate whose `IconAtlasPairingLookup::Resolve`
returns `kInvalidIconId` — which happens for every prop in an externally-authored map, since
`pairingLookup` is built only from SanGen's own loaded sanpack (`Application_Assets_UI.cpp:89-128`) and
has no knowledge of the external game install's `.santp` tree. `LogMissOnce` already exists at the miss
site — confirm what it currently does (log-only, presumably to a debug/console log the user doesn't see
in normal use).

**Fix:** on a miss, draw a visible fallback placeholder for the prop instance instead of silently
skipping it — reuse the existing placeholder machinery already built for a different miss case
(`AssetAtlasCache_PropThumbnail_IO.cpp`'s `MakePlaceholderImage`, currently only reached for a
corrupt/missing thumbnail on an *already-recognized* atlas entry) by routing the atlas-name-miss case
through the same placeholder path, so the user sees *something* (a generic marker chip) at every prop's
actual position instead of nothing. Pair this with a one-time, user-visible summary (not just a debug
log) after map load — e.g. "N props could not be matched to an icon and are shown as placeholders" —
surfaced wherever import/load status is already reported to the user (check for an existing
import-summary/toast mechanism before adding a new one).

## Files touched
**Modified:** `MapCanvas_Draw_UI.cpp`, `MapCanvas_MarkerDrag_UI.cpp`, `MapCanvas_MarkerRosterDraw_UI.cpp`
(Part 1); `MapCanvas_IconLayer_CullEmit_UI.cpp`, `AssetAtlasCache_PropThumbnail_IO.cpp` or its caller,
whatever existing import-status/toast surface is found (Part 2); tests covering overlay visibility and
prop icon resolution.

## Acceptance test
1. Load the repro map, open View → Overlays, toggle "Alloys" off: Alloy markers disappear from canvas;
   toggle back on: they reappear. Same for "Spawns". No leftover dots survive a toggle-off.
2. Actively dragging a marker still renders correctly mid-drag (Part 1's narrower-fix path, if taken).
3. Load the repro map: every prop position shows a visible placeholder chip instead of nothing: a
   user-visible message reports how many props fell back to placeholder.
4. A map whose props DO resolve against the loaded sanpack (a SanGen-authored map with ingested props)
   is unaffected — real icons still show, no placeholder, no spurious "N props unresolved" message.
