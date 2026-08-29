# STEP219 — Map areas: retire drag suppression for real per-moved-frame recomposite, plus the mandatory Tier-B2 cost watchdog (ARCH §14.18 Part 3)

**Layer:** UI. **Domain:** `MapCanvas`'s Area gesture dispatch/draw, `PreviewComposite`'s self-timing, `PreviewCompositeSettings`. **Executor:** SanGen Coder. Authored by the SanGen UI Expert, per `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` Part 3 (items 17-24, ratified 2026-08-29). Every file this ticket cites was read directly against the live post-STEP216/217/218 tree while drafting it — every line number below was re-verified against that tree, not copied from the ARCH ruling's own citations, several of which described the file *before* this ticket's own edits.

## Summary
Finishes the Map Areas blend-fidelity work. `ARCH_14_18` Part 1 ruled that an area's fill has exactly one renderer in every state — the composite — and named the shipped `mapAreaSuppressedIndex`/two-recomposites-per-gesture design as the interim shape while its per-frame replacement's cost was unproven. Part 3, written after STEP218's benchmark passed, promotes that replacement to law:
- `ContinueAreaDrag` fires a real GPU recompose on every frame the dragged rectangle actually moved (snapshot-compare, the same idiom `SetAreaToMapSize` already established), and fires nothing on a held-but-motionless frame.
- `mapAreaSuppressedIndex`/`SetMapAreaSuppression` is retired **entirely** — deleted, not repurposed, everywhere it appears.
- `TryBeginAreaDrag` fires **no** refresh request (a begin changes no composite input — selection is chrome, not composited); it resets the throttle on a successful begin.
- `EndAreaDrag`'s request is unconditional and exempt from the throttle.
- `DrawAreaOverlayPass` draws **zero fill** of any kind — border and handles share one `if (selectedIndex …)` scope.
- A mandatory, always-armed watchdog (`AreaRecompositeThrottle_UI.h`) throttles the per-frame recompose to at most one per 33 ms after 5 consecutive over-budget composes, recovering immediately on the first under-budget sample, with a `bDeferredMove` latch so a throttled moving frame followed by a motionless hold still fires once the interval elapses.
- `PreviewComposite::LastComposeMillis()` brackets the **whole** `Compose()` call (both backends, including the two silent Cpu-twin fallbacks inside `ComposeOnGpu`), closing STEP218's own measured-blind-spot (`PrepareRun()` was outside that ticket's timing window).

## Required reading
`ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` in full, especially Part 3 (items 17-24) — this ticket's entire binding law. Items 1-9 (Part 1) are amended by items 17-24 and must not be cited alone; item 10's benchmark gate (closed PASS, item 17) is background, not something this ticket re-runs.

---

## 1. New file: `src/ui/AreaRecompositeThrottle_UI.h`

