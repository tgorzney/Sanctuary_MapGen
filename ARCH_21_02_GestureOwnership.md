[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.2. **Only the ARCH Expert writes this file.**

### 21.2 Gesture ownership — press-time drag-begin-first, release-time click/marquee, the independent right-button pan

**Ratified as designed**, confirmed against `MapCanvas_Draw_UI.cpp`'s real `ApplyPointerInput`
(today: a left-press tries a manual-marker drag first via `TryBeginManualMarkerDrag`; a miss falls
through to LEFT-drag-pans/click disambiguation by travel; Scenario Edit Mode exclusivity is checked
first, unchanged by this ruling). Confirmed `ImGui::IsItemActivated`/`IsItemActive`/
`IsItemDeactivated` are LEFT-button-only for a default-flags `InvisibleButton` — the design's stated
reason a second, independent right-button tracker is needed, not an unverified assumption.

**Left button — press.** Scenario Edit Mode's existing exclusivity check runs first, unchanged.
Otherwise: `TryBeginManualInstanceDrag` (renamed from `TryBeginManualMarkerDrag`) tries a hit across
Markers, Props, and Decals — three calls to §21.3's generic `HitTestManualInstances<GroupT>` (one
per domain), nearest-hit-wins, no fixed collection priority; a same-distance tie keeps the first
domain tested in a fixed evaluation order (Markers, then Props, then Decals), mirroring
`HitTestManualMarkers`'s own existing "lowest index wins a tie" convention one level up. A hit
begins a drag (§21.3); a miss starts neither a drag nor a pan — it merely lets travel accumulate,
exactly as today's `pressTravelPixels` already does for both branches.

**Left button — release.** `pressTravelPixels <= view.settings.clickDragTolerancePixels`: a click,
regardless of whether a drag gesture was active (today's existing "a gesture that never moved is a
click in disguise" rule, `MapCanvas_Draw_UI.cpp:158-165`, generalized to all three domains
unchanged) — resolves through §21.1's `ApplySelectionGesture` single-key overload with the frame's
live Ctrl/Shift state (`io.KeyCtrl`/`io.KeyShift`). Whether this click resolution re-runs the
hit-test from scratch or reuses press-time's already-known hit is implementation freedom, not ruled
here — behaviorally equivalent either way. `pressTravelPixels > clickDragTolerancePixels` AND no
drag was active: a marquee release. The press-start region-local point (a NEW pair of tracking
fields on `MapCanvas`, mirroring `pressTravelPixels`'s own naming convention — e.g.
`pressStartRegionLocalX`/`pressStartRegionLocalY`, captured at `IsItemActivated()` the same frame
`pressTravelPixels` resets to 0) and the release-time region-local point resolve, via the SAME
region-local -> preview-pixel -> world chain `ApplyClick` already uses, to ONE world-space
axis-aligned rectangle (whichever corner pair is min/max — a press can end up left-of or right-of
its start in either axis). That one rectangle feeds BOTH §21.6's `PickInstancesInRegion` (against
each of `Data::SpatialGridSet`'s `markers`/`props`/`decals` grids — Units explicitly OUT OF SCOPE,
see §21's own closing note) and §21.3's `CollectManualInstancesInWorldRegion<GroupT>`
(Markers/Props/Decals, lock-gated per §21.5). Every resulting key across all six queries is
concatenated into one ordered list and resolved through §21.1's `ApplySelectionGesture` batch
overload with the release frame's Ctrl/Shift state. A drag that WAS active never reaches marquee
resolution — its own end-of-gesture handling (§21.3) is exclusive for that press.

**Right button — independent tracker, replaces left-drag-pans entirely.** New private state
mirroring `bPressActive`/`pressTravelPixels`'s existing shape but keyed off `ImGuiMouseButton_Right`/
raw `io.MouseDown[1]` (imgui's item-activation does not see button 1): begins when
`ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)` — the SAME idiom
`ScenarioEditModePointerFrame_UI::bRightClicked` already uses in this exact file
(`MapCanvas_Draw_UI.cpp:125`), a real, verified precedent, not an invented convention; continues
every frame `io.MouseDown[ImGuiMouseButton_Right]` is true, independent of hover (mirroring how
`IsItemActive()` persists for the left button after the cursor leaves the item once a press began);
ends on `ImGui::IsMouseReleased(ImGuiMouseButton_Right)`. While active, every frame's
`io.MouseDelta` drives `ApplyDrag` directly — the ENTIRE content of today's left-button pan branch
(`MapCanvas_Draw_UI.cpp:149-151`), moved onto this new tracker and deleted from the left-button
path. Scenario Edit Mode's own exclusivity gate covers this too (checked first, unchanged) — no
right-button pan while it owns the canvas.
