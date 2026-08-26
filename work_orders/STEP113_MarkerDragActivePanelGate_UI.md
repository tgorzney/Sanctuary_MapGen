# STEP113 — Marker drag gated on Markers tab being the active panel

**Layer:** UI (framework wiring, gate check). **Domain:** `MapCanvas_UI.h`, `Application_UI.cpp`,
`MapCanvas_MarkerDrag_UI.cpp`. **Sequence:** independent of STEP111/STEP112/STEP114 — no shared
call sites, safe to implement in any order or concurrently.

## Problem
`MapCanvas` (`src/ui/MapCanvas_UI.h`) draws and processes pointer input for the preview canvas
every frame regardless of which left-column panel (`Ui::ApplicationPanel`) is currently active —
the canvas is a persistent center view, not scoped to a tab. `TryBeginManualMarkerDrag`
(`src/ui/MapCanvas_MarkerDrag_UI.cpp:101-114`), the press-time entry point STEP94 wired into
`ApplyPointerInput`'s press-vs-pan disambiguation (`MapCanvas_Draw_UI.cpp:128-131`), will begin a
manual-marker drag on ANY press that hits a marker (`HitTestManualMarkers`), on ANY panel — e.g.
while the user is on the Heightmap or Water tab with the Markers roster merely visible behind it.
Today the only gate on drag-begin is `IsMarkerInstanceLayerLocked` (STEP106, confirmed shipped at
`MarkerDragGesture_UI.cpp:43` inside `BeginMarkerDragGesture`, and `MarkerDragGesture_UI.cpp:84`
inside `RepositionSymmetryGroupMember`). There is no check that the Markers panel is the one the
user is actually looking at, so a marker can be accidentally dragged from a tab where the user has
no visual context for what they just moved.

`MapCanvas` has NO reference to `Ui::ApplicationTabState`/`ApplicationPanel` today — confirmed by
reading `MapCanvas_UI.h` in full (lines 1-187): the only comparable injected interaction-ownership
state is `ScenarioEditModeState* scenarioEditModeState = nullptr;` (line 173), set via
`SetScenarioEditModeState` (line 91) and wired once in `Application::WireCallbacks()`
(`Application_UI.cpp:93`, `canvas.SetScenarioEditModeState(&scenarioEditMode);`), read every frame
with a null-checked dereference in `MapCanvas_Draw_UI.cpp` (lines 55, 60, 111, 119).
`Ui::ApplicationPanel` (`src/ui/Application_Panels_UI.h:26-30`) is a plain enum,
`ApplicationPanel::Markers` its confirmed literal (line 28). `Ui::ApplicationTabState::activePanel`
(`src/ui/Application_TabState_UI.h:73`) is the field naming the currently active tab, default
`ApplicationPanel::Heightmap`.

## Fix

### 1. New member + setter — `MapCanvas_UI.h`
New include, first in the file's `ui/`-local include block (alphabetically before
`MapCanvasView_UI.h`):
```cpp
#include "Application_Panels_UI.h"
```
New setter, immediately after `SetScenarioEditModeState` (current line 91), same injected-pointer
posture:
```cpp
    // STEP113 — the active-panel gate: a manual-marker drag may only BEGIN while the Markers panel
    // is the shell's active tab. Mirrors SetScenarioEditModeState exactly (same class of injected,
    // caller-owned, read-every-frame pointer; see this header's own comment on why a second copy of
    // this state is never made).
    void SetActivePanelSource(const ApplicationPanel* panel) { activePanelSource = panel; }
```
New member, immediately after `scenarioEditModeState` (current line 173):
```cpp
    // STEP113 — mirrors scenarioEditModeState exactly: injected, caller-owned, read every frame.
    const ApplicationPanel* activePanelSource = nullptr;
```
`MapCanvas_UI.h` including `Application_Panels_UI.h` is legal — both `Ui`-layer, no Constitution
§1/§3 boundary crossed. `Application_Panels_UI.h` itself includes only
`PreviewComposite_Settings_UI.h`, confirmed no include cycle back to `MapCanvas_UI.h`.

### 2. One-time wiring — `Application::WireCallbacks()`
`Application_UI.cpp:93` today:
```cpp
    canvas.SetScenarioEditModeState(&scenarioEditMode);
```
`WireCallbacks()` is a member function of `Application`, and `Application` owns
`ApplicationTabState tabState;` as a plain member (`Application_UI.h:164`, confirmed; accessor
`ApplicationTabState& TabState()` at line 105 also confirmed) — `tabState` is reachable in this
scope with no new parameter or injection plumbing. Add immediately after line 93:
```cpp
    // STEP113 — the active-panel gate; see MapCanvas_UI.h's SetActivePanelSource. Points at the
    // shell's own live tabState.activePanel — one source of truth, never a second copy (same
    // posture as every other canvas.Set*Source call in this function).
    canvas.SetActivePanelSource(&tabState.activePanel);
```