```cpp
// AreaRecompositeThrottle_UI.h — the Tier-B2 cost watchdog's pure decision function (ARCH
// §14.18 items 18/20/21). Layer: UI. Depends on nothing — no imgui, no PreviewComposite, no
// PARAMS — the same "pure header, no includes beyond what the declarations themselves need"
// shape AreaColorTable_UI.h / AreaLockTable_UI.h / AreaDragGesture_UI.h already establish for
// this subsystem's own tiny, independently-testable pieces. `MapCanvas` holds one
// AreaRecompositeThrottleState member and calls ShouldRequestAreaRecomposite once per
// ContinueAreaDrag frame; the three constants below are compile-time SAFETY-FLOOR law, not a
// Constitution §8 tunable — item 18's own ruling: a designer-facing knob that can make the
// program stutter is not a creative value, so there is no PARAMS/settings home for these.
#pragma once

namespace SanmapGen {
namespace Ui {

// ARCH §14.18 item 18 — basis tagged in the ARCH ruling itself, not re-derived here:
// `kAreaRecompositeCostBudgetMillis` is ~2.06x STEP218's measured worst representative case
// (3.88 ms) — the margin that absorbs item 17's unmeasured PrepareRun() remainder and a
// Release-vs-Debug or GPU-class difference without firing on ordinary variance.
// `kAreaRecompositeBreachFrameCount` is the smallest count that provably cannot fire on
// STEP218's own single-frame outlier (a 4.63 ms readback spike) — five consecutive frames at
// 60 Hz is ~83 ms of SUSTAINED overrun.
// `kAreaRecompositeThrottleIntervalMillis` is ~30 Hz, a rate concession only: the area is in
// its true blend equation in every frame it is drawn, in both modes (item 10's own "trades
// update rate, never correctness").
inline constexpr double kAreaRecompositeCostBudgetMillis       = 8.0;
inline constexpr int    kAreaRecompositeBreachFrameCount        = 5;
inline constexpr double kAreaRecompositeThrottleIntervalMillis  = 33.0;

// One gesture's own throttle bookkeeping. `TryBeginAreaDrag` resets this to a fresh, default-
// constructed instance on every successful begin (item 20's own "no per-gesture re-arm latch"
// ruling) — there is no cross-gesture memory of any kind.
struct AreaRecompositeThrottleState {
    int    breachFrameCount      = 0;      // consecutive over-budget samples — a COUNTER, not a
                                             // rolling window (item 20's own "one int" ruling:
                                             // a ring buffer would store values no rule reads
                                             // and would let a future edit redefine this as an
                                             // average)
    double lastRequestTimeMillis = 0.0;
    bool   bSamplePending        = false;  // a compose I asked for has since run; the next
                                             // LastComposeMillis() read belongs to THIS gesture
    bool   bDeferredMove         = false;  // the rectangle moved but the throttle ate the
                                             // request — item 21's stranding-prevention latch;
                                             // without it a moved-then-held gesture can freeze
                                             // its fill for the rest of the gesture
};

// Folds one gesture frame into `state` and answers whether ContinueAreaDrag may fire
// areaCompositeRefreshCallback NOW. `lastComposeMillis` is PreviewComposite::LastComposeMillis();
// it is only consumed when `state.bSamplePending` says the sample belongs to this gesture — the
// compose requested last frame has, by construction, already completed by the time this frame's
// ContinueAreaDrag runs (MapCanvas's gesture dispatch is frame step 8, ServiceDirtyTier() is step
// 7, ARCH §14.18 item 20's own frame-ordering argument). `nowMillis` is `ImGui::GetTime() *
// 1000.0` at the call site (deterministic under this codebase's existing fixed-`io.DeltaTime`
// test harnesses) — this header itself takes no imgui dependency of its own.
// Ruled step-by-step in ARCH §14.18 item 20 — transcribed here, not re-derived:
//   1. If a prior sample is pending, fold it into the consecutive-breach counter (any at-or-
//      under-budget sample resets it to zero — recovery is immediate and symmetric, item 20's
//      own closing ruling).
//   2. A moved rectangle sets the deferred-move latch.
//   3. A motionless pointer with nothing outstanding costs nothing (item 4's idle-cost rule).
//   4. Five-or-more consecutive breaches within the throttle interval refuses — the latch stays
//      set so the stranded move (item 21) still fires once the interval elapses.
//   5. Otherwise: record the request and clear the latch.
inline bool ShouldRequestAreaRecomposite(AreaRecompositeThrottleState& state, bool bRectangleMoved,
                                         double lastComposeMillis, double nowMillis) {
    if (state.bSamplePending) {
        state.breachFrameCount = lastComposeMillis > kAreaRecompositeCostBudgetMillis
            ? state.breachFrameCount + 1 : 0;
        state.bSamplePending = false;
    }
    if (bRectangleMoved) state.bDeferredMove = true;
    if (!state.bDeferredMove) return false;
    if (state.breachFrameCount >= kAreaRecompositeBreachFrameCount
        && nowMillis - state.lastRequestTimeMillis < kAreaRecompositeThrottleIntervalMillis)
        return false;   // throttled; bDeferredMove deliberately stays set (item 21)
    state.lastRequestTimeMillis = nowMillis;
    state.bSamplePending = true;
    state.bDeferredMove = false;
    return true;
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 2. New test file: `src/ui/AreaRecompositeThrottle_UI_Test.cpp`

Pure-logic, no GL, no imgui, no gesture, no stopwatch — fabricated costs and times only, matching `AreaDragGesture_UI_Test.cpp`'s own `Check`/`failureCount` harness.

```cpp
// AreaRecompositeThrottle_UI_Test.cpp — pure-logic acceptance for AreaRecompositeThrottle_UI.h
// (ARCH §14.18 items 18/20/21). Headless: no GL, no imgui, no MapCanvas gesture, no real
// stopwatch — every cost and every timestamp below is a fabricated literal, exercising the pure
// function ShouldRequestAreaRecomposite directly. MapCanvas_AreaDragRecomposite_UI_Test.cpp is
// the OTHER half of this ticket's coverage (a real press/drag/release cycle proving the wiring);
// this file owns the throttle's own arithmetic in isolation.
#include "AreaRecompositeThrottle_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

constexpr double kOverBudgetMillis  = 9.0;   // > kAreaRecompositeCostBudgetMillis (8.0)
constexpr double kUnderBudgetMillis = 2.0;   // <= kAreaRecompositeCostBudgetMillis (8.0)

// One request+sample round trip: fires (or refuses) the request, then folds `sampleCostMillis`
// into `state` as if that request's own compose had since completed — the fold only actually
// happens on the NEXT call, per ShouldRequestAreaRecomposite's own `bSamplePending` contract,
// so this helper issues the fold-carrying call and returns whether IT fired.
bool DriveOneMovedFrame(AreaRecompositeThrottleState& state, double nowMillis, double lastComposeMillis) {
    return ShouldRequestAreaRecomposite(state, /*bRectangleMoved=*/true, lastComposeMillis, nowMillis);
}

void RunNeverMovingChecks() {
    AreaRecompositeThrottleState state;
    bool bFiredAny = false;
    for (int frame = 0; frame < 10; ++frame)
        bFiredAny = ShouldRequestAreaRecomposite(state, /*bRectangleMoved=*/false, kUnderBudgetMillis,
                                                 static_cast<double>(frame) * 16.0) || bFiredAny;
    Check(!bFiredAny, "a never-moving gesture fires zero recomposite requests, ever");
    Check(state.breachFrameCount == 0, "and never accumulates a breach — no sample was ever pending");
}

void RunFirstMovedFrameFiresChecks() {
    AreaRecompositeThrottleState state;   // fresh, as TryBeginAreaDrag resets it every gesture
    const bool bFired = ShouldRequestAreaRecomposite(state, /*bRectangleMoved=*/true, kUnderBudgetMillis, 0.0);
    Check(bFired, "the first moved frame of a fresh gesture fires immediately (breach count starts at 0)");
    Check(state.bSamplePending, "and marks a sample as pending for the next frame to fold in");
}

// ARCH §14.18 item 18/20 — five consecutive over-budget samples engage the throttle; the sixth
// attempt, made before the 33 ms interval elapses, is refused.
void RunFiveConsecutiveBreachesEngageThrottleChecks() {
    AreaRecompositeThrottleState state;
    double nowMillis = 0.0;
    bool bAllFired = true;
    for (int request = 0; request < kAreaRecompositeBreachFrameCount; ++request) {
        bAllFired = DriveOneMovedFrame(state, nowMillis, kOverBudgetMillis) && bAllFired;
        nowMillis += 1.0;   // requests issued faster than the 33 ms interval, as a fast gesture would
    }
    Check(bAllFired, "each of the first 5 moved-and-pending requests fires — breach count is still < 5 "
                     "when each of THEM is evaluated (the 5th over-budget SAMPLE is only folded in by "
                     "the call that follows it)");
    Check(state.breachFrameCount == kAreaRecompositeBreachFrameCount,
          "after 5 consecutive over-budget samples have been folded in, the counter reads exactly 5");
    const bool bSixthFired = DriveOneMovedFrame(state, nowMillis, kOverBudgetMillis);
    Check(!bSixthFired, "the 6th request, made within the 33 ms interval with 5 consecutive breaches "
                        "on record, is throttled");
    Check(state.bDeferredMove, "and the deferred-move latch stays SET while throttled (item 21)");
}

// ARCH §14.18 item 20's own closing ruling — recovery is immediate and symmetric: ANY at-or-
// under-budget sample resets the counter to zero, un-throttling on the very next call regardless
// of elapsed time.
void RunUnderBudgetSampleClearsImmediatelyChecks() {
    AreaRecompositeThrottleState state;
    state.breachFrameCount      = kAreaRecompositeBreachFrameCount;   // fabricated: already throttled
    state.bSamplePending        = true;                               // a request's sample is due
    state.lastRequestTimeMillis = 100.0;
    state.bDeferredMove         = true;
    // nowMillis is only 5 ms after the last request — well inside the 33 ms interval, so a
    // breach-count-based refusal would still apply here if recovery were not immediate.
    const bool bFired = ShouldRequestAreaRecomposite(state, /*bRectangleMoved=*/true, kUnderBudgetMillis, 105.0);
    Check(state.breachFrameCount == 0, "a single at-or-under-budget sample resets the counter to zero");
    Check(bFired, "and the very next request fires immediately — no hysteresis, no re-arm latch, no "
                 "separate exit threshold (item 20's own explicit ruling)");
}

// ARCH §14.18 item 21 — the load-bearing case: a moved frame gets throttled, the pointer then
// holds still, and the deferred recompose must still fire once the 33 ms interval elapses, or the
// fill silently strands at the pre-hold rectangle for the rest of the gesture.
void RunDeferredMoveStrandingChecks() {
    AreaRecompositeThrottleState state;
    state.breachFrameCount      = kAreaRecompositeBreachFrameCount;   // fabricated: already throttled
    state.bSamplePending        = false;
    state.lastRequestTimeMillis = 100.0;
    state.bDeferredMove         = false;

    // Frame 1: the rectangle moves while throttled — refused, but the latch is now set.
    const bool bMovedFrameFired = ShouldRequestAreaRecomposite(state, /*bRectangleMoved=*/true,
                                                               kOverBudgetMillis, 110.0);
    Check(!bMovedFrameFired, "a moved frame while already throttled is refused (10 ms < 33 ms interval)");
    Check(state.bDeferredMove, "and sets the deferred-move latch");

    // Frame 2: the pointer holds still (bRectangleMoved=false) — still inside the interval, still
    // refused, but the latch survives a motionless frame (this is the exact bug item 21 exists to
    // prevent: a naive "only check on a moved frame" rule would never look again).
    const bool bHeldFrameFired = ShouldRequestAreaRecomposite(state, /*bRectangleMoved=*/false,
                                                              kOverBudgetMillis, 120.0);
    Check(!bHeldFrameFired, "a motionless frame still inside the interval is refused");
    Check(state.bDeferredMove, "but the latch survives the motionless frame — it is NOT cleared by "
                              "the absence of movement");

    // Frame 3: still motionless, but now past the 33 ms interval (135 - 100 = 35 >= 33) — the
    // stranded recompose fires even though THIS frame itself never moved.
    const bool bPastIntervalFired = ShouldRequestAreaRecomposite(state, /*bRectangleMoved=*/false,
                                                                 kOverBudgetMillis, 135.0);
    Check(bPastIntervalFired, "once the throttle interval elapses, the deferred recompose fires on a "
                             "MOTIONLESS frame — the stranding case item 21 exists to prevent");
    Check(!state.bDeferredMove, "and the latch clears once the stranded request has fired");
}

} // namespace

int main() {
    RunNeverMovingChecks();
    RunFirstMovedFrameFiresChecks();
    RunFiveConsecutiveBreachesEngageThrottleChecks();
    RunUnderBudgetSampleClearsImmediatelyChecks();
    RunDeferredMoveStrandingChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

---

## 3. Modified: `src/ui/PreviewComposite_UI.h`

**Accessor** — insert directly after `bool LastRunUsedGpu() const { return bLastRunUsedGpu; }` (currently line 122):

```cpp
    // ARCH §14.18 item 19 — the ONE place that measures BOTH backends, including the two silent
    // fallbacks to the Cpu twin inside ComposeOnGpu (no GL program, no texture) — those fallbacks
    // are invisible to ComposeGpuTiming by construction, and are exactly the catastrophic per-frame
    // case the Tier-B2 watchdog (AreaRecompositeThrottle_UI.h) exists to catch. Brackets the WHOLE
    // Compose() call, including PrepareRun() — deliberately wider than ComposeGpuTiming's own
    // three phases, closing STEP218's own measured blind spot (item 17). Always-on (two clock reads
    // per compose, ROUGH-ESTIMATE ~40-60ns against a >=1ms compose): a gate here would make the
    // safety floor's own input conditional, which is the exact class of bug item 7 already warns
    // against on a hot path.
    double LastComposeMillis() const { return lastComposeMillis; }
```

**Member** — insert directly after `bool bLastRunUsedGpu  = false;` (currently line 177):

```cpp
    double lastComposeMillis = 0.0;   // ARCH §14.18 item 19 — see LastComposeMillis() above.
```

No other change to this file. `ComposeRequest`, `ComposeGpuTiming` and every existing declaration are untouched.

---

## 4. Modified: `src/ui/PreviewComposite_UI.cpp`

**Add `<chrono>`** to the include block (currently lines 5-6):

```cpp
#include "PreviewComposite_UI.h"
#include <chrono>
#include <cstddef>
```

**`Compose()`** (currently lines 36-39) — bracket the whole body:

```cpp
void PreviewComposite::Compose(ComposeRequest request) {
    const std::chrono::steady_clock::time_point composeStart = std::chrono::steady_clock::now();
    if (gpuResourceManager != nullptr) ComposeOnGpu(request);
    else ComposeOnCpu();
    lastComposeMillis = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - composeStart).count();
}
```

Nothing else in this file changes.

---

## 5. Modified: `src/ui/MapCanvas_AreaDragDispatch_UI.cpp`

Full rewrite — the suppression mechanism is gone, replaced by the throttle-gated per-frame refresh.

```cpp
// MapCanvas_AreaDragDispatch_UI.cpp — MapCanvas::AreaGestureEligible/IsAreaLocked/TryBeginAreaDrag/
// ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag (ARCH §21.8). ARCH §14.18 Part 3 — the retired
// mapAreaSuppressedIndex/SetMapAreaSuppression "exactly two recomposites per gesture" design is
// GONE: TryBeginAreaDrag fires no refresh at all (selection is not a composite input);
// ContinueAreaDrag fires one real GPU recompose per frame the rectangle actually moves, gated by
// the Tier-B2 cost watchdog (AreaRecompositeThrottle_UI.h, items 18/20); EndAreaDrag's refresh is
// unconditional and exempt from the throttle. STEP212's per-area lock query (IsAreaLocked) and its
// gating of TryBeginAreaDrag's two hit-test steps are untouched by this ticket. Standalone sibling
// of MapCanvas_ManualDragDispatch_UI.cpp's 3-way Markers/Props/Decals dispatcher — Areas has no
// group/transform/lock shape to fit into that dispatcher's switch (§21.8 correction 1/5).
#include "MapCanvas_UI.h"
#include "AreaRecompositeThrottle_UI.h"
#include "AreasTab_List_UI.h"       // NextAreaName, MakeNamesUnique, ResolveAreaLocked
#include "PreviewComposite_UI.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

