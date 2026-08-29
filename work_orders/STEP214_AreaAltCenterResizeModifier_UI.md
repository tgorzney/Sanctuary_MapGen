# STEP214 — Area resize center-modifier: Alt as an additional trigger for the existing Ctrl center-resize behavior

**Layer:** UI. **Domain:** `MapCanvas`'s Area drag gesture (`ContinueAreaDrag`, `AreaDragGesture_UI.h`/`.cpp`). **Executor:** SanGen Coder. Authored by the SanGen UI Expert. Widens the trigger condition for an already-ratified behavior — `UpdateAreaDragGesture`'s `bCtrlHeld`-driven center-resize math, ARCH §21.8 (`ARCH_21_08_AreaCanvasGesture.md`) — it invents no new law and needs no ARCH ruling. Every file this ticket cites was read directly against the live tree while drafting it, post-STEP210/211/212/213 (`AreaDragGesture_UI.h`/`.cpp` and `MapCanvas_AreaDragDispatch_UI.cpp` were confirmed unmodified by STEP211/212/213's field-layer/lock/suppression work — this ticket has zero file overlap with those tickets).

## Summary
The human asked for an ALT modifier on the Area resize gesture doing "delta from center" — and that
behavior **already exists today, on Ctrl.** `AreaDragGesture_UI.cpp`'s `UpdateAreaDragGesture`
(shipped STEP210, untouched since) doubles the extent delta and recenters the new rect on the
original rectangle's own center (`centerX`/`centerZ`, computed from `dragStartRect`) whenever
`bCtrlHeld` is true — for every one of the 8 handles, edges and corners alike. This was re-derived
line-by-line against `AreaDragGesture_UI.cpp:100,120-136` and confirmed to correctly implement
symmetric growth/shrink about the original centroid in every case (edge handles: the doubled delta
plus a recenter on both axes is exactly equivalent to moving both edges by the same magnitude in
opposite directions; corner handles: both axes double and recenter independently, same result). No
bug was found in Ctrl's own math — nothing here is silently "fixed" alongside the Alt addition.

The human said **ADD**, not replace, so this ticket makes Alt an equally-valid alternate trigger for
the *identical* code path, OR'd with Ctrl — not a second, different modifier semantic, and not a
retirement of Ctrl. The merge happens at the one and only place in the whole canvas that reads
imgui's `io` state at all: `MapCanvas_Draw_UI.cpp` (its own header comment: "This is the ONLY
translation unit of the canvas that includes imgui... so what a click means is defined in exactly
one place" — this ticket extends that same discipline to "what a center-resize modifier means").
`UpdateAreaDragGesture`'s and `ContinueAreaDrag`'s own signatures, parameter names, and math are all
**completely unchanged** — they still take one opaque `bCtrlHeld` bool; only the value fed into it
from `io.KeyCtrl` alone widens to `io.KeyCtrl || io.KeyAlt`.

**Practical check on reserving the keys separately (the human's own question 2):** `Ctrl` is indeed
heavily overloaded elsewhere in this app — `MapCanvas_SelectionGesture_UI.cpp`'s `ApplyClickGesture`/
`ApplySelectionGesture`/`ApplyMarqueeGesture` all read `io.KeyCtrl` for multi-select toggle (and
`io.KeyShift` for range-add), and `MapCanvas_ManualDragDispatch_UI.cpp` reads no modifier keys at
all (verified via grep — zero `KeyCtrl`/`KeyShift`/`KeyAlt` hits in that file). But none of this
collides with Areas: `MapCanvas_Draw_UI.cpp`'s own release handler is a strict if/else-if chain where
the `bAreaDragActive`/`AreaGestureEligible()` branches are mutually exclusive with the branches that
call `ApplyClickGesture`/`ApplyMarqueeGesture` — while an Area gesture (or the Areas panel) is live,
Ctrl's multi-select meaning is simply never reached. There is no live use of `io.KeyAlt` anywhere in
`src/ui` today (grepped, zero hits). Conclusion: no reason to reserve Ctrl and Alt for different
things — both can mean the same thing for this one gesture with zero practical conflict.

## Required reading
`ARCH_21_08_AreaCanvasGesture.md` lines 111-117 (the ratified `UpdateAreaDragGesture` contract:
"Ctrl doubles the extent delta and resizes from the rect's own center") and line 202 (the ratified
literal call site, `ContinueAreaDrag(regionLocalX, regionLocalY, io.KeyShift, io.KeyCtrl)`) — this
ticket widens the second argument's *value* at that exact call site; it does not change, and is not
required to change, the ratified text itself (see Explicit out-of-scope).

---

## 1. Modified: `src/ui/MapCanvas_Draw_UI.cpp`

The one line in this entire ticket that changes runtime behavior. Currently (`MapCanvas_Draw_UI.cpp:178-187`):
```cpp
    const bool bManualDragActive = bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive;
    if (bPressActive && ImGui::IsItemActive()) {
        // Human's own bug report (predates §21.2, still applies) — this must accumulate regardless
        // of which branch below runs, or a gesture's own pressTravelPixels would stay frozen at its
        // activation-time 0.0f for the gesture's ENTIRE duration, and the release check below would
        // then treat every drag, however large, as a zero-travel click.
        pressTravelPixels += std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y);
        if (bManualDragActive) ContinueManualInstanceDrag(regionLocalX, regionLocalY);
        else if (bAreaDragActive) ContinueAreaDrag(regionLocalX, regionLocalY, io.KeyShift, io.KeyCtrl);
    }
```
Change the last line only:
```cpp
    const bool bManualDragActive = bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive;
    if (bPressActive && ImGui::IsItemActive()) {
        // Human's own bug report (predates §21.2, still applies) — this must accumulate regardless
        // of which branch below runs, or a gesture's own pressTravelPixels would stay frozen at its
        // activation-time 0.0f for the gesture's ENTIRE duration, and the release check below would
        // then treat every drag, however large, as a zero-travel click.
        pressTravelPixels += std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y);
        if (bManualDragActive) ContinueManualInstanceDrag(regionLocalX, regionLocalY);
        // STEP214 — Alt is an ADDITIONAL trigger for the exact same Ctrl center-resize behavior ARCH
        // §21.8 already ratified (UpdateAreaDragGesture's own `bCtrlHeld` parameter — name and math
        // both unchanged by this ticket); the human's own request was to ADD a key, not replace one.
        // This is the ONLY line in this whole ticket that changes runtime behavior — every other
        // file this ticket touches is either a doc-comment clarification or new test coverage. Ctrl
        // is also already the modifier ApplyClickGesture/ApplySelectionGesture/ApplyMarqueeGesture
        // use for multi-select elsewhere in this file (MapCanvas_SelectionGesture_UI.cpp) — verified
        // no collision: those are only ever reached from branches that are mutually exclusive with
        // bAreaDragActive/AreaGestureEligible() in this function's own release handler below, and
        // MapCanvas_ManualDragDispatch_UI.cpp reads no modifier keys at all — Areas already owns
        // Ctrl's meaning exclusively while its own gesture is live, so there is no reason to reserve
        // Ctrl and Alt for different things here; both simply mean the same thing for this gesture.
        else if (bAreaDragActive)
            ContinueAreaDrag(regionLocalX, regionLocalY, io.KeyShift, io.KeyCtrl || io.KeyAlt);
    }
```

---

## 2. Modified: `src/ui/AreaDragGesture_UI.h`

Doc-comment clarification only — `UpdateAreaDragGesture`'s signature, parameter names, and math are
untouched. Currently (`AreaDragGesture_UI.h:65-71`):
```cpp
// One drag frame. Center: pure translate. Any of the 8 resize handles: Ctrl doubles the extent
// delta and resizes from the rect's own center; Shift locks the opposite axis to aspectLockRatio,
// the larger-magnitude delta deciding which axis leads on a corner handle. Each axis floors to
// kAreaMinimumExtentWorldUnits. No-op if `state` is not active or `state.areaIndex` is out of range
// (in which case state.bActive is also cleared, defensively).
void UpdateAreaDragGesture(AreaDragGestureState& state, std::vector<Params::MapArea>& areas,
                           float worldX, float worldZ, bool bShiftHeld, bool bCtrlHeld);
```
Replace with:
```cpp
// One drag frame. Center: pure translate. Any of the 8 resize handles: `bCtrlHeld` doubles the
// extent delta and resizes from the rect's own center; Shift locks the opposite axis to
// aspectLockRatio, the larger-magnitude delta deciding which axis leads on a corner handle. Each
// axis floors to kAreaMinimumExtentWorldUnits. No-op if `state` is not active or `state.areaIndex`
// is out of range (in which case state.bActive is also cleared, defensively).
// STEP214 — `bCtrlHeld` is a boolean gesture flag, not literally "the physical Ctrl key is down":
// its one caller, MapCanvas_AreaDragDispatch_UI.cpp's ContinueAreaDrag, is itself fed by
// MapCanvas_Draw_UI.cpp's own `io.KeyCtrl || io.KeyAlt` (the ONE translation unit in this canvas
// that reads imgui's `io` at all) — Alt is an ADDITIONAL trigger for this exact same center-resize
// path, not a second, different modifier semantic. This function's own math and signature are
// completely unchanged by that widening; it still only ever sees one opaque bool, matching this
// codebase's own established `bCtrlHeld`/`bShiftHeld` literal-modifier-name convention elsewhere —
// the name is not changed to something like `bCenterResizeModifierHeld` at this layer or any layer
// below MapCanvas_Draw_UI.cpp (see STEP214's own ticket text for why).
void UpdateAreaDragGesture(AreaDragGestureState& state, std::vector<Params::MapArea>& areas,
                           float worldX, float worldZ, bool bShiftHeld, bool bCtrlHeld);
```

No other declaration in this header, and no line in `AreaDragGesture_UI.cpp`, `MapCanvas_UI.h`, or
`MapCanvas_AreaDragDispatch_UI.cpp` changes — `ContinueAreaDrag`'s own declaration (`MapCanvas_UI.h:353`,
`// ditto`) and its body (`MapCanvas_AreaDragDispatch_UI.cpp:102-112`) never claimed "Ctrl" as a fact
in their own comments, so nothing there goes stale and nothing there needs editing.

---

## 3. New test file: `src/ui/MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp`

GL-backed (mirrors `MapCanvas_AreaDragSuppression_UI_Test.cpp`'s own technique exactly), because the
one thing this ticket actually changed lives in the ONE translation unit that reads imgui's `io`
state — the only way to prove it is a real `MapCanvas::Draw()` press/drag/release cycle with a real,
`AddKeyEvent`-driven `io.KeyAlt`. Deliberately does **not** re-derive the resize math's own
closed-form expected numbers (`AreaDragGesture_UI_Test.cpp`'s `RunCtrlCenterResizeChecks` already
owns that, headlessly, against `UpdateAreaDragGesture` directly, unaffected by this ticket) — instead
it runs the *identical* press/drag/release screen-pixel sequence four times (no modifier / Ctrl-only
/ Alt-only / Ctrl+Alt) and compares the resulting `Params::MapArea` bit-for-bit across runs, which
sidesteps this file's own screen↔world projection math entirely while still proving Alt-alone reaches
the exact same code path Ctrl-alone does, and that holding both together doesn't double-apply the
doubling.

```cpp
// MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp — STEP214 acceptance: Alt is an ADDITIONAL
// trigger for the exact same center-resize behavior ARCH §21.8 already ratified for Ctrl
// (UpdateAreaDragGesture's own math in AreaDragGesture_UI.cpp — completely unmodified by this
// ticket), not a new/different modifier semantic. GL-backed (mirrors
// MapCanvas_AreaDragSuppression_UI_Test.cpp's own technique exactly) because the one thing this
// ticket actually changed — MapCanvas_Draw_UI.cpp's own `io.KeyCtrl || io.KeyAlt` merge at the
// ContinueAreaDrag call site — lives in the ONE translation unit that reads imgui's `io` state, so
// the only way to prove it is a real MapCanvas::Draw() press/drag/release cycle with a real,
// AddKeyEvent-driven `io.KeyAlt`. Deliberately does NOT re-derive the resize math's own closed-form
// expected numbers (AreaDragGesture_UI_Test.cpp's RunCtrlCenterResizeChecks already owns that,
// headlessly, against UpdateAreaDragGesture directly) — it instead runs the IDENTICAL press/drag/
// release screen-pixel sequence four times (no modifier / Ctrl-only / Alt-only / Ctrl+Alt) and
// compares the resulting Params::MapArea bit-for-bit across runs, which sidesteps this file's own
// screen<->world projection math entirely while still proving Alt-alone reaches the exact same code
// path Ctrl-alone does, and that holding both together doesn't double-apply the doubling.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution = 64;
constexpr float kRegionSidePixels  = 256.0f;
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000005ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFontAtlasIdentifier));
    ImGui::NewFrame();
}

ImVec2 DrawOneFrame(MapCanvas& canvas) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(600.0f, 600.0f));
    ImGui::Begin("AreaAltCenterResizeModifierTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

// io.KeyCtrl/io.KeyAlt are read-only, recomputed by NewFrame() from the key-event queue every frame
// (imgui.h's own comment — MapCanvas_GestureOwnership_UI_Test.cpp's own SetModifierKeys already
// established this exact caveat for Ctrl/Shift) — the real modifier state must go through
// AddKeyEvent(ImGuiMod_*, ...), exactly as a real keyboard backend would report it.
void SetModifierKeys(bool bCtrl, bool bAlt) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, bCtrl);
    io.AddKeyEvent(ImGuiMod_Alt, bAlt);
}

ImVec2 ScreenPositionFor(const ImVec2& regionOrigin, MapCanvas& canvas, const PreviewComposite& composite,
                         float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

// One full press-(E handle)-drag-(+40 screen px in X)-release cycle, with the given modifier keys
// held for the drag's own duration, against a FRESH Params::MapArea reset to the same starting
// rectangle every call — returns the resulting area so the caller can compare across runs. Does NOT
// touch `areaLocks` — the caller pre-seeds a single UNLOCKED entry for the shared area name once,
// up front, since every call here reuses that same name.
Params::MapArea RunEHandleResizeGesture(MapCanvas& canvas, const ImVec2& regionOrigin,
                                        const PreviewComposite& composite,
                                        std::vector<Params::MapArea>& areas,
                                        int& selectedAreaIndex, bool bCtrl, bool bAlt) {
    areas.clear();
    Params::MapArea area;
    area.name = "Resizable"; area.originX = 0.0f; area.originZ = 0.0f; area.width = 2.0f; area.length = 2.0f;
    areas.push_back(area);
    selectedAreaIndex = 0;

    // The E handle sits at (maxX, midZ) = (2, 1) for this rect — pressing exactly there guarantees
    // TryBeginAreaDrag's handle hit-test (kAreaHandleScreenRadiusPixels==8, distance 0 here) resolves
    // AreaHandle_UI::E, never Center.
    const ImVec2 handlePosition = ScreenPositionFor(regionOrigin, canvas, composite, 2.0f, 1.0f);
    const ImVec2 releasePosition(handlePosition.x + 40.0f, handlePosition.y);

    ImGuiIO& io = ImGui::GetIO();
    SetModifierKeys(bCtrl, bAlt);
    io.AddMousePosEvent(handlePosition.x, handlePosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(releasePosition.x, releasePosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    SetModifierKeys(false, false);

    return areas[0];
}

bool SameRect(const Params::MapArea& a, const Params::MapArea& b) {
    return a.originX == b.originX && a.originZ == b.originZ && a.width == b.width && a.length == b.length;
}

} // namespace

void RunMapCanvasAreaAltCenterResizeModifierChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    std::vector<Params::MapArea> areas;
    std::vector<AreaColorEntry>  areaColors;
    std::vector<AreaLockEntry>   areaLocks;
    // STEP212's per-area lock table defaults a first-touch name to LOCKED — pre-seed "Resizable"
    // UNLOCKED once, up front, exactly mirroring MapCanvas_AreaDragSuppression_UI_Test.cpp's own
    // established precedent for its own "Existing" area.
    ResolveAreaLocked(areaLocks, "Resizable", /*bDefaultLocked=*/false);
    int selectedAreaIndex = -1;
    int mapAreaSuppressedIndex = -1;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;
    io.ConfigInputTrickleEventQueue = false;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    ApplicationPanel activePanel = ApplicationPanel::Areas;
    canvas.SetActivePanelSource(&activePanel);
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex,
                                   &mapAreaSuppressedIndex);

    // Frame 0 — priming, mouse away: establishes the region origin this window layout produces
    // (mirrors every sibling GL-backed MapCanvas test's own frame-0 priming convention).
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);

    const Params::MapArea noModifierResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/false, /*bAlt=*/false);
    const Params::MapArea ctrlOnlyResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/true, /*bAlt=*/false);
    const Params::MapArea altOnlyResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/false, /*bAlt=*/true);
    const Params::MapArea bothHeldResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/true, /*bAlt=*/true);

    check(!SameRect(noModifierResult, ctrlOnlyResult),
          "sanity: Ctrl actually changes the E-handle resize result versus no modifier at all "
          "(otherwise the comparisons below would pass vacuously)");
    check(SameRect(altOnlyResult, ctrlOnlyResult),
          "STEP214 - Alt alone reaches the EXACT SAME center-resize result Ctrl alone already "
          "produces (ARCH Sec21.8's ratified UpdateAreaDragGesture math, unmodified by this ticket) - "
          "Alt is an additional trigger for the identical behavior, not a different one");
    check(SameRect(bothHeldResult, ctrlOnlyResult),
          "STEP214 - holding Ctrl AND Alt together produces the SAME result as either alone (a "
          "single OR'd boolean reaches UpdateAreaDragGesture - holding both never doubles the "
          "doubling)");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 4. Modified: `src/ui/MapCanvas_UI_Test.cpp`

Add the forward declaration directly after the existing `RunMapCanvasAreaDragSuppressionChecks`
declaration (currently line 32):
```cpp
// ARCH §14.17 item 11 — MapCanvas_AreaDragSuppression_UI_Test.cpp.
void RunMapCanvasAreaDragSuppressionChecks(Sys::GpuResourceManager& manager);
// STEP214 — MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp.
void RunMapCanvasAreaAltCenterResizeModifierChecks(Sys::GpuResourceManager& manager);
```
Add the call directly after the existing `RunMapCanvasAreaDragSuppressionChecks` call (currently
line 57):
```cpp
    Ui::RunMapCanvasAreaDragSuppressionChecks(manager);
    Ui::RunMapCanvasAreaAltCenterResizeModifierChecks(manager);
    wglMakeCurrent(nullptr, nullptr);
```

---

## 5. Modified: `CMakeLists.txt`

Add the new source file to the existing `MapCanvas_UI_Test` target (currently `CMakeLists.txt:576-588`):
```cmake
add_sangen_test(MapCanvas_UI_Test
    src/ui/MapCanvas_UI_Test.cpp
    src/ui/MapCanvas_Render_UI_Test.cpp
    src/ui/MapCanvas_View_UI_Test.cpp
    src/ui/MapCanvas_Picking_UI_Test.cpp
    src/ui/MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp
    src/ui/MapCanvas_ActivePanelGate_UI_Test.cpp
    # ARCH §21.2/§21.5 — end-to-end pointer-state-machine coverage: right-button pans, left-button
    # never pans (drag-a-manual-instance or marquee-select instead), a marquee's lock-gate exclusion,
    # and a live Ctrl-click toggle.
    src/ui/MapCanvas_GestureOwnership_UI_Test.cpp
    # ARCH §14.17 item 11 — the Area drag-suppression / exactly-two-recomposites acceptance.
    src/ui/MapCanvas_AreaDragSuppression_UI_Test.cpp
    # STEP214 — Alt is an additional trigger for the identical Ctrl center-resize behavior.
    src/ui/MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp)
```

---

## ARCH rules invoked
- `ARCH_21_08_AreaCanvasGesture.md` (§21.8) — the entire binding law for the behavior this ticket
  widens the trigger of; `UpdateAreaDragGesture`'s doubling+recentering math and `ContinueAreaDrag`'s
  signature are untouched, per that section's own already-ratified text (lines 111-117, 168, 202).
  This ticket needs no new ARCH ruling because it changes only which physical key(s) satisfy an
  already-named boolean parameter, not the parameter's meaning or the math it drives.
- Constitution §1 — UI sets PARAMS, never simulates; this ticket writes no new sim logic, only widens
  an input-reading boolean expression in the one file that already owns "what a click/modifier means."

## Explicit out-of-scope
- **No change to Marker/Prop/Decal drag gestures, or to any other modifier-key behavior anywhere else
  in the app** — `MapCanvas_ManualDragDispatch_UI.cpp` reads no modifier keys today and is untouched;
  `MapCanvas_SelectionGesture_UI.cpp`'s own Ctrl/Shift multi-select semantics are untouched.
- **No signature or math change to `UpdateAreaDragGesture`, `ContinueAreaDrag`, or
  `SetManualAreaDragSource`** — Alt is merged into a single boolean before it ever reaches any of
  these; every one of them still takes exactly the parameters it took before this ticket.
- **No rename of `bCtrlHeld` anywhere below `MapCanvas_Draw_UI.cpp`** — see Interpretation call 2.
- **No ARCH file edit.** `ARCH_21_08_AreaCanvasGesture.md:202`'s own literal example line
  (`ContinueAreaDrag(regionLocalX, regionLocalY, io.KeyShift, io.KeyCtrl)`) becomes slightly stale
  prose once this ships (it no longer shows the full expression at that call site) — re-syncing ARCH
  prose to shipped code is the ARCH Expert's own call, not authored by this ticket or this agent.
- **No touch to any STEP211/212/213 file** — `AreaDragGesture_UI.h`/`.cpp` and
  `MapCanvas_AreaDragDispatch_UI.cpp` were read fresh against the live tree and confirmed to carry
  only STEP210's original shipped shape (plus STEP212's lock/suppression additions, which this ticket
  does not touch); zero file overlap otherwise.
- **No fix for any independently-observed Ctrl/Shift interaction quirk** — none was found. Ctrl's own
  center-resize math was re-derived and confirmed correct for every handle (edges and corners) and
  correctly composes with Shift's aspect-lock branch (the doubling is applied to the deltas before
  Shift ever reads `newWidth`/`newLength`, so Ctrl+Shift, Alt+Shift, and Ctrl+Alt+Shift all already
  behave consistently with each other) — nothing here needed silent repair.

## Acceptance test
1. Holding **only Alt** while dragging the E resize handle produces a byte-identical
   `Params::MapArea` (`originX`/`originZ`/`width`/`length`) to holding **only Ctrl** for the
   identical screen-pixel press/drag/release sequence (`MapCanvasAreaAltCenterResizeModifierChecks`).
2. Holding **both Ctrl and Alt** together produces the same result as either alone — no compounding
   of the doubling.
3. Holding **neither** still produces the pre-existing edge-fixed resize, distinct from the
   Ctrl/Alt result — proves the widened OR condition didn't accidentally force center-resize
   unconditionally.
4. `AreaDragGesture_UI_Test.cpp`'s existing `RunCtrlCenterResizeChecks` (and every other case in that
   file) continues to pass unmodified — `UpdateAreaDragGesture`'s own signature and math are untouched.
5. Full `SanGenV2` build stays clean; every existing test continues to pass; the `MapCanvas_UI_Test`
   binary passes with `ALL PASS`, including the new `RunMapCanvasAreaAltCenterResizeModifierChecks`.

## Interpretation calls made
1. **Alt is an additional OR'd trigger for the identical behavior, not a distinct semantic** — the
   human's own wording ("Need to ADD Alt modifier") and the confirmation that Ctrl's existing math
   already IS the "typical expected" center-resize convention (re-derived and verified, no gap found)
   together rule out inventing a second, different math path for Alt.
2. **The merge point is `MapCanvas_Draw_UI.cpp`'s own `ContinueAreaDrag` call site, not a widened
   `UpdateAreaDragGesture`/`ContinueAreaDrag` signature.** This file is the sole translation unit in
   the entire canvas that includes imgui and reads `io` at all (its own header comment says so) — it
   is already the established single place "what an input means" is decided (mirrors its own existing
   `bManualDragActive`/`bClick` boolean derivations). Pushing the OR down into `ContinueAreaDrag`
   would require adding `#include <imgui.h>` to a file that currently has none, or threading a new
   parameter through `MapCanvas_UI.h`'s declaration for zero behavioral benefit.
3. **`bCtrlHeld`'s name is not changed to something like `bCenterResizeModifierHeld` anywhere.** This
   codebase's own overwhelming convention (dozens of call sites across `MapCanvas_UI.cpp`,
   `MarkersTab_*`, `ListWidget_TestFrame_UI.h`, etc.) names modifier-key booleans literally after the
   physical key (`bCtrlHeld`/`bShiftHeld`), not after the semantic action they trigger. Renaming this
   one parameter to a semantic name would break that convention's consistency for a single leaf-level,
   never-serialized boolean; a doc-comment clarification in `AreaDragGesture_UI.h` was judged
   sufficient and more consistent with the rest of the codebase.
4. **No ARCH file edit**, per this agent's own absolute rule (never writes ARCH.md or any
   `ARCH_NN_*.md` section file) and because §21.8's underlying rule (Ctrl triggers center-resize) is
   not contradicted, only widened at a layer ARCH doesn't itself pin down (which physical key(s) feed
   `io.KeyCtrl`-equivalent input) — flagged in Explicit out-of-scope as a prose-staleness item for
   whichever process re-syncs ARCH text to shipped code, not authored here.
5. **The new test's verification strategy is a 4-way bit-for-bit rect comparison across identical
   screen-space gestures**, rather than closed-form expected numbers, specifically to avoid needing to
   hand-derive this test scene's own screen↔world projection math — `AreaDragGesture_UI_Test.cpp`
   already owns exhaustive closed-form coverage of `UpdateAreaDragGesture` itself, headlessly, and
   that coverage is untouched and still valid since the function's signature/math didn't change.
6. **The integration test drives only the E (single-axis) handle**, not a corner — sufficient to prove
   the `io.KeyAlt` wiring reaches the real gesture; corner-handle Ctrl/Alt behavior is exercised by the
   existing headless `UpdateAreaDragGesture` coverage, unaffected by this ticket.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_21_08_AreaCanvasGesture.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaDragGesture_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaDragGesture_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaDragGesture_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDragDispatch_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ManualDragSources_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ManualDragDispatch_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionGesture_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDragSuppression_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_GestureOwnership_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaLockTable_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_TestScene_UI.h`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt`,
and `work_orders\STEP210_AreaCanvasGesture_UI.md` (used as this document's own structure/rigor
template, per the dispatching instruction).
