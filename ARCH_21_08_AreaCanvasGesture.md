[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.8. **Only the ARCH Expert writes this file.**

### 21.8 Area canvas gesture — create-by-drag, 8-handle resize + body-move for `Params::MapArea`, its own hand-written substrate

Ratifies track A of the human-approved Areas-on-canvas plan (a second track, about Scenario data,
is being ratified separately and is not this section's concern). Independently verified against the
live code before ruling, not taken on the design's word alone: `MapCanvas_UI.h`, `MapCanvas_Draw_UI.cpp`,
`MapCanvas_ManualDragDispatch_UI.cpp`, `MapCanvas_SelectionGesture_UI.cpp`, `MapCanvas_ManualDragSources_UI.h`,
`MapCanvas_MarkerDrag_UI.cpp`, `AreasTab_UI.h`/`.cpp`, `AreasTab_List_UI.h`, `MapArea_PARAMS.h`,
`MapRecipe_PARAMS.h`, `Geometry_PARAMS.h`, `Application_Panels_UI.h`, `Application_TabState_UI.h`,
`Application_PanelEnvironment_UI.cpp`, `Application_UI.cpp`, `PropDragGesture_UI.h`,
`InstanceDragGesture_UI.h`, `ManualInstanceHitTest_UI.h`, `MapCanvasView_UI.h`, `PreviewComposite_UI.h`,
and the legacy `gui/widgets/Widget_AreaEditor.h`/`.cpp` (the algorithm this ports; its own architecture
is not ported, per Constitution law on the untouched v1 tree). Several real corrections to the
relayed design surfaced by that re-read, ratified in place below rather than left implicit.

#### Corrections to the approved design

1. **`recipe.areas` is a flat `std::vector<Params::MapArea>`** (`MapArea_PARAMS.h:11-18`) — NOT a
   `Group`-of-`Transform`s two-level structure like `MarkerInstanceGroup`/`PropInstanceGroup`/
   `DecalInstanceGroup`. `MapArea` carries no `layerIndex`, no `symmetryGroupIdentifier`, no
   `instanceIdentifier`, and there is no per-area lock (only the tab-wide `AreasTabState::bAreasLocked`,
   confirmed the ONLY lock field touching areas, `AreasTab_UI.h:36-37`). This is a stronger version of
   the design's own claim 3 (no `Traits` reuse) than the design stated: it is not merely that Areas
   lack a "symmetry concept" — Areas lack the entire two-level group/transform shape `InstanceDragGestureState`
   is built around (`groupIndex`/`transformIndex` pair, `InstanceDragGesture_UI.h:37-38`), and lack any
   per-item lock for `Traits::IsInstanceLayerLocked` to gate on. A single flat index into `recipe.areas`
   is the entire addressing scheme needed.
2. **`AreaDragGesture_UI.h`/`.cpp` do NOT mirror `PropDragGesture_UI.h`'s shape** (a thin, header-only,
   no-`.cpp` `Traits` struct wrapping the shared generic algorithm in `InstanceDragGesture_UI.h`) — there
   is no shared generic algorithm for Areas to wrap. `AreaDragGesture_UI.h`/`.cpp` instead hold the real,
   standalone, non-template algorithm directly (state + hit-test + begin/update/end), a genuine `.cpp`
   with real logic in it, the shape `MarkerDragGesture_UI.h`/`.cpp` had BEFORE §21.3 shrank it to a thin
   `Traits` shim. No `Traits` contract of any kind is introduced.
3. **The proposed setter needs no `Params::Geometry*`.** The world-space <-> region-local <-> preview-pixel
   chain a click/drag already needs is fully carried by `composite->WorldToPreviewPixel`/`PreviewPixelToWorld`
   (`PreviewComposite_UI.h:62,68`) and `view.ResolvePreviewPixel`/`ProjectPreviewPixelToRegionLocal`
   (`MapCanvasView_UI.h:81,100`) — neither needs `Geometry` (confirmed by direct read of both; the ONLY
   consumers of `Params::Geometry` in the existing drag substrate are `BuildWorldSymmetryOrbit` and
   `Traits::QuantizePositionToLayerGrid`, both symmetry/grid concepts Areas do not have per correction 1).
   `AreaOriginSliderRange`/`AreaExtentSliderRange`'s own use of `recipe.geometry.mapSize`
   (`AreasTab_UI.h:61-69`) is a TAB-only slider-fencing concern; the canvas gesture fences neither origin
   nor extent to the map size (v1 parity — `Widget_AreaEditor.cpp` fenced nothing but a 1.0 floor on
   Width/Length, `Widget_AreaEditor.cpp:183-184`) — a deliberate, explicit non-requirement, not an
   oversight.
4. **"Commit-on-mouse-release" in the ported v1 feature list refers only to the expensive recomposite
   gate, not to withholding the `MapArea` field writes.** Direct read of `Widget_AreaEditor.cpp:125-217`
   shows the drag loop writes `activeArea->X/Y/Width/Length` directly into `params.Areas` EVERY FRAME
   (line 205-208); only `bNeedsPreviewRender` (the expensive terrain recomposite trigger) is deferred to
   `isMouseReleased` (line 219-225). `AreasTab_UI.h`'s own SCOPE NOTE 1 confirms an Area feeds no
   generation stage — `PreviewDriver` derives a recomposite-only signal from it, never a regeneration —
   so this section's gesture writes `recipe.areas[areaIndex]` LIVE, every `ContinueAreaDrag` frame,
   exactly like `UpdateInstanceDragGesture` already does for Markers/Props/Decals. `MapCanvas` injects
   no `PreviewDriver` pointer at all today and gains none here — the live visual feedback comes for free
   from the new draw pass (§21.8 below) re-reading the SAME live `recipe.areas` every frame, the identical
   "no stopgap draw needed" reasoning `MapCanvas_UI.h:264-268` already gives for Props/Decals. `MakeNamesUnique`
   is the one write this section's own gesture must perform itself rather than rely on `DrawAreasTab`'s
   end-of-frame call to catch — see ruling 6 below.
5. **Areas never join `OverlayInstanceKeySet_UI`/§21.1's multi-select machinery.** `PlacementCollectionKind_UI`
   (`MapCanvas_IconLayer_UI.h:19`) enumerates `{Markers, Props, Units, Decals}` only; Areas selection stays
   exactly what `AreasTabState::selectedAreaIndex` already is — a single scalar, no Ctrl/Shift batch
   operations, no marquee-selects-multiple-areas. A canvas drag while the Areas panel is active NEVER
   falls through to §21.2's ordinary marquee resolution (ruling 2 below) — it is unconditionally either
   an area gesture or a create-by-drag, replacing marquee-select for that panel entirely, never
   supplementing it.

#### Ruling

**Scope gate.** The whole area-authoring canvas surface — handle-hit, body-hit/select, create-by-drag,
and the empty-click deselect — is reachable only while `*activePanelSource == ApplicationPanel::Areas`
(the existing enum entry, `Application_Panels_UI.h:28`; the shell already wires
`canvas.SetActivePanelSource(&tabState.activePanel)`, `Application_UI.cpp:133`) AND `AreasTabState::bAreasLocked`
is false. Locked refuses the ENTIRE surface uniformly — no handle/body hit-test, no create, no
select/deselect-by-click — the same "null-safe/false-safe refuses, never defaults to permitting"
posture `TryBeginManualInstanceDrag`'s own `requiredPanel` gate already uses
(`MapCanvas_ManualDragDispatch_UI.cpp:80-88`). This single gate expression is written in exactly one
place — a new private `bool MapCanvas::AreaGestureEligible() const` — and reused by both the press-time
begin attempt and the release-time create/deselect branch below, never two independently-maintained
copies of the same condition.

**New standalone substrate — `AreaDragGesture_UI.h`/`.cpp` (new files).** No `Traits`, no template:

```cpp
enum class AreaHandle_UI : int { None, N, NE, E, SE, S, SW, W, NW, Center };

struct AreaDragGestureState {
    bool            bActive        = false;
    int             areaIndex       = -1;     // index into recipe.areas
    AreaHandle_UI   handle          = AreaHandle_UI::None;
    Params::MapArea dragStartRect;            // full snapshot at gesture-start — every frame's delta
                                               // is computed against this, mirroring Widget_AreaEditor.cpp's
                                               // own dragStartArea snapshot verbatim
    float           dragStartWorldX = 0.0f;
    float           dragStartWorldZ = 0.0f;
    float           aspectLockRatio = 1.0f;   // startWidth / startLength, frozen at gesture-start
};

// Screen-space, tie-break N/NE/E/SE/S/SW/W/NW/Center in that fixed order (mirrors
// Widget_AreaEditor.cpp:88-96's own if/else-if priority) — each handle's world position projected via
// composite.WorldToPreviewPixel + view.ProjectPreviewPixelToRegionLocal, compared against
// regionLocalX/Y within kAreaHandleScreenRadiusPixels. None if no handle is within radius.
AreaHandle_UI HitTestAreaHandles(const Params::MapArea& area, const PreviewComposite& composite,
                                 const MapCanvasView& view, float regionLocalX, float regionLocalY);

// World-space exact rectangle containment (mathematically equivalent to a screen-space test for this
// composite's affine mapping — cheaper, no per-corner projection needed).
bool IsWorldPointInsideArea(const Params::MapArea& area, float worldX, float worldZ);

bool BeginAreaDragGesture(AreaDragGestureState& state, const std::vector<Params::MapArea>& areas,
                          int areaIndex, AreaHandle_UI handle, float worldX, float worldZ);
// Ports Widget_AreaEditor.cpp:145-209's delta/aspect-lock/center-resize math verbatim onto
// originX/originZ/width/length. Center handle: pure translate. Any of the other 8: Ctrl doubles the
// extent delta and resizes from the rect's own center (Widget_AreaEditor.cpp:161-164,192-194); Shift
// locks the opposite axis to aspectLockRatio, using the larger-magnitude delta to decide which axis
// leads on a corner handle (Widget_AreaEditor.cpp:169-181). Each axis floors to kAreaMinimumExtentWorldUnits.
void UpdateAreaDragGesture(AreaDragGestureState& state, std::vector<Params::MapArea>& areas,
                           float worldX, float worldZ, bool bShiftHeld, bool bCtrlHeld);
// Areas have no materialize/cascade-delete step (correction 1) — this only clears state; every field
// write already landed live during Update.
void EndAreaDragGesture(AreaDragGestureState& state);

inline constexpr float kAreaHandleScreenRadiusPixels = 8.0f;   // ported verbatim, Widget_AreaEditor.cpp:46
inline constexpr float kAreaMinimumExtentWorldUnits  = 1.0f;   // ported verbatim, Widget_AreaEditor.cpp:183-184
```

**Injected-pointer setter — `MapCanvas_UI.h`, mirrors `SetManualPropDragSource`'s shape minus `Geometry`
(correction 3).** A new bundle struct in the already-established companion header
`MapCanvas_ManualDragSources_UI.h` (the SAME file `ManualPropDragSources_UI`/`ManualDecalDragSources_UI`
already live in, §21.7's own split):

```cpp
struct ManualAreaDragSources_UI {
    std::vector<Params::MapArea>* areas             = nullptr;   // mutable: the canvas creates/moves/resizes
    std::vector<AreaColorEntry>*  areaColors         = nullptr;   // mutable: ResolveAreaColor lazily
                                                                    // appends a default entry for a
                                                                    // freshly canvas-created area
    const bool*                   bAreasLocked       = nullptr;   // read-only: canvas never writes the lock
    int*                          selectedAreaIndex  = nullptr;   // mutable: auto-select-on-touch/deselect
    AreaDragGestureState           state;
};
```
```cpp
void SetManualAreaDragSource(std::vector<Params::MapArea>* areas, std::vector<AreaColorEntry>* areaColors,
                              const bool* areasLocked, int* selectedAreaIndex) {
    manualAreaDrag.areas = areas; manualAreaDrag.areaColors = areaColors;
    manualAreaDrag.bAreasLocked = areasLocked; manualAreaDrag.selectedAreaIndex = selectedAreaIndex;
}
```
`selectedAreaIndex` is deliberately MUTABLE (unlike `SetManualMarkerSelectionSource`'s `const int*`) —
Markers' selection mutates through §21.1's `OverlayInstanceKeySet_UI`/`ApplySelectionGesture` and mirrors
OUT into `MarkersTabState` via `Application::WireCallbacks()`; Areas have no such multi-select plumbing
(ruling above) and no callback round-trip to build one for a single scalar, so the canvas writes
`AreasTabState::selectedAreaIndex` directly — same address, one source of truth, just the opposite
mutation direction from the Marker precedent, not a "second copy." Wired in `Application_UI.cpp`
alongside the other `canvas.Set*Source` calls:
```cpp
canvas.SetManualAreaDragSource(&recipe.areas, &tabState.areas.areaColors,
                               &tabState.areas.bAreasLocked, &tabState.areas.selectedAreaIndex);
```

**Dispatch — new `MapCanvas_AreaDragDispatch_UI.cpp` (new file), mirroring
`MapCanvas_ManualDragDispatch_UI.cpp`'s role one tier over, as its own independent sibling, NOT folded
into `TryBeginManualInstanceDrag`'s 3-way switch (Areas is not a `PlacementCollectionKind_UI`, correction 5):**

```cpp
bool MapCanvas::AreaGestureEligible() const;                              // the one gate expression
bool MapCanvas::TryBeginAreaDrag(float regionLocalX, float regionLocalY); // press-time
void MapCanvas::ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld);
void MapCanvas::EndAreaDrag();
void MapCanvas::CreateAreaFromDrag(float pressRegionLocalX, float pressRegionLocalY,
                                   float releaseRegionLocalX, float releaseRegionLocalY);   // release-time only
```
`ContinueAreaDrag` takes `bShiftHeld`/`bCtrlHeld` as explicit parameters (read from `io.KeyShift`/`io.KeyCtrl`
at the ONE call site in `MapCanvas_Draw_UI.cpp`) rather than querying `ImGui::GetIO()` itself — the same
"dispatch/gesture files stay imgui-free, only `MapCanvas_Draw_UI.cpp` reads live pointer/modifier state"
discipline `ApplyClickGesture(regionLocalX, regionLocalY, bCtrlHeld, bShiftHeld)` already establishes.

`TryBeginAreaDrag`: refuses if `!AreaGestureEligible()`. Else, if a selection exists
(`*selectedAreaIndex` valid), hit-tests that ONE area's 8 handles first (`HitTestAreaHandles`) — a hit
begins a resize gesture (`BeginAreaDragGesture` with that handle). A miss on handles falls through to a
body hit-test over EVERY area (forward iteration, last match wins — the SAME "later-in-vector-drawn-later-
wins" precedent `Widget_AreaEditor.cpp`'s own unconditional-overwrite loop already establishes, since the
last vector entry is drawn topmost per its own "reverse Z-order" comment, `Widget_AreaEditor.cpp:50`). A
body hit on a DIFFERENT area than the current selection immediately reassigns `*selectedAreaIndex` (a
documented widening of the approved design's claim 2, which described handles/move as reachable only for
the already-selected area — see "correction beyond claim 2" below) and begins a Center-handle move
gesture on it; a body hit on the ALREADY-selected area begins the move gesture directly. A miss on both
returns false WITHOUT recording any extra state — `pressStartRegionLocalX`/`pressStartRegionLocalY` (the
existing §21.2 fields) already carry everything `CreateAreaFromDrag` needs at release.

Handles win priority over body on purpose: a handle circle sits ON the rectangle's own border, so a
body-hit test that ran first would make every handle permanently unreachable.

**Wiring into `ApplyPointerInput` (`MapCanvas_Draw_UI.cpp`).** Press (`IsItemActivated()`): AFTER the
existing `TryBeginManualInstanceDrag(...)` call, add `if (!<that call's own return value>) TryBeginAreaDrag(regionLocalX, regionLocalY);`
— the existing call's return value must now be captured (today it is not) to know whether to attempt the
Area path; because `TryBeginManualInstanceDrag` already refuses unconditionally whenever the active panel
is not Markers/Props/Decals, and `TryBeginAreaDrag` already refuses unconditionally whenever the active
panel is not Areas, the two are mutually exclusive by construction regardless of call order — this
ordering is chosen only to keep the diff additive (append, don't reorder). Continue
(`bPressActive && IsItemActive()`): extend the existing `if (bManualDragActive) ContinueManualInstanceDrag(...)`
with `else if (bAreaDragActive) ContinueAreaDrag(regionLocalX, regionLocalY, io.KeyShift, io.KeyCtrl);`.
Release (`bPressActive && IsItemDeactivated()`): insert a new branch BEFORE the existing
`bClick ? ApplyClickGesture(...) : ApplyMarqueeGesture(...)` fallback (ruling 5 — Areas must pre-empt
ordinary click/marquee entirely while its own panel is active, never let them fight):
```cpp
if (bManualDragActive) { EndManualInstanceDrag(); if (bClick) ApplyClickGesture(...); }
else if (bAreaDragActive) { EndAreaDrag(); }
else if (AreaGestureEligible()) {
    if (bClick) { if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = -1; }
    else CreateAreaFromDrag(pressStartRegionLocalX, pressStartRegionLocalY, regionLocalX, regionLocalY);
} else if (bClick) ApplyClickGesture(...);
else ApplyMarqueeGesture(...);
```
A new `bAreaDragActive` flag (mirrors `bManualPropDragActive`'s own declaration shape) joins the existing
`bManualDragActive` OR-chain wherever it currently reads
`bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive` is NOT extended to include
it — Areas is deliberately its own independent flag, not a fourth member of that domain's OR-chain, since
`DrawMarqueeRectanglePass`'s own suppression guard (below) needs to name it separately alongside, not
folded into, the manual-instance one.

**Draw pass — new `MapCanvas_AreaDraw_UI.cpp` (new file), `void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY)`, called from `Draw()` as a new sibling
line beside `DrawOverlayIconLayerPass`/`DrawMarqueeRectanglePass`.** Every area in `*manualAreaDrag.areas`
draws a filled (via `ResolveAreaColor`, lazily appending a default entry for a brand-new area) + bordered
rect, EVERY FRAME, regardless of drag state — a deliberate v2 simplification over v1 (which only filled
during an active drag, bordered-only otherwise, `Widget_AreaEditor.cpp:56` vs `211-215`); the live-drag
"immediate feedback" v1 needed a special-cased fill for comes for free here since the always-on fill
already re-reads the same live, being-dragged rect every frame. **(This paragraph's fill/border clause is
AMENDED — see the 2026-08-29 amendment at the end of this file; the fill is now the composite's job in
the steady state and the border is edit-time-only. The handle and cursor-shape rulings below are
unchanged.)** The 8 handles draw only for
`*manualAreaDrag.selectedAreaIndex`'s own area (mirrors the approved design's claim 8 and STEP110's
"only the selected/expanded row's extra UI ever draws" convention). Cursor-shape feedback
(`ImGui::SetMouseCursor`, N/S->ResizeNS, E/W->ResizeEW, NE/SW->ResizeNESW, NW/SE->ResizeNWSE,
Center->ResizeAll, ported verbatim from `Widget_AreaEditor.cpp:101-107`) is computed HERE, from a fresh,
hover-only re-run of `HitTestAreaHandles`/body containment against the CURRENT cursor position — cosmetic
only, deliberately not threaded through `AreaDragGestureState`, so a hover nicety never couples to the
authoritative gesture state. `DrawMarqueeRectanglePass`'s own suppression guard
(`MapCanvas_Draw_UI.cpp:118`, `bManualDragActive = bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive`)
gains `|| bAreaDragActive` — otherwise the generic rubber-band box would draw simultaneously with a live
resize/move. Note what this buys for free: with that one guard extended, `DrawMarqueeRectanglePass`
ALREADY draws the create-by-drag preview rectangle with zero new code — its own header comment already
states it is "collection-agnostic by construction — keyed only on the press state" (`MapCanvas_Draw_UI.cpp:112-114`),
and a create-attempt is exactly `bPressActive` with neither a manual-instance nor an area drag active, the
precise condition that pass already draws under.

**Correction beyond claim 2 — body-hit auto-selects a DIFFERENT area, not only the already-selected
one.** The approved design's claim 2 described handles/move as reachable only for the tab's currently-
selected area. Read literally, that would leave no canvas-only way to select a different area at all (the
Area Stack list would be the only selector) — a worse canvas experience than Markers/Props/Decals, where
any hit selects/drags regardless of prior selection. Ruled: a body hit on a non-selected area reassigns
`*selectedAreaIndex` to it immediately (live, at press-time, the same "select on touch" responsiveness a
mousedown gives in most rectangle editors) and begins its move gesture in the same press — handles stay
selected-only (unchanged from claim 2/8), only body-hit-to-select is widened.

**Create-by-drag (`CreateAreaFromDrag`).** Resolves `pressStartRegionLocalX/Y` and the release-time
region-local point to world space via the SAME two-call chain (`view.ResolvePreviewPixel` then
`composite->PreviewPixelToWorld`) `ContinueManualInstanceDrag`/`ResolveMarqueeWorldRect` already each
inline independently (`MapCanvas_ManualDragDispatch_UI.cpp:129-131`, `MapCanvas_SelectionGesture_UI.cpp:92-97`)
— a third inline instance of this two-line glue matches, rather than breaks, that existing precedent; this
section does NOT mandate extracting a shared helper the rest of the codebase does not bother with either.
Builds one `Params::MapArea`: `originX/originZ` = the world rect's min corner, `width`/`length` = the
rect's span per axis, each floored to `kAreaMinimumExtentWorldUnits` (never the tab's 100x100 default —
claim 1's own instruction). Named via `NextAreaName(static_cast<int>(areas.size()))` (`AreasTab_List_UI.h:62`,
the SAME helper the "Add New Area" button uses, `AreasTab_UI.cpp:112`), pushed, THEN
`MakeNamesUnique(*manualAreaDrag.areas)` is called by this function itself — NOT left for
`DrawAreasTab`'s own end-of-frame call to catch (`AreasTab_UI.cpp:142`), because that call's own
`bAreasMoved` is a local computed purely from the TAB's own widget draws that same frame and has no
visibility into a canvas-driven `push_back` (a real latent duplicate-name bug the approved design did not
consider — flagged and closed here). Finally `*manualAreaDrag.selectedAreaIndex = static_cast<int>(areas.size()) - 1`
— auto-selects the new area, mirroring the "Add New Area" button's own identical closing line
(`AreasTab_UI.cpp:119`).

#### Explicit rulings on the plan's open questions

- **Handle hit-test tolerance: screen pixels, not world units.** `kAreaHandleScreenRadiusPixels = 8.0f`,
  a named `constexpr` (Constitution §8), ported verbatim from v1's own hardcoded screen-space
  `cornerRadius` (`Widget_AreaEditor.cpp:46`) — not a new `ApplicationSettings` field, since neither v1
  nor the approved design asked for one to be user-configurable.
- **Travel below click-tolerance must not create a zero-size area.** Reuses the existing
  `view.settings.clickDragTolerancePixels` (`MapCanvasView_UI.h:38`), the SAME constant §21.2's own
  click/drag/marquee disambiguation already uses — no second tolerance constant. Below tolerance: the
  empty-space release is a click, which deselects (below), never a create.
- **A brand-new area auto-selects.** Yes — mirrors the "Add New Area" button's own existing behavior
  verbatim (`AreasTab_UI.cpp:119`).
- **A brand-new area needs a generated unique name.** Yes — `NextAreaName` at creation, `MakeNamesUnique`
  called by the canvas gesture itself immediately after (not deferred to the tab's own frame-end call —
  see "Create-by-drag" above for why deferring would miss the tab's own change-detection).
- **Empty-space click (Areas panel active, nothing hit, no real drag).** Deselects
  (`*selectedAreaIndex = -1`) — mirrors `ApplyClickGesture`'s own existing "a miss clears the (Marker)
  selection" convention (`MapCanvas_SelectionGesture_UI.cpp:55-61`'s `missKey`), applied to the Area
  domain's own single-scalar selection.
- **Locked gates the whole surface, uniformly, including selection-by-click.** Not merely "drag or
  resize" per the tab's own tooltip text (`AreasTab_UI.cpp:107-108`) — `AreaGestureEligible()` is one
  all-or-nothing gate, matching every other injected-pointer gate's own binary posture in this file
  (`activePanelSource`, `scenarioEditModeState`). Selecting a different area while locked is still
  possible through the Area Stack list (`DrawAreaList`'s own `Select` signal, `AreasTab_UI.cpp:89-91`),
  unaffected — no real capability is lost, only the canvas-side shortcut.

#### File-size ceiling note

`MapCanvas_UI.h` was already flagged over ceiling by §21.7 (261 lines at that ruling; 394 lines today,
confirmed by direct read, already past that flag with `SetManualPropDragSource`/`SetManualDecalDragSource`
and §21.1/§21.2's own fields landed). This section adds one more setter of the same one-line shape plus
one more bundle field (`manualAreaDrag`, mirroring `manualPropDrag`/`manualDecalDrag`) and one more bool
flag (`bAreaDragActive`) — the same small, already-precedented growth shape §21.7 already accepted for
its Props/Decals siblings, not a new class of growth. §21.7's own ruling stands: measure at build time,
split a companion header if actually over the hard ceiling, no new shape mandated here.

#### New files this section ratifies

- `src/ui/AreaDragGesture_UI.h` / `.cpp` — the standalone state + hit-test + begin/update/end algorithm.
- `src/ui/MapCanvas_AreaDragDispatch_UI.cpp` — `MapCanvas::AreaGestureEligible`/`TryBeginAreaDrag`/
  `ContinueAreaDrag`/`EndAreaDrag`/`CreateAreaFromDrag`.
- `src/ui/MapCanvas_AreaDraw_UI.cpp` — `MapCanvas::DrawAreaOverlayPass` (fill+border every area, handles
  for the selected one, cursor-shape feedback).

Modified (by the coder, not by this expert): `MapCanvas_UI.h` (new setter, new field, new private method
declarations, new flag), `MapCanvas_ManualDragSources_UI.h` (new `ManualAreaDragSources_UI` struct),
`MapCanvas_Draw_UI.cpp` (`ApplyPointerInput` wiring, `DrawMarqueeRectanglePass`'s guard, the new draw-pass
call site), `Application_UI.cpp` (`SetManualAreaDragSource` wiring). None of `AreasTab_UI.h`/`.cpp`/
`AreasTab_List_UI.h`/`MapArea_PARAMS.h` need to change — this section is additive over their existing,
already-ratified shape.

---

#### AMENDED 2026-08-29 — the draw pass is re-scoped: the composite owns the steady-state fill (see [§14.17](ARCH_14_17_MapAreaFieldLayer.md))

Everything above shipped as ratified and is confirmed live by direct read (`AreaDragGesture_UI.h`/`.cpp`,
`MapCanvas_AreaDragDispatch_UI.cpp`, `MapCanvas_AreaDraw_UI.cpp`, `MapCanvas_ManualDragSources_UI.h:41-49`,
`Application_UI.cpp:146-147`). Its gesture, dispatch, create-by-drag, scope-gate and open-question rulings
are **unchanged**. What changes is the **draw-pass** ruling, because Map Areas have since been folded into
the real GPU-composited preview blend pipeline as a `PreviewFieldLayer` of the new kind
`PreviewLayerKind::MapAreas` (human-approved; full data shape, bindings, defaults and reasoning in
[§14.17](ARCH_14_17_MapAreaFieldLayer.md)). Four clauses are superseded, in place:

1. **"Fill + bordered rect, EVERY FRAME, regardless of drag state" is retired.** The area FILL is now the
   composite's job in the steady state — `MapAreas` is a real composited field layer, blended per-pixel
   from `PreviewCompositeSettings::areaColors` and `recipe.areas` flattened to cell space at `PrepareRun()`
   (§14.17 items 3-7). This section's own rationale for the always-on immediate-mode fill ("the live-drag
   immediate feedback comes for free since the always-on fill already re-reads the same live rect every
   frame") no longer applies to the composite path, because a composited fill of a rectangle being dragged
   would cost a GPU recomposite per drag frame — a Tier B cost (`ARCH_14_08_DirtyFlagTiers.md`) this
   section deliberately avoided and which correction 4 above exists to keep avoiding.

2. **Drag performance — exactly two recomposites per gesture, via a suppressed index.**
   `PreviewCompositeSettings` gains a transient `int mapAreaSuppressedIndex = -1` (presentation state,
   never serialized). `TryBeginAreaDrag` sets it to the dragged area's index and requests ONE recomposite;
   every `ContinueAreaDrag` frame writes `recipe.areas` live exactly as correction 4 above already rules
   and requests **zero** recomposites, with the composite's `BuildMapAreaConfigurations()` omitting the
   suppressed index's rectangle; `EndAreaDrag` resets it to `-1` and requests one more.
   `CreateAreaFromDrag` requests one (a brand-new area must appear). **Net: two recomposites per
   drag/resize gesture, not one per frame** — correction 4's "commit-on-mouse-release is about the
   expensive recomposite gate, not the field writes" reading is thereby restored in its exact original v1
   sense, now that a real expensive recomposite exists again. The suppression is an **index**, deliberately
   not a flip of `fieldLayers[i].bEnabled`, so transient interaction state can never clobber the user's
   own View-popup / left-column enable toggle. Plumbing — a fifth `int* mapAreaSuppressedIndex` parameter
   on `SetManualAreaDragSource` (and the matching field on `ManualAreaDragSources_UI`), plus a
   `SetAreaCompositeRefreshCallback(std::function<void()>)` mirroring `SetSelectionChangedCallback`'s
   existing injection shape and bound in `Application::WireCallbacks()` to
   `previewDriver.NotifyParametersChanged()` — is specified in §14.17 item 11. `MapCanvas`'s
   `const PreviewComposite* composite` stays **const**: this section's "the canvas never composites"
   posture is preserved, which is exactly why the suppressed index arrives as its own injected pointer
   rather than through the composite.

3. **`DrawAreaOverlayPass`'s amended contract.** Per frame it draws the **fill** only for the ONE area
   currently being manipulated (the `mapAreaSuppressedIndex`) — the immediate-mode stand-in for the
   composite fill suppressed this frame; drawing every area's fill would double-paint on top of the
   composite's own fill for every non-dragged area. It draws that area's **border** only when all three
   hold: (a) the `MapAreas` field layer is enabled, (b) that area is the suppressed one, and (c) it is the
   selected area. **If the MapAreas layer is disabled entirely, the border never draws, regardless of
   selection** — a disabled layer means "do not show me areas," and a border is showing an area. The 8
   handles (selected-area-only) and the hover-only cursor-shape feedback keep their rulings above
   verbatim, as does `DrawMarqueeRectanglePass`'s `|| bAreaDragActive` guard. §14.17 item 12 is the
   authoritative statement of the border rule.

4. **`AreasTab_List_UI.h` DOES now change**, contrary to this section's closing line — not for any gesture
   reason, but because `AreaColorEntry`/`ResolveAreaColor` move to a new minimal
   `src/ui/AreaColorTable_UI.h`, and the color table's single owner becomes
   `PreviewCompositeSettings::areaColors` rather than `AreasTabState::areaColors` (§14.17 item 9).
   `MapCanvas_ManualDragSources_UI.h:11`'s `#include "AreasTab_List_UI.h" // AreaColorEntry` retargets to
   the new header; `ManualAreaDragSources_UI::areaColors` keeps its exact declared type and every
   `ResolveAreaColor` call site — including `MapCanvas_AreaDraw_UI.cpp`'s — keeps its exact existing form.
   `MapArea_PARAMS.h` and the `.sanmap` schema remain untouched (§14.17 item 13), as this section
   originally stated.