// STEP212 — the Areas-panel-active gate ONLY. The lock check this function used to also perform
// (`!*manualAreaDrag.bAreasLocked`) is retired: lock is now per-area, so it cannot be answered until
// a specific area is known — every call site below asks IsAreaLocked(index) once it has one.
bool MapCanvas::AreaGestureEligible() const {
    return activePanelSource != nullptr && *activePanelSource == ApplicationPanel::Areas;
}

// STEP212 — missing sources or an out-of-range index refuse (answer locked), never silently permit
// (Constitution §6 — the same "null/false-safe refuses" posture AreaGestureEligible's own panel
// gate already uses). Declared `const`: it mutates only the POINTEE of `manualAreaDrag.areaLocks`
// (a lazy append, exactly `ResolveAreaColor`'s own already-established precedent elsewhere in this
// class), never a member of `*this`. Reused verbatim by DrawAreaOverlayPass's cursor-shape section
// (MapCanvas_AreaDraw_UI.cpp) — one lock query, not two independently-maintained checks.
bool MapCanvas::IsAreaLocked(int areaIndex) const {
    if (manualAreaDrag.areas == nullptr || manualAreaDrag.areaLocks == nullptr) return true;
    if (areaIndex < 0 || areaIndex >= static_cast<int>(manualAreaDrag.areas->size())) return true;
    const Params::MapArea& area = (*manualAreaDrag.areas)[static_cast<std::size_t>(areaIndex)];
    return *ResolveAreaLocked(*manualAreaDrag.areaLocks, area.name);
}

bool MapCanvas::TryBeginAreaDrag(float regionLocalX, float regionLocalY) {
    bAreaDragActive = false;
    if (!AreaGestureEligible()) return false;
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return false;
    std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;

    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));

    // Step 1 — a selection exists AND is unlocked: hit-test THAT one area's own 8 handles + body
    // first. STEP212: the old single upfront AreaGestureEligible() lock check is replaced by this
    // per-area IsAreaLocked() test, now that the lock is per-name, not a tab-wide bool.
    if (manualAreaDrag.selectedAreaIndex != nullptr) {
        const int selectedIndex = *manualAreaDrag.selectedAreaIndex;
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())
            && !IsAreaLocked(selectedIndex)) {
            const AreaHandle_UI handle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                             *composite, view, regionLocalX, regionLocalY);
            if (handle != AreaHandle_UI::None) {
                bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, selectedIndex, handle,
                                                       worldPoint.worldX, worldPoint.worldZ);
                // ARCH §14.18 item 23-A — a begin fires NO refresh request: with the suppression
                // index gone, a begin changes no composite input (BeginAreaDragGesture does not move
                // the rectangle, and selection is not a composite input — border/handles are the
                // immediate-mode chrome pass's job, BuildMapAreaConfigurations reads no selection). A
                // begin-time recompose would produce a byte-identical image. It DOES reset the
                // watchdog, so every gesture starts un-throttled (item 20's own "no per-gesture
                // re-arm latch" — the reset lives at the START of the next gesture instead).
                if (bAreaDragActive) areaRecompositeThrottle = AreaRecompositeThrottleState();
                return bAreaDragActive;
            }
        }
    }

    // Step 2 — a miss on the selected area's own handles/body (or it was locked and so never
    // tested): body hit-test over EVERY UNLOCKED area, forward iteration, last match wins
    // (later-in-vector is drawn topmost, Widget_AreaEditor.cpp's own "reverse Z-order" comment).
    // STEP212 interpretation call 1: a LOCKED area is excluded from this scan entirely.
    int hitIndex = -1;
    for (int index = 0; index < static_cast<int>(areas.size()); ++index)
        if (!IsAreaLocked(index)
            && IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ))
            hitIndex = index;
    if (hitIndex < 0) return false;   // total miss (or every candidate locked) — release resolves click/create

    if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = hitIndex;
    bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, hitIndex, AreaHandle_UI::Center,
                                           worldPoint.worldX, worldPoint.worldZ);
    if (bAreaDragActive) areaRecompositeThrottle = AreaRecompositeThrottleState();
    return bAreaDragActive;
}

