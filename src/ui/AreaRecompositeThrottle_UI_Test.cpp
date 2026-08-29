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
    // Only 4 of the 5 fired requests' own samples have been folded in by this point — the 5th
    // request's (request index 4's) sample is pending and is only folded in by the NEXT call
    // (ShouldRequestAreaRecomposite's own `bSamplePending` contract: a sample is folded in at the
    // START of the call that follows the request that produced it, never the request's own call).
    Check(state.breachFrameCount == kAreaRecompositeBreachFrameCount - 1,
          "after 5 fired requests, only 4 of their own samples have been folded in — the 5th is "
          "still pending");
    const bool bSixthFired = DriveOneMovedFrame(state, nowMillis, kOverBudgetMillis);
    Check(!bSixthFired, "the 6th request, made within the 33 ms interval, folds in the 5th request's "
                        "own pending sample FIRST (reaching exactly 5 consecutive breaches) and is "
                        "then itself throttled");
    Check(state.breachFrameCount == kAreaRecompositeBreachFrameCount,
          "the 6th call's own fold-in step brings the counter to exactly 5");
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
