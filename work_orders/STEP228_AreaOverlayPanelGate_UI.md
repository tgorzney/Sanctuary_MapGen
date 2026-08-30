# STEP228 — Area selection chrome must disappear when the Areas panel isn't active

## Summary
The human's bug report: "There is an error where leaving the areas tab, leaves the control
handles for a selected area still showing, they should disappear if Areas is disabled."

Confirmed by direct code reading this session: `MapCanvas::DrawAreaOverlayPass`
(`src/ui/MapCanvas_AreaDraw_UI.cpp:27-84`) draws the selected area's border and 8 resize handles
gated ONLY on `selectedIndex >= 0` (line 43) and, for the border specifically, on the MapAreas
field layer's `bEnabled` flag (line 45, `IsMapAreasLayerEnabled`). Neither condition has anything
to do with which tab/panel is currently active. `MapCanvas::AreaGestureEligible()`
(`src/ui/MapCanvas_AreaDragDispatch_UI.cpp:26-28` — `return activePanelSource != nullptr &&
*activePanelSource == ApplicationPanel::Areas;`) already exists and already gates every INPUT/
gesture path (`TryBeginAreaDrag`, the click-release deselect handler in `MapCanvas_Draw_UI.cpp`) —
but it is never called from the draw path at all. Separately, `AreasTabState::selectedAreaIndex`
is never reset when the active panel changes away from Areas (confirmed: the panel-switch handler,
`Application_LeftColumn_UI.cpp`'s `DrawPanelRow`, only mutates `activePanel`, nothing else) — so
the stale selection persists and `DrawAreaOverlayPass` keeps drawing its chrome every frame on
every OTHER tab too.

There is already an established precedent for exactly this shape of fix in this codebase — Scenario
Edit Mode auto-exits when its owning panel loses focus (`Application_Draw_UI.cpp:48-52`:
"browsing away from the Scenarios panel closes its detail panel... Scenario Edit Mode must not
keep exclusive canvas ownership behind it"). This ticket applies the equivalent gate to the Area
overlay draw pass, reusing the SAME `AreaGestureEligible()` check the gesture code already trusts —
not a new concept, not a second independently-maintained eligibility rule.

## Required reading
- `src/ui/MapCanvas_AreaDraw_UI.cpp` (full file, 88 lines) — the function this ticket edits.
- `src/ui/MapCanvas_AreaDragDispatch_UI.cpp:26-28` (`AreaGestureEligible`) — confirm it is a
  `MapCanvas::` member function, callable directly from `DrawAreaOverlayPass` (also a `MapCanvas::`
  member) with zero new plumbing.
- `src/ui/Application_Draw_UI.cpp:48-52` — the Scenario Edit Mode auto-exit precedent this ticket's
  reasoning is modeled on (read-only reference, not itself touched by this ticket).
- `src/ui/MapCanvas_Draw_UI.cpp:42-53` — `DrawAreaOverlayPass`'s call site, to confirm nothing else
  needs to change there.

## 1. `src/ui/MapCanvas_AreaDraw_UI.cpp` — one early-return gate

Change the top of `DrawAreaOverlayPass` from:
```cpp
void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
```
to:
```cpp
void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
    // STEP228 — the SAME eligibility check every Area input/gesture path already trusts
    // (TryBeginAreaDrag, the click-release deselect handler): the border, the 8 resize handles, AND
    // the hover cursor-shape feedback below are all panel-scoped chrome. A stale selectedAreaIndex
    // surviving a switch to another tab must not keep drawing this area's chrome over whatever the
    // human is actually looking at now.
    if (!AreaGestureEligible()) return;
```
Nothing else in the function changes — this single early return covers both the border/handle
block (lines 43-56) and the hover cursor-shape feedback block (lines 58-83) in one gate, since both
are equally "Areas panel only" concerns and neither should run on any other tab.

## ARCH rules invoked
- ARCH §21.8 (the section this whole file is already scoped under) — this ticket does not touch
  any Z-order/gesture semantic already ratified there, it only adds the missing panel-scope gate
  the gesture code already had and the draw code was missing.
- Constitution — reuses an existing, already-tested eligibility function rather than inventing a
  second, parallel "is Areas active" check.

## Explicit out-of-scope
- No change to `AreaGestureEligible()` itself, `MapCanvas_AreaDragDispatch_UI.cpp`, or any input/
  gesture code — it is already correct; only the draw path was missing the same check.
- No reset of `AreasTabState::selectedAreaIndex` on panel switch — deliberately NOT done. The
  human's report is specifically about the CHROME disappearing while away from the tab; nothing
  asked for the selection itself to be forgotten. Returning to the Areas tab should still show the
  same area selected, exactly as before this fix — only the cross-tab leakage is removed. If the
  human later wants the selection itself cleared on tab-away (mirroring Scenario Edit Mode's own
  full deactivation), that is a separate, deliberate follow-up ask, not assumed here.
- No change to `IsMapAreasLayerEnabled`'s border-only gating, or to the "handles draw regardless of
  layer-enabled" rule (ARCH §14.18 item 9, explicitly cited in this file's own comment as
  unchanged/still-law) — this ticket adds a panel gate on top of, not instead of, those existing
  rules.

## Acceptance test
- A test driving `MapCanvas::DrawAreaOverlayPass` (or whatever test file already exercises it —
  likely `MapCanvas_UI_Test.cpp` or a sibling) with a valid `selectedAreaIndex` but
  `activePanelSource` pointing at a NON-Areas panel: confirm no border/handle draw calls occur (no
  `AddRect`/`AddCircleFilled` for the area), and no `ImGui::SetMouseCursor` resize-cursor call
  occurs even when the mouse hovers a handle's screen position.
- The same scenario with `activePanelSource` pointing AT `ApplicationPanel::Areas`: confirm chrome
  still draws exactly as before this ticket (no regression to the existing behavior).
- If any EXISTING test exercising `DrawAreaOverlayPass`'s chrome output does not currently wire
  `activePanelSource` to `ApplicationPanel::Areas` at all (i.e. it relied on the previously-missing
  gate to not care), it will now need to do so to keep passing — wire it, don't weaken the
  assertion, since gesture-path tests already had to do exactly this for `TryBeginAreaDrag`.
- Full existing test suite: zero regressions.