void MapCanvas::ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld) {
    if (!bAreaDragActive || manualAreaDrag.areas == nullptr || composite == nullptr) return;
    std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int areaIndex = manualAreaDrag.state.areaIndex;

    // ARCH §14.18 item 4/23-A — the exact snapshot/compare idiom AreasTab_List_UI.h's own
    // SetAreaToMapSize already establishes: "reports whether the rectangle moved, so a button press
    // that changes nothing costs no recomposite." Guarded on range exactly as UpdateAreaDragGesture's
    // own defensive check (AreaDragGesture_UI.h) — an out-of-range areaIndex snapshots nothing and
    // bRectangleMoved stays false, matching UpdateAreaDragGesture's own no-op in that case.
    const bool bAreaIndexValid = areaIndex >= 0 && areaIndex < static_cast<int>(areas.size());
    Params::MapArea beforeRect;
    if (bAreaIndexValid) beforeRect = areas[static_cast<std::size_t>(areaIndex)];

    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    UpdateAreaDragGesture(manualAreaDrag.state, areas, worldPoint.worldX, worldPoint.worldZ,
                          bShiftHeld, bCtrlHeld);

    bool bRectangleMoved = false;
    if (bAreaIndexValid) {
        const Params::MapArea& afterRect = areas[static_cast<std::size_t>(areaIndex)];
        bRectangleMoved = beforeRect.originX != afterRect.originX || beforeRect.originZ != afterRect.originZ
                        || beforeRect.width != afterRect.width || beforeRect.length != afterRect.length;
    }
    // ARCH §14.18 items 4/18/20 — the watchdog decides; this call site only supplies the frame's own
    // facts (did it move, what did the LAST compose cost, what time is it) and fires the SAME
    // areaCompositeRefreshCallback STEP211 already wired — never a second recomposite path.
    if (ShouldRequestAreaRecomposite(areaRecompositeThrottle, bRectangleMoved,
                                     composite->LastComposeMillis(), ImGui::GetTime() * 1000.0)
        && areaCompositeRefreshCallback)
        areaCompositeRefreshCallback();
}

void MapCanvas::EndAreaDrag() {
    EndAreaDragGesture(manualAreaDrag.state);
    bAreaDragActive = false;
    // ARCH §14.18 item 4/23-A — unconditional: the final rectangle must always be composited,
    // regardless of throttle state and regardless of whether the last frame moved, and is
    // explicitly EXEMPT from the throttle's interval (the gesture must always end in a
    // guaranteed-correct final image).
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
    areaRecompositeThrottle = AreaRecompositeThrottleState();
}

void MapCanvas::CreateAreaFromDrag(float pressRegionLocalX, float pressRegionLocalY,
                                   float releaseRegionLocalX, float releaseRegionLocalY) {
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate pressPixel = view.ResolvePreviewPixel(pressRegionLocalX, pressRegionLocalY);
    const PreviewPixelCoordinate releasePixel = view.ResolvePreviewPixel(releaseRegionLocalX, releaseRegionLocalY);
    const PreviewComposite::PreviewWorldPoint pressWorld = composite->PreviewPixelToWorld(
        static_cast<float>(pressPixel.pixelX), static_cast<float>(pressPixel.pixelY));
    const PreviewComposite::PreviewWorldPoint releaseWorld = composite->PreviewPixelToWorld(
        static_cast<float>(releasePixel.pixelX), static_cast<float>(releasePixel.pixelY));

    Params::MapArea area;
    area.originX = std::min(pressWorld.worldX, releaseWorld.worldX);
    area.originZ = std::min(pressWorld.worldZ, releaseWorld.worldZ);
    area.width   = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldX - pressWorld.worldX));
    area.length  = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldZ - pressWorld.worldZ));
    area.name = NextAreaName(static_cast<int>(manualAreaDrag.areas->size()));   // AreasTab_List_UI.h,
                                                                                 // the SAME helper "Add New Area" uses
    manualAreaDrag.areas->push_back(area);
    MakeNamesUnique(*manualAreaDrag.areas);   // called HERE, not left for DrawAreasTab's end-of-frame
                                               // call — see ARCH §21.8's own "Create-by-drag" section
    const int newIndex = static_cast<int>(manualAreaDrag.areas->size()) - 1;
    // STEP212 — the human's own explicit rule: a freshly created area starts UNLOCKED. Reads the
    // area's FINAL (post-MakeNamesUnique) name back out of the vector rather than reusing the local
    // `area` copy's own name.
    if (manualAreaDrag.areaLocks != nullptr)
        ResolveAreaLocked(*manualAreaDrag.areaLocks,
                          (*manualAreaDrag.areas)[static_cast<std::size_t>(newIndex)].name,
                          /*bDefaultLocked=*/false);
    if (manualAreaDrag.selectedAreaIndex != nullptr)
        *manualAreaDrag.selectedAreaIndex = newIndex;
    // ARCH §14.18 item 4/14 — a brand-new area must appear: one recomposite, unchanged by this
    // ticket. Not throttle-gated (a create is a one-shot event, not a per-frame gesture).
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 6. Modified: `src/ui/MapCanvas_AreaDraw_UI.cpp`

Full rewrite — zero fill in every state; border folds into the selected-area/handles scope.

```cpp
// MapCanvas_AreaDraw_UI.cpp — MapCanvas::DrawAreaOverlayPass. ARCH §14.18 Part 3 (items 1/9) — an
// area's fill has EXACTLY ONE renderer, in every state including mid-gesture: the composite. This
// pass draws chrome ONLY — border (when the MapAreas field layer is enabled AND the area is
// selected) and the 8 resize handles (selected-area only), sharing one `if (selectedIndex ...)`
// scope and one pair of corner projections. There is no `AddRectFilled` anywhere in this file.
// STEP212's per-area-lock-gated cursor-shape feedback is unchanged.
#include "MapCanvas_UI.h"
#include "PreviewComposite_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// ARCH §14.17 item 12's own preferred path: read through the canvas's existing `const
// PreviewComposite*` — it needs no new plumbing at all, since `Settings()` already has a const
// overload. A plain linear scan (not `PreviewFieldLayerOfKind`, which has no const overload) since
// this is the one place a const settings reference is available.
bool IsMapAreasLayerEnabled(const PreviewCompositeSettings& settings) {
    for (const PreviewFieldLayer& layer : settings.fieldLayers)
        if (layer.kind == PreviewLayerKind::MapAreas) return layer.bEnabled;
    return false;
}

} // namespace

