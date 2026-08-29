# STEP207 — Marquee (left-drag box-select) draws no visual rubber-band rectangle

**Layer:** UI. **Domain:** `src/ui/MapCanvas_Draw_UI.cpp` (new draw pass), `src/ui/MapCanvas_UI.h`
(new method declaration), `src/ui/MapCanvas_GestureOwnership_UI_Test.cpp` (acceptance test extension).
Pure CPU/imgui draw-list layer — no GPU or compute-dispatch involvement, no accuracy-class concern.

## Root problem
The underlying marquee SELECTION logic already works end-to-end and is not touched by this ticket.
`MapCanvas::ApplyPointerInput` (`src/ui/MapCanvas_Draw_UI.cpp:112-187`, post-STEP166-169
gesture-ownership rewrite, ARCH §21.2) resolves a left-button drag that exceeded
`clickDragTolerancePixels` with no manual-instance drag active into a call to `ApplyMarqueeGesture`
(line 168-171), which resolves a world-space AABB and applies the selection via `ApplySelectionGesture`'s
batch overload at mouse-up (ARCH §21.2/§21.6).

**The actual defect: `MapCanvas::Draw` never draws the box itself.** `MapCanvas::Draw`
(`MapCanvas_Draw_UI.cpp:17-58`) issues exactly three overlay draw passes after the composited image —
`DrawOverlayIconLayerPass` (line 44), `DrawManualMarkerDragPass` (line 46), and
`DrawScenarioEditModeOverlayPass` (line 48) — and NONE of them draws a rubber-band rectangle from the
drag's press-start screen position to the live cursor position while a marquee is in progress. The
selection change applies correctly, but silently, at mouse-up — zero visual feedback during the drag
itself. Contrast with right-button pan, which visibly re-renders the view every frame it is held (its
own feedback mechanism), which is why a right-drag reads as "working" while left-drag marquee reads as
completely non-functional even though it isn't.

The press-start point this new pass needs is already tracked: `pressStartRegionLocalX`/
`pressStartRegionLocalY` (`src/ui/MapCanvas_UI.h:339-340`), captured at `ImGui::IsItemActivated()` the
same frame `pressTravelPixels` resets to 0 (`MapCanvas_Draw_UI.cpp:140-142`). The live cursor position
during the drag is available the same way every other pass in this file already reads it
(`ImGui::GetIO().MousePos`). Whether a marquee is currently in progress is exactly the same condition
`ApplyPointerInput` itself uses to decide the release-time branch: `bPressActive == true` AND no manual
instance drag is active (`bManualMarkerDragActive`/`bManualPropDragActive`/`bManualDecalDragActive` all
false, `MapCanvas_UI.h:366,374-375`) — all private `MapCanvas` members, readable from a new `MapCanvas`
method in the same translation unit.

## Fix approach
Add a new private `MapCanvas` method, e.g. `void DrawMarqueeRectanglePass(float regionOriginX, float
regionOriginY)`, defined in `MapCanvas_Draw_UI.cpp` (the file's own header comment already states it is
the only translation unit of the canvas that includes imgui — this new pass belongs here, mirroring how
the existing three passes are thin functions declared alongside the class and called from `Draw()`).
Guard clause: draw nothing unless `bPressActive` is true AND none of `bManualMarkerDragActive`/
`bManualPropDragActive`/`bManualDecalDragActive` is true (a drag gesture, once active, owns the whole
press exclusively per ARCH §21.2 — never show a marquee box mid-drag). Compute the rectangle's two
screen-space corners as `regionOrigin + pressStartRegionLocal*` and `regionOrigin + (current io.MousePos
- regionOrigin)` (the same region-local convention `ApplyPointerInput` itself uses, line 114-115), take
the min/max per axis (a press can end up left-of or right-of its start in either axis). Draw via
`ImGui::GetWindowDrawList()->AddRect(...)` for the outline, optionally `AddRectFilled(...)` with a
translucent fill, using whichever imgui draw-list convention this file's sibling passes already
establish (no dedicated "marquee box" color constant exists yet; a reasonable choice is
`ImGuiCol_TextSelectedBg` or `ImGuiCol_ButtonActive` for the outline — a coder/visual-design decision).

Recommended: draw the box whenever `bPressActive` is true (regardless of whether `pressTravelPixels` has
yet crossed `clickDragTolerancePixels`) — a box drawn while still within click tolerance is
imperceptibly small and causes no incorrect signal, and gating on the threshold would cause a visible
"pop-in" the instant the drag crosses it. UX recommendation, not a hard requirement.

Wire the new pass into `MapCanvas::Draw` (`MapCanvas_Draw_UI.cpp:17-58`) alongside the other three
overlay passes.

## Explicit out-of-scope
- No change to `ApplyPointerInput`, `ApplyMarqueeGesture`, or `ApplySelectionGesture` — the selection
  LOGIC is confirmed already correct; this ticket is visual-only.
- No new persistent style/theme constant is mandated for the box's color — left to the coder.
- Props/Decals get the SAME fix for free (the shared `ApplyPointerInput`/`ApplyMarqueeGesture`
  substrate covers all three collections uniformly) — the new draw pass must be collection-agnostic
  (draw the box once, keyed only on `bPressActive`/no-drag-active, never per collection).
- No change to how the box's resulting selection is computed once released.

## Acceptance test
Extend `src/ui/MapCanvas_GestureOwnership_UI_Test.cpp`, which already drives the real
`ApplyPointerInput` state machine through live imgui frames and already has a marquee-drag scenario
(`SimulatePressDragRelease(canvas, boxPressPosition, boxReleasePosition, /*button=*/0)`). It already runs
a mid-drag frame (button held, mouse moved to the release position, one frame before the button-up
frame) — capture `ImGui::GetWindowDrawList()`'s command/vertex count (or an equivalent draw-data signal)
right after that specific frame's `DrawOneFrame(canvas)` call and assert it is strictly greater than the
same signal captured during `SimulateStationaryClick`'s equivalent frame (a zero-travel click, never a
marquee) — proving a rectangle draw command is emitted during an active marquee drag and is absent for an
ordinary click. Also assert the signal returns to the "no rectangle" baseline once the button-up frame
runs. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.