### 3. Gate check — `MapCanvas::TryBeginManualMarkerDrag`
`TryBeginManualMarkerDrag` (`MapCanvas_MarkerDrag_UI.cpp:101-114`) is the correct insertion point,
not `BeginMarkerDragGesture`: it is the canvas-level entry point that already has direct access to
`activePanelSource` as a `MapCanvas` member (mirroring how it already reads
`manualMarkerDragMarkers`/`manualMarkerDragLayers`/etc.), whereas `BeginMarkerDragGesture`
(`MarkerDragGesture_UI.cpp`) is a free, imgui-free, pure function taking only recipe-shaped
parameters (`markers`, `markerLayers`, `geometry`, ...) with no concept of "the shell's active
panel" — threading a panel enum into that pure gesture-state-machine function would be a layering
smell this ticket does not need. Gating at `TryBeginManualMarkerDrag` keeps the change smallest and
matches the shape of every other MapCanvas-level ownership gate (`scenarioEditModeState`'s own
`IsActive()` check lives in `MapCanvas_Draw_UI.cpp`, at the canvas level, not inside a pure
gesture/geometry function either).

Current (`MapCanvas_MarkerDrag_UI.cpp:101-114`):
```cpp
bool MapCanvas::TryBeginManualMarkerDrag(float regionLocalX, float regionLocalY) {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr
        || manualMarkerDragRecipe == nullptr || composite == nullptr) return false;
    int hitGroupIndex = -1, hitTransformIndex = -1;
    if (!HitTestManualMarkers(*manualMarkerDragMarkers, *composite, view, regionLocalX, regionLocalY,
                              pickRadiusScreenPixels, hitGroupIndex, hitTransformIndex))
        return false;
    ...
```
Becomes:
```cpp
bool MapCanvas::TryBeginManualMarkerDrag(float regionLocalX, float regionLocalY) {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr
        || manualMarkerDragRecipe == nullptr || composite == nullptr) return false;
    // STEP113 — a drag may only BEGIN while the Markers panel is active. Guard-clause negated-OR
    // form, matching this function's OWN existing null-check style immediately above (not
    // DrawScenarioEditModeOverlayPass's positive "!= nullptr && ->IsActive()" gate-and-proceed
    // form) — both are the same null-safety posture, applied as the shape each call site already
    // uses. Null (no shell has wired a panel source, e.g. a test harness) refuses, never defaults
    // to permitting a drag — same null-safe-refuses posture as the existing scenarioEditModeState
    // pointer (MapCanvas_UI.h:173), not a new convention.
    if (activePanelSource == nullptr || *activePanelSource != ApplicationPanel::Markers) return false;
    int hitGroupIndex = -1, hitTransformIndex = -1;
    if (!HitTestManualMarkers(*manualMarkerDragMarkers, *composite, view, regionLocalX, regionLocalY,
                              pickRadiusScreenPixels, hitGroupIndex, hitTransformIndex))
        return false;
    ...
```
Placed before `HitTestManualMarkers` (not after) so an off-panel press never pays the O(markers)
scan cost. `MapCanvas_MarkerDrag_UI.cpp` already `#include "MapCanvas_UI.h"` (line 6), which now
transitively includes `Application_Panels_UI.h` — no new include needed in this `.cpp`.

This combines with, does not replace, the existing lock gate: `IsMarkerInstanceLayerLocked` still
runs inside `BeginMarkerDragGesture` (`MarkerDragGesture_UI.cpp:43`), reached only if
`TryBeginManualMarkerDrag`'s new panel check passes — both gates must independently pass for a
drag to actually begin. They are not the same call site (one is a canvas-level guard clause before
hit-testing even runs; the other is inside the pure gesture function hit-testing feeds into) but
are cumulative in effect.