void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int selectedIndex = manualAreaDrag.selectedAreaIndex != nullptr ? *manualAreaDrag.selectedAreaIndex : -1;

    auto ToScreen = [&](float worldX, float worldZ) {
        const PreviewComposite::PreviewPixelPoint pixel = composite->WorldToPreviewPixel(worldX, worldZ);
        const RegionLocalPoint local = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
        return ImVec2(regionOriginX + local.regionLocalX, regionOriginY + local.regionLocalY);
    };

    // ARCH §14.18 item 9 — border + the 8 handles, ONE scope, selected-area only. The border draws
    // only when the MapAreas layer is enabled; the "layer disabled => no border at all, regardless
    // of selection" rule is unchanged and still law. The handles draw unconditionally for the
    // selected area, exactly as before this ticket.
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(selectedIndex)];
        if (IsMapAreasLayerEnabled(composite->Settings())) {
            const ImVec2 nwScreen = ToScreen(area.originX, area.originZ);
            const ImVec2 seScreen = ToScreen(area.originX + area.width, area.originZ + area.length);
            drawList->AddRect(nwScreen, seScreen, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
        }
        AreaHandleWorldPoint_UI handlePoints[8];
        ComputeAreaHandleWorldPoints(area, handlePoints);
        const ImU32 handleColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        for (const AreaHandleWorldPoint_UI& handlePoint : handlePoints)
            drawList->AddCircleFilled(ToScreen(handlePoint.worldX, handlePoint.worldZ),
                                      kAreaHandleScreenRadiusPixels, handleColor);
    }

    // Cursor-shape feedback — hover-only, re-hit-tested fresh against the CURRENT cursor position —
    // gated on the cursor being within the canvas region (view.RegionSidePixels(), not
    // ImGui::IsItemHovered(), since this pass runs before this frame's InvisibleButton is declared).
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(areas.size())) return;
    // STEP212 fix — a LOCKED selected area shows no drag-affordance cursor at all: falls through to
    // imgui's own default arrow. Reuses IsAreaLocked, the SAME query TryBeginAreaDrag itself gates
    // on (MapCanvas_AreaDragDispatch_UI.cpp) — one lock check, not a second, independently-maintained
    // one.
    if (IsAreaLocked(selectedIndex)) return;
    const ImGuiIO& io = ImGui::GetIO();
    const float hoverRegionLocalX = io.MousePos.x - regionOriginX;
    const float hoverRegionLocalY = io.MousePos.y - regionOriginY;
    const float regionSide = view.RegionSidePixels();
    if (hoverRegionLocalX < 0.0f || hoverRegionLocalY < 0.0f
        || hoverRegionLocalX > regionSide || hoverRegionLocalY > regionSide) return;

    const AreaHandle_UI hoveredHandle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                           *composite, view, hoverRegionLocalX, hoverRegionLocalY);
    switch (hoveredHandle) {
        case AreaHandle_UI::N: case AreaHandle_UI::S: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
        case AreaHandle_UI::E: case AreaHandle_UI::W: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
        case AreaHandle_UI::NE: case AreaHandle_UI::SW: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
        case AreaHandle_UI::NW: case AreaHandle_UI::SE: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
        case AreaHandle_UI::Center: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll); break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
```

Note: `#include "AreasTab_List_UI.h"` is dropped — `ResolveAreaColor` was its sole use in this file, and that call is deleted along with the fill.

---

## 7. Modified: `src/ui/MapCanvas_ManualDragSources_UI.h`

**Delete** the comment block and field (currently lines 60-64):

```cpp
    // ARCH §14.17 item 11 — mutable: the canvas sets/clears this to omit the dragged area from the
    // composite input for the duration of a gesture. Points at
    // `PreviewCompositeSettings::mapAreaSuppressedIndex` — one source of truth, never a second copy.
    // STEP212 — untouched; this field's own plumbing is STEP211 territory.
    int*                          mapAreaSuppressedIndex = nullptr;
```

leaving the struct as:

```cpp
struct ManualAreaDragSources_UI {
    std::vector<Params::MapArea>* areas             = nullptr;   // mutable: canvas creates/moves/resizes
    std::vector<AreaColorEntry>*  areaColors         = nullptr;   // mutable: ResolveAreaColor lazily
                                                                    // appends a default entry for a
                                                                    // freshly canvas-created area
    // STEP212 — replaces the retired `const bool* bAreasLocked`: one lock bit PER AREA, the exact
    // same UI-only name-keyed side-table shape as `areaColors` above (AreaLockTable_UI.h's own
    // AreaLockEntry/ResolveAreaLocked, mirroring AreaColorTable_UI.h's AreaColorEntry/
    // ResolveAreaColor). Mutable because ResolveAreaLocked lazily appends a default-LOCKED entry on
    // first touch, exactly as ResolveAreaColor already does for areaColors, AND because
    // CreateAreaFromDrag must insert a freshly created area's own entry as UNLOCKED (STEP212 Fix 1).
    // Unlike areaColors, this table has NO composite-side reader at all — its single owner stays
    // `AreasTabState::areaLocks`, never `PreviewCompositeSettings`.
    std::vector<AreaLockEntry>*   areaLocks          = nullptr;
    int*                          selectedAreaIndex  = nullptr;   // mutable: auto-select-on-touch/deselect
    AreaDragGestureState           state;
};
```

No `#include` changes needed in this file.

---

## 8. Modified: `src/ui/MapCanvas_UI.h`

**Add `#include`** — insert after `#include "Application_Panels_UI.h"` (currently line 29):

```cpp
#include "Application_Panels_UI.h"
#include "AreaRecompositeThrottle_UI.h"
#include "DecalDragGesture_UI.h"
```

**`SetManualAreaDragSource`** (currently lines 155-172) — drop the fifth parameter and its assignment, rewrite the comment:

```cpp
    // ARCH §21.8 / STEP212 — mirrors SetManualPropDragSource's shape minus Geometry/
    // globalSymmetryRecipe (Areas carry no symmetry/layer concept of their own, §21.8 correction
    // 1/3). `areas`/`areaColors`/`areaLocks`/`selectedAreaIndex` are all mutable: STEP212 replaced
    // the retired, read-only, tab-wide `const bool* areasLocked` with a per-area lock TABLE the
    // canvas legitimately writes into too (CreateAreaFromDrag inserts a freshly created area's own
    // entry as unlocked). ARCH §14.18 item 23-D — the FIFTH parameter, `mapAreaSuppressedIndex`, is
    // RETIRED along with the composite-side field it pointed at: with the suppression mechanism
    // gone, this setter's own signature shrinks back to four pointers.
    void SetManualAreaDragSource(std::vector<Params::MapArea>* areas, std::vector<AreaColorEntry>* areaColors,
                                  std::vector<AreaLockEntry>* areaLocks, int* selectedAreaIndex) {
        manualAreaDrag.areas = areas; manualAreaDrag.areaColors = areaColors;
        manualAreaDrag.areaLocks = areaLocks; manualAreaDrag.selectedAreaIndex = selectedAreaIndex;
    }
    // ARCH §14.18 items 4/18/20 — the recomposite-request callback. Mirrors
    // SetSelectionChangedCallback's own injection shape verbatim. Unset = no refresh, never a
    // crash. Fired: never at TryBeginAreaDrag (a begin changes no composite input); once per frame
    // ContinueAreaDrag's own dragged rectangle actually MOVED, subject to the Tier-B2 cost watchdog
    // (AreaRecompositeThrottle_UI.h) throttling to at most one per kAreaRecompositeThrottleIntervalMillis
    // after kAreaRecompositeBreachFrameCount consecutive over-budget composes; unconditionally,
    // exempt from the throttle, at EndAreaDrag; and once, unthrottled, at CreateAreaFromDrag.
    void SetAreaCompositeRefreshCallback(std::function<void()> refreshCallback) {
        areaCompositeRefreshCallback = std::move(refreshCallback);
    }
```

