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