### 4. `RepositionSymmetryGroupMember` / mid-drag tab switch — reasoned conclusion
**`RepositionSymmetryGroupMember` needs no gate from this ticket.** Confirmed by grep
(`RepositionSymmetryGroupMember` matches across `src/`): its only call sites are
`MarkerDragGesture_UI_Test.cpp:236,242,278` (test fixtures) — there is **no production call site**
today. Its own header comment (`MarkerDragGesture_UI.h:122-124`) confirms this explicitly: "the
roster-slider counterpart... STEP94's own file is NOT wired to call this by this ticket." The
roster Position sliders (`DrawSelectedMarkerInstance`, `MarkersTab_ManualInstance_UI.cpp`) write
positions directly via `DrawSliderScalar`, gated by STEP106's `IsMarkerInstanceLayerLocked`
`BeginDisabled`/`EndDisabled` wrap — they are drawn only while the Markers panel itself is the
active panel and expanded (they live inside that panel's own draw call), so an active-panel gate
there is structurally redundant, not a gap.

**Gating BEGIN only is sufficient — a mid-drag tab switch cannot happen.** Reasoning, grounded in
the real code:
- `Ui::ApplicationTabState::activePanel` is written in exactly one place in the whole codebase
  (confirmed by grep for `activePanel\s*=`): `Application_LeftColumn_UI.cpp:30`,
  `if (ImGui::Selectable(entry.label, activePanel == entry.panel)) activePanel = entry.panel;` — a
  standard imgui `Selectable` click, which imgui only fires when BOTH press and release land on
  that exact widget while it holds mouse-button ActiveId capture.
- While a manual-marker drag is in progress, the canvas's own `InvisibleButton`
  (`MapCanvas_Draw_UI.cpp:51`) holds that same mouse button's ActiveId for the ENTIRE gesture — set
  on `IsItemActivated()` (line 128), held through `IsItemActive()` (lines 132-140), released only on
  `IsItemDeactivated()` (line 141), which unconditionally calls `EndManualMarkerDrag()` (lines
  143-145) the same frame.
- Standard imgui semantics: only one widget can hold a given mouse button's ActiveId at a time: the
  `Selectable` cannot register its own press-then-release click while the canvas's `InvisibleButton`
  still holds that capture. The user must release the mouse (over the canvas or off it) first —
  which ends the drag unconditionally via `EndManualMarkerDrag()` — before a fresh, independent
  press-and-release on the `Selectable` can fire and change `activePanel`.
- Therefore no sequence of ordinary mouse input can leave `manualMarkerDragState.bActive == true`
  spanning a frame where `activePanel` has also changed. No guard is needed in
  `ContinueManualMarkerDrag`, `UpdateMarkerDragGesture`, or `EndManualMarkerDrag` for this ticket.

## Out of scope
- **Props/Decals gating.** `PropsTab`/decal placement have no equivalent canvas-drag path today
  (confirmed: `SetManualMarkerDragSource` and `TryBeginManualMarkerDrag` are markers-only, STEP94's
  own scope) — nothing to gate. Matches every prior marker-only ticket's domain split
  (STEP60, STEP68, STEP81, STEP106).
- **Gating `RepositionSymmetryGroupMember`.** Dead in production today (§4) — out of scope; do not
  wire a new production call site to it as part of this ticket.
- **Any change to `IsMarkerInstanceLayerLocked` or the STEP106 lock-check call sites.** Untouched;
  this ticket adds a second, independent, cumulative gate — it does not touch the first.
- **A guard in `ContinueManualMarkerDrag`/`UpdateMarkerDragGesture`/`EndManualMarkerDrag`.** §4's
  reasoned conclusion: unreachable, not a gap left open.
- **Any visual affordance for the refusal** (e.g. a cursor change, a tooltip when a press on a
  marker is refused because the wrong panel is active). `TryBeginManualMarkerDrag` returning `false`
  already falls through to the normal pan/click path unchanged (`MapCanvas_Draw_UI.cpp:132-140`) —
  same posture as a locked-layer refusal, which also has no dedicated visual cue beyond the
  press-becomes-a-pan behavior itself.

## Files touched
- `src/ui/MapCanvas_UI.h` — new `#include "Application_Panels_UI.h"`; new `SetActivePanelSource`
  setter; new `activePanelSource` member
- `src/ui/Application_UI.cpp` — `WireCallbacks()` gains one new wiring line,
  `canvas.SetActivePanelSource(&tabState.activePanel);`, immediately beside the existing
  `SetScenarioEditModeState` call
- `src/ui/MapCanvas_MarkerDrag_UI.cpp` — `TryBeginManualMarkerDrag` gains the active-panel guard
  clause, before `HitTestManualMarkers`
- `src/ui/MapCanvas_ActivePanelGate_UI_Test.cpp` — **new file**, headless acceptance coverage (see
  Verify)
- `src/ui/MapCanvas_UI_Test.cpp` — new `extern` declaration + call site for the new test's runner
  function, alongside the existing `RunMapCanvasScenarioEditModeOwnershipChecks` call (both need a
  real GL context past `MapCanvas::Draw`'s "nothing composited" early return, per that file's own
  header comment)

## Verify
Acceptance bar: a press-drag-release on a manual marker moves it only while
`ApplicationPanel::Markers` is the active panel; the identical gesture on any other panel (or with
no panel source wired at all) falls through to the normal pan behavior and leaves the marker
untouched.

This IS headlessly testable — a strong, directly-analogous precedent already exists:
`MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp` (`RunMapCanvasScenarioEditModeOwnershipChecks`)
already drives real live imgui frames with a synthetic press-drag-release gesture against a real
`MapCanvas`+GL-backed `PreviewComposite`, asserting on an ownership gate of the exact same shape
(one interaction-ownership pointer, gates whether a gesture is allowed to begin). Mirror it exactly
rather than inventing a new technique, extended with `SetManualMarkerDragSource` (which that file
never calls, confirming no existing test's assertions are affected by this ticket — see below) and
a marker positioned at a known world point (mirroring `ScreenPointFor`,
`MapCanvas_MarkerDrag_UI_Test.cpp:66-69`, to resolve press coordinates).

- **New test file `MapCanvas_ActivePanelGate_UI_Test.cpp`**, `RunMapCanvasActivePanelGateChecks(Sys::GpuResourceManager&
  manager)`:
  - Build a `PreviewTestScene`/`PreviewComposite` (mirroring
    `MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp`'s own fixture construction exactly), a
    one-entry `markers`/`markerLayers` fixture with an unlocked layer and one ungrouped
    `MarkerTransform` at a known world position, a `Params::Geometry`, a `Params::MapRecipe`.
    Construct `MapCanvas`, wire `SetPreviewTexture`/`SetPreviewComposite`/`SetOverlayRecipe` (as the
    precedent test does) plus `SetManualMarkerDragSource(&markers, &markerLayers, &geometry,
    &recipe)`. Compute the marker's expected press point via `composite.WorldToPreviewPixel` +
    `view.ProjectPreviewPixelToRegionLocal` (mirroring `ScreenPointFor`).
  - **Case 1 — Markers active**: a local `ApplicationPanel activePanel = ApplicationPanel::Markers;`,
    `canvas.SetActivePanelSource(&activePanel);`. Simulate a press-drag-release at the marker's
    screen point with a real delta (mirroring `SimulatePressDragRelease`,
    `MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp:44-63`). Assert the marker's
    `transform.positionX`/`positionZ` changed (the drag began and followed the cursor) — the same
    "assert on the actually-moved state" shape STEP94's own drag tests already use.
  - **Case 2 — a non-Markers panel active**: set `activePanel = ApplicationPanel::Heightmap;`
    (same `MapCanvas`/fixture, marker's position first reset to its Case-1-starting value). Identical
    press-drag-release at the identical screen point. Assert the marker's position is UNCHANGED (the
    drag was refused) AND, mirroring the precedent test's own dual-assertion shape, that the view's
    pan center DID move (`canvas.View().ViewCenterPixelX()/ViewCenterPixelY()` differ from before —
    proving the press fell through to the normal pan path, not merely that nothing happened).
  - **Case 3 — no panel source wired (`activePanelSource` left at its `nullptr` default)**: same
    fixture, `SetActivePanelSource` never called. Identical press-drag-release. Assert the marker's
    position is UNCHANGED — null must refuse, never default-permit (Constitution §6), matching this
    ticket's Fix §3 comment.
- **`MapCanvas_UI_Test.cpp`** — add `extern void RunMapCanvasActivePanelGateChecks(Sys::GpuResourceManager&
  manager);` beside the existing `RunMapCanvasScenarioEditModeOwnershipChecks` declaration, and call
  it in `main()` immediately after that call (same GL-context-gated block).
- **No existing test regresses**: confirmed by grep that `SetManualMarkerDragSource` is called only
  in production (`Application_UI.cpp:97`) — no existing test file calls it, so `TryBeginManualMarkerDrag`'s
  existing null-check at the top (`manualMarkerDragMarkers == nullptr`) already short-circuits before
  reaching the new gate in every pre-existing test, including
  `MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp`'s own "Mode OFF: pans normally" case. The new gate
  is exercised for the first time by this ticket's own new test file.