(The `SetAreaCompositeRefreshCallback` method itself does not move in the file — only its contract comment, which previously promised *"Fired exactly twice per drag … never once per ContinueAreaDrag frame,"* the exact opposite of the law this ticket ships, is rewritten in place, immediately above its own existing declaration.)

**Delete** the `SetMapAreaSuppression` declaration and its comment (currently lines 358-361):

```cpp
    // ARCH §14.17 item 11 — writes `*manualAreaDrag.mapAreaSuppressedIndex` null-safely and fires
    // `areaCompositeRefreshCallback` only when the value actually changed, so TryBeginAreaDrag/
    // EndAreaDrag never hold two copies of that "did it actually change" condition.
    void SetMapAreaSuppression(int areaIndex);                                 // MapCanvas_AreaDragDispatch_UI.cpp
```

**Add member** — insert directly after `bool bAreaDragActive = false;` (currently line 443):

```cpp
    ManualAreaDragSources_UI manualAreaDrag;
    bool                     bAreaDragActive = false;
    // ARCH §14.18 items 18/20 — the Tier-B2 cost watchdog's own per-gesture bookkeeping. Reset to a
    // fresh instance on every successful TryBeginAreaDrag and again at EndAreaDrag — no cross-
    // gesture memory of any kind.
    AreaRecompositeThrottleState areaRecompositeThrottle;
    // ARCH §14.17 item 11 — the recomposite-request callback (see SetAreaCompositeRefreshCallback).
    std::function<void()>    areaCompositeRefreshCallback;
```

This class was already over its §21.7 size ceiling; the net change here is -1 method parameter, -1 declaration (`SetMapAreaSuppression`), +1 member (`areaRecompositeThrottle`) — the correct direction.

---

## 9. Modified: `src/ui/PreviewComposite_Settings_UI.h`

**Delete** the comment block and field (currently lines 105-109):

```cpp
    // ARCH §14.17 item 11 — the ONE area currently mid-drag/resize/move on the canvas, omitted from
    // this frame's composited input so a live drag costs exactly two recomposites (begin+end), never
    // one per frame. Transient interaction state: NEVER serialized, and an out-of-range value
    // suppresses nothing (the safe degradation if a list reorder ever races a gesture).
    int mapAreaSuppressedIndex = -1;
```

`areaColors` (item 9's own field, immediately above) is untouched.

---

## 10. Modified: `src/ui/PreviewComposite_Prepare_UI.cpp`

**Delete** the suppression skip inside `BuildMapAreaConfigurations` (currently line 139):

```cpp
void PreviewComposite::BuildMapAreaConfigurations() {
    mapAreaRectangles.clear();
    const float cellsPerWorldUnit = ReciprocalOrZero(settings.worldUnitsPerCell);
    for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(index)];
        PreviewMapAreaRectangle record;
        record.minimumX = area.originX * cellsPerWorldUnit;
        record.minimumZ = area.originZ * cellsPerWorldUnit;
        record.maximumX = (area.originX + area.width) * cellsPerWorldUnit;
        // ... rest of the function unchanged ...
```

(Only the one `if (index == settings.mapAreaSuppressedIndex) continue;` line is removed; nothing else in the function changes.)

---

## 11. Modified: `src/ui/Application_UI.cpp`

**The `SetManualAreaDragSource` call** (currently lines 151-159) drops its fifth argument and the comment's suppression clause:

```cpp
    // ARCH §21.8 / §14.17 item 9 / STEP212 — the Area canvas gesture's drag source: `recipe.areas`,
    // `composite.Settings().areaColors` and `tabState.areas.areaLocks` are the SAME storage the
    // tab/composite already own — one source of truth, never a second copy. `areaLocks` replaces
    // the retired `&tabState.areas.bAreasLocked` (STEP212 — per-area lock); it stays TAB-owned,
    // unlike `areaColors`, because lock has no composite-side reader (AreaLockTable_UI.h's own
    // ruling). ARCH §14.18 item 23-D/G — the fifth argument, `mapAreaSuppressedIndex`, is retired
    // along with the composite-side field and setter parameter it fed.
    canvas.SetManualAreaDragSource(&recipe.areas, &composite.Settings().areaColors,
                                   &tabState.areas.areaLocks, &tabState.areas.selectedAreaIndex);
```

No other line in this file changes — `SetAreaCompositeRefreshCallback`'s own binding (`canvas.SetAreaCompositeRefreshCallback([this] { previewDriver.NotifyParametersChanged(); });`) is untouched; it still just derives the tier, same as every other presentation-only mutation path.

---

## 12. Test file: `src/ui/MapCanvas_AreaDragRecomposite_UI_Test.cpp` (new, replaces `MapCanvas_AreaDragSuppression_UI_Test.cpp`)

`MapCanvas_AreaDragSuppression_UI_Test.cpp` is **deleted** — its entire premise ("exactly two recomposites per gesture … never one per ContinueAreaDrag frame") is now the opposite of law, and it references the deleted `mapAreaSuppressedIndex` field/parameter, so it cannot compile unmodified. This new file replaces it, GL-backed for the same reason the old one was (`TryBeginAreaDrag`/`ContinueAreaDrag`/`EndAreaDrag`/`CreateAreaFromDrag` are `MapCanvas`-private).

```cpp
// MapCanvas_AreaDragRecomposite_UI_Test.cpp — ARCH §14.18 Piece C acceptance (supersedes the
// retired MapCanvas_AreaDragSuppression_UI_Test.cpp — its entire premise, "exactly two
// recomposites per gesture," is now the opposite of law): TryBeginAreaDrag fires ZERO refresh
// requests (a begin changes no composite input — selection is not a composite input);
// ContinueAreaDrag fires exactly one refresh per frame the dragged rectangle actually moved and
// ZERO on a held-but-motionless frame; EndAreaDrag's refresh is unconditional, always fires
// exactly once, regardless of throttle state or whether the final frame moved; CreateAreaFromDrag's
// single request is unchanged. GL-backed (mirrors the retired test's own technique exactly)
// because TryBeginAreaDrag/ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag are MapCanvas-private —
// the only way to exercise them is through a real MapCanvas::Draw() press/drag/release sequence.
// The composite's own real Compose() cost, measured on this test's tiny fixture scene, stays well
// under kAreaRecompositeCostBudgetMillis for the two-move sequence this file drives, so the
// watchdog never engages here regardless of the exact (machine-dependent) measured value — see the
// per-frame walkthrough in this ticket's own text for why that is true independent of the number.
// AreaRecompositeThrottle_UI_Test.cpp is the throttle's own dedicated, fully-deterministic,
// GPU-free coverage of the watchdog's arithmetic in isolation.
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
    ImGui::Begin("AreaDragRecompositeTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

ImVec2 ScreenPositionForWorld(MapCanvas& canvas, const PreviewComposite& composite,
                              float worldX, float worldZ) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

} // namespace

void RunMapCanvasAreaDragRecompositeChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    std::vector<Params::MapArea> areas;
    Params::MapArea existingArea;
    existingArea.name = "Existing";
    existingArea.originX = 1.0f; existingArea.originZ = 1.0f;
    existingArea.width = 1.0f;   existingArea.length = 1.0f;
    areas.push_back(existingArea);
    std::vector<AreaColorEntry> areaColors;
    std::vector<AreaLockEntry>  areaLocks;
    // STEP212's per-area lock table defaults a first-touch name to LOCKED — pre-seed "Existing"
    // UNLOCKED explicitly, mirroring the retired test's own established precedent.
    ResolveAreaLocked(areaLocks, existingArea.name, /*bDefaultLocked=*/false);
    int  selectedAreaIndex = -1;
    int  refreshCount = 0;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    ApplicationPanel activePanel = ApplicationPanel::Areas;
    canvas.SetActivePanelSource(&activePanel);
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex);
    canvas.SetAreaCompositeRefreshCallback([&] { ++refreshCount; });

    // --- Case 1: create-by-drag on empty canvas space fires exactly ONE refresh (unchanged) ---
    const ImVec2 emptyPressPosition = ScreenPositionForWorld(canvas, composite, 3.2f, 3.2f);
    io.AddMousePosEvent(emptyPressPosition.x, emptyPressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(emptyPressPosition.x + 60.0f, emptyPressPosition.y + 60.0f);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);

    check(areas.size() == 2u, "a press-drag-release on empty canvas space creates a new area");
    check(refreshCount == 1, "create-by-drag requests exactly one recomposite");

    // --- Case 2: a body-move on the pre-existing area fires ZERO refreshes at begin, ONE per
    // moved frame, ZERO on a held-but-motionless frame, and exactly ONE unconditional refresh at
    // release ---
    selectedAreaIndex = 0;   // the pre-existing "Existing" area, index 0
    const int refreshCountBeforeMove = refreshCount;
    // Dead center of the 1x1 world rect — ~32 screen px from every 8px handle circle at this zoom,
    // so step 1's handle hit-test correctly misses and step 2's body/AABB test correctly hits.
    const ImVec2 bodyPressPosition = ScreenPositionForWorld(canvas, composite, 1.5f, 1.5f);
    io.AddMousePosEvent(bodyPressPosition.x, bodyPressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove,
          "TryBeginAreaDrag fires NO refresh request — a begin changes no composite input");

    // A moving frame: the rectangle actually changes (originX/originZ), so exactly one refresh
    // fires.
    io.AddMousePosEvent(bodyPressPosition.x + 20.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 1,
          "ContinueAreaDrag requests exactly one recomposite on a frame the rectangle moved");

    // A held-but-motionless frame (mouse position unchanged since the last frame): zero refreshes.
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 1,
          "ContinueAreaDrag requests ZERO recomposites on a held-but-motionless frame");

    // Another moving frame: one more refresh.
    io.AddMousePosEvent(bodyPressPosition.x + 30.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 2,
          "a second moved frame requests a second recomposite");

    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 3,
          "EndAreaDrag's refresh is unconditional and always fires, exactly once, at release");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 13. Modified: `src/ui/MapCanvas_UI_Test.cpp`

**Declaration** (currently lines 31-32):

```cpp
// ARCH §14.18 Piece C — MapCanvas_AreaDragRecomposite_UI_Test.cpp.
void RunMapCanvasAreaDragRecompositeChecks(Sys::GpuResourceManager& manager);
```

**Call site** (currently line 59):

```cpp
    Ui::RunMapCanvasAreaDragRecompositeChecks(manager);
```

Nothing else in this file changes.

---

## 14. Modified: `src/ui/MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp`

**Line 129** — delete the local variable:

```cpp
    int selectedAreaIndex = -1;
```

(the `int mapAreaSuppressedIndex = -1;` line directly below it is removed entirely.)

**Lines 143-144** — drop the fifth argument:

```cpp
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex);
```

No other line in this file changes — every one of its `RunEHandleResizeGesture` calls and its own comparison assertions are untouched, since none of them ever read `mapAreaSuppressedIndex`.

---

## 15. Modified: `src/ui/PreviewComposite_MapAreas_UI_Test.cpp`

**Header comment** — drop the retired clause (currently line 5):

```cpp
// PreviewComposite_MapAreas_UI_Test.cpp — ARCH §14.17 acceptance: Params::MapArea rectangles
// compositing as a real PreviewFieldLayer (`PreviewLayerKind::MapAreas`) — an empty list paints
// nothing (the degenerate sentinel), a single area colors every cell it covers, and overlapping
// areas resolve forward-iteration LAST-match-wins (the same Z rule §21.8's own body hit-test
// uses). Runs the Cpu twin only — no GL context needed (PreviewComposite_UI_Test.cpp's own
// established posture).
```

**Delete** the entire `TestSuppressedIndexOmitsRectangle` function (currently lines 86-119) — it asserts retired law (`PreviewCompositeSettings::mapAreaSuppressedIndex` no longer exists, so it cannot compile unmodified) and is not a regression to preserve:

```cpp
void TestOverlapLastMatchWins() {
    // ... unchanged, exactly as it stands today ...
}

} // namespace

int main() {
    TestEmptyAreaListPaintsNothing();
    TestSingleAreaColorsCoveredCells();
    TestOverlapLastMatchWins();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
```

(`TestSuppressedIndexOmitsRectangle`'s own preceding comment block, its body, and its call in `main()` are all removed together — nothing else in this file changes.)

---

## 16. `CMakeLists.txt`

**Rename the suppression test source and its comment** (currently lines 592-593, inside the `MapCanvas_UI_Test` multi-file target):

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
    # ARCH §14.18 Piece C — zero refresh at begin, one per moved frame, zero on a held-but-
    # motionless frame, and an unconditional refresh at release (supersedes the retired
    # "exactly two recomposites per gesture" suppression-index design).
    src/ui/MapCanvas_AreaDragRecomposite_UI_Test.cpp
    # STEP214 — Alt is an additional trigger for the identical Ctrl center-resize behavior.
    src/ui/MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp)
```

**New standalone test target** — insert directly after the existing `add_sangen_test(AreaDragGesture_UI_Test src/ui/AreaDragGesture_UI_Test.cpp)` line (currently line 878):

```cmake
add_sangen_test(AreasTab_UI_Test          src/ui/AreasTab_UI_Test.cpp)
add_sangen_test(AreaDragGesture_UI_Test   src/ui/AreaDragGesture_UI_Test.cpp)
# ARCH §14.18 items 18/20 — the Tier-B2 cost watchdog's pure decision function: headless, no GL,
# no imgui, fabricated costs/times only.
add_sangen_test(AreaRecompositeThrottle_UI_Test src/ui/AreaRecompositeThrottle_UI_Test.cpp)
add_sangen_test(PlacementRuleSections_UI_Test src/ui/PlacementRuleSections_UI_Test.cpp)
```

The four new/modified production files (`AreaRecompositeThrottle_UI.h`, `PreviewComposite_UI.h`/`.cpp`, `MapCanvas_AreaDragDispatch_UI.cpp`, `MapCanvas_AreaDraw_UI.cpp`, `MapCanvas_ManualDragSources_UI.h`, `MapCanvas_UI.h`, `PreviewComposite_Settings_UI.h`, `PreviewComposite_Prepare_UI.cpp`, `Application_UI.cpp`) need no CMake edit of their own — they are already covered by the existing `src/ui/*.cpp`/`*.h` glob.

---

## ARCH rules invoked
- `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` Part 3 (items 17-24) — this ticket's entire binding law; every deletion/addition above traces to item 23's own file-by-file delta, verified against the live tree rather than transcribed blindly (several of item 23's own cited line numbers had already drifted by the time this ticket was drafted, per its own instruction to re-verify).
- Item 1 — no second fill renderer, in any state; enforced by `DrawAreaOverlayPass`'s complete removal of `AddRectFilled`.
- Item 4 — a held-but-motionless pointer costs nothing; enforced by `ShouldRequestAreaRecomposite`'s own step 3.
- Items 18/20/21 — the watchdog's three constants, its state shape, its five-step decision body and the `bDeferredMove` stranding fix, transcribed verbatim into `AreaRecompositeThrottle_UI.h`.
- Item 19 — `PreviewComposite::Compose()` self-measures the whole call, both backends, closing item 17's `PrepareRun()` blind spot; `ComposeGpuTiming`/`ComposeOnGpu`'s own `outTiming` are untouched.
- Constitution §1 — UI sets PARAMS/settings and requests a recomposite; it never composites itself and never re-derives the watchdog's decision at more than one call site.
- Constitution §6 — every index (`areaIndex`, `selectedIndex`) is range-checked before dereference in both the dispatch and the draw pass, unchanged from before this ticket.
- Constitution §8 — the three watchdog constants are explicitly ruled as compile-time safety-floor law, **not** a §8 tunable (item 18's own distinction) — they are `inline constexpr`, not settings.

## Explicit out-of-scope
- **No change to `ComposeGpuTiming`, `ComposeOnGpu`'s `outTiming`, or the STEP218 benchmark binary** (item 24). `LastComposeMillis()` is a different bracket, a different lifetime, and a different consumer — it does not replace or feed STEP218's own diagnostic.
- **The fence spin and the entity-id readback stay** (item 7/24) — still the named next lever, still gated on a picking-correctness argument this ticket does not make.
- **Item 17's two newly-identified levers are NOT built here**: hoisting `WorldToPreviewPixel`'s loop-invariant calls out of `BuildEntityPoints`'s per-instance loop, and skipping `compositeTexels.assign(...)` when `bNeedsTexelReadback == false`. Both are a separate ticket, owned by the SanGen Compute Optimization Expert, gated on a **Release**-build re-run of STEP218 with the timing window widened to include `PrepareRun()` — flagged here per the task's own instruction, not implemented.
- **No per-area blend mode** (item 15). **No shader callback, no third blend-math implementation** (items 1-2).
- **No `.sanmap` schema, `Params::MapArea`, picking or `SanGenVersion` change of any kind.**
- **No change to `AreasTab_UI.h`/`.cpp`/`AreasTab_List_UI.h`/`AreaColorTable_UI.h`/`AreaLockTable_UI.h`/`MapArea_PARAMS.h`** — every one of Part 1/2's earlier pieces (STEP211/212/216/217) is unmodified by this ticket.
- **No change to `AreaDragGesture_UI.h`/`.cpp`** — `kAreaHandleScreenRadiusPixels`/`kAreaMinimumExtentWorldUnits` and the resize/move math are untouched; this ticket only changes WHEN a recompose is requested and HOW the fill is rendered, never the gesture's own geometry.

## Acceptance test (end-to-end, in addition to the two test files' own unit coverage)
1. Dragging a selected area's body or handle recomposites the visible fill live, every frame the rectangle actually moves — confirmed by `MapCanvas_AreaDragRecomposite_UI_Test.cpp`'s own refresh-count assertions standing in for a real GPU recompose (the test's callback counts requests; `Application_UI.cpp`'s real wiring turns each one into `previewDriver.NotifyParametersChanged()`, unchanged by this ticket).
2. Pressing to begin a drag (no movement yet) fires zero recomposite requests; releasing always fires exactly one, regardless of whether the last held frame moved.
3. A held-but-motionless frame mid-drag fires zero requests — the idle path is preserved, `PumpWindowEvents` is never kept artificially hot by a stationary pointer.
4. `DrawAreaOverlayPass` never calls `AddRectFilled` — grep confirms it after this ticket lands. The composite's own `MapAreas` field layer is the only thing that ever paints an area's interior, in every state.
5. A synthetic five-consecutive-over-budget-compose sequence engages the throttle (`AreaRecompositeThrottle_UI_Test.cpp`); a single at-or-under-budget sample clears it immediately; a moved-then-held gesture while throttled still fires the stranded recompose once the 33 ms interval elapses.
6. Full `SanGenV2` build stays clean; every existing test continues to pass; both new test binaries (`AreaRecompositeThrottle_UI_Test`, and `MapCanvas_UI_Test` with its renamed `MapCanvas_AreaDragRecomposite_UI_Test.cpp` case) pass with `ALL PASS`.
7. `grep -rn "mapAreaSuppressedIndex\|SetMapAreaSuppression" src/` returns zero matches anywhere in the tree after this ticket lands.

## Interpretation calls made beyond the ARCH ruling's text
1. **`ContinueAreaDrag`'s snapshot is taken via `manualAreaDrag.state.areaIndex` read into a local, rather than re-deriving the dragged index some other way** — this is the only index the gesture state carries, and it is the same one `UpdateAreaDragGesture` itself validates defensively; no second source of truth is introduced.
2. **The snapshot/compare uses four `!=` float comparisons, not an epsilon-tolerant "near enough" check** — deliberate: `SetAreaToMapSize`'s own cited precedent (`AreasTab_List_UI.h`) is a plain bit-exact compare, and `UpdateAreaDragGesture`'s math is deterministic given identical inputs, so a truly motionless frame produces bit-identical floats, never a near-miss that an epsilon would be needed to catch.
3. **`AreaRecompositeThrottle_UI_Test.cpp`'s `DriveOneMovedFrame` helper is a same-file-local convenience, not exposed by the header** — the ARCH ruling's own pseudocode is transcribed directly into the production header; the test's helper only avoids repeating the two-argument call shape across its own loop.
4. **The watchdog constants' placement is the new header itself, not literally beside `kAreaHandleScreenRadiusPixels`/`kAreaMinimumExtentWorldUnits` in `AreaDragGesture_UI.h`** — read as a stylistic parallel (top-of-file `inline constexpr` block, same as that header already establishes), not a literal same-file requirement; item 20 itself separately mandates a **new**, dedicated, dependency-free header for the throttle, which a same-file placement would contradict.
5. **`MapCanvas_AreaDragRecomposite_UI_Test.cpp`'s two-moved-frame sequence is argued, not benchmarked, to never trip the watchdog** — the walkthrough is in this ticket's own text (a stale first-compose `LastComposeMillis()` value can push `breachFrameCount` to at most 1 within this short sequence, far under the 5-breach threshold) rather than asserted with a numeric bound, since the real GL compose cost is machine-dependent and this test must stay deterministic across machines.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_14_18_AreaLiveBlendFidelityAndPalette.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDragDispatch_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDraw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ManualDragSources_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaDragGesture_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Settings_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Prepare_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Gpu_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Cpu_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDragSuppression_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_MapAreas_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreaDragGesture_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt`,
and `work_orders\STEP210_AreaCanvasGesture_UI.md` used as this document's own structure/rigor template.
