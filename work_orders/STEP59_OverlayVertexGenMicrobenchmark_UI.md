# STEP59 — Overlay vertex-gen microbenchmark: SIMD-transform / bulk-write / naive-`AddImage`, real numbers for §14.9's placeholder budget

**Layer:** UI (test-only; produces no shipped production code). **Domain:** `MapCanvas` overlay
rendering (`OverlayLayer_UI`), the exact code STEP53 lands. **Sequence:** Phase 3.3,
`work_orders/SEQUENCE_PreviewOverlayLayering.md`. This is the benchmark §14.9 names by number of
instances and scenario, and that `SEQUENCE_PreviewOverlayLayering.md` line 42 marks
"READY, but sequenced after STEP53 is implemented" — this ticket is the draft that line points at.

**Depends on STEP53 (`STEP53_OverlayIconDrawPass_UI.md`), which must be IMPLEMENTED — not merely
DRAFTED — and buildable before this ticket is dispatchable.** This ticket needs STEP53's actual
built binary/public surface to call into (the real per-layer cull path, the real
`FlushIconLayerBucket`-shaped bulk-write function); it does not reimplement or approximate either.
STEP53 itself is gated on STEP47/STEP50/STEP51/STEP52 — by the time STEP53 is implemented those are
already landed too, so this ticket inherits that chain without adding a new dependency of its own.
**Do not dispatch this ticket until STEP53 has landed and its test suite is green.**

## Problem
`ARCH_14_09_RenderingPerformance.md` §14.9 states the cross-layer visible-vertex budget default (~400,000-500,000 instances)
is "explicitly a placeholder pending a real microbenchmark (SIMD-transform, bulk-write, and
naive-`AddImage` timed separately at N ∈ {100k, 300k, 600k}, both 0%-culled and ~5%-visible, on
real dev hardware, before this number becomes a ratified constant per Constitution §7/§12
basis-tag law)." §14.13 item 2 restates this as one of only two genuinely open items left by the
§14 ratification. STEP53 itself ships that placeholder as a named tweakable
(`OverlayRenderingSettings::visibleInstanceBudget = 450000`) with a comment pointing at this exact
future benchmark, and lists "Phase 3.3's microbenchmark itself" as explicitly out of its own scope,
sequenced after it specifically because it needs STEP53's built binary to exist first.

**Note on the "§12" citation.** `ARCH_14_09_RenderingPerformance.md` §14.9 and STEP53 both cite "Constitution §7/§12
basis-tag law"; `sangen_arch_pack/specs/OPTIMIZATION_PILLARS.md` also cites "Constitution §12
basis tags." `sangen_arch_pack/CONSTITUTION.md` as it stands today has only 8 numbered sections —
there is no §12. §7 ("Work-order schema") is the section that actually states the basis-tag
requirement: "solution + benchmark-backed performance estimate (with basis)." This ticket treats
§7 as the operative citation and flags the "§12" references elsewhere in the ARCH pack as a
likely stale/renumbered cross-reference for the ARCH Expert to reconcile — not something this
ticket invents a fix for.

## Fix

### 0. Existing precedent in this codebase (confirmed before proposing anything new)
Grepped `src/` for `benchmark`/`Benchmark` and for wall-clock timing APIs
(`std::chrono`, `high_resolution_clock`, `steady_clock`, `QueryPerformanceCounter`). Exactly one
hit for both: `src/proc/Placement_Symmetry_PROC_Test.cpp`, "Acceptance test 9" (its own comment:
"throughput check, typical vs. worst case... Simple wall-clock comparison, not a formal benchmark
harness"). Its shape:
- `std::chrono::steady_clock::now()` around the timed call, `std::chrono::duration<double, milli>`
  to get elapsed ms.
- `std::printf` reports the raw numbers (and a ratio) unconditionally, every run, for a human to
  read — not a machine-parsed report format, no new dependency.
- The one `Check()` this test applies to the timing is a **loose sanity bound**
  (`worstMillis < 5000.0`, "stays well under 5s (record the actual ratio above; flag a follow-up
  ticket if it is severe, do not hand-wave it)") — not an assertion on the actual measured number.

Confirmed separately: every `_Test.cpp` in this codebase's `Check(bool bCondition, const char*
label)` helper (74 files use some variant; representative definition,
`ApplicationShell_Window_UI_Test.cpp:33-37`) takes a **boolean** and prints `FAIL:` + increments a
failure counter on false — it has no notion of "how fast," confirming the task brief's own
hypothesis. There is no dedicated benchmark framework anywhere in this codebase; this ticket
follows the one precedent above rather than inventing new test infrastructure.

Also confirmed: `MapCanvas_Render_UI_Test.cpp` (`RunMapCanvasRenderChecks`) is this codebase's
existing technique for driving a real `ImDrawList` without a live window or renderer backend —
`ImGui::CreateContext()`, a manual `BeginHeadlessFrame()` (sets `io.DisplaySize`/`io.DeltaTime`,
builds the font atlas the legacy way, `ImGui::NewFrame()`), draw calls, `ImGui::Render()`, then
inspects `ImGui::GetDrawData()`. This is needed here too — timings 2 and 3 below are genuine
`ImDrawList` operations (`PrimReserve`/`PrimWriteVtx`/`PrimWriteIdx` and `AddImage` both require a
live draw list inside a frame).

### 1. New dedicated test binary, not bundled into `MapCanvas_UI_Test`
`CMakeLists.txt`'s own stated convention (line 331 comment): "each composite acceptance test
declares its own `main()`, so each is its own binary." Mirror that here rather than adding this
benchmark's synthetic-600k-instance runs into the existing `MapCanvas_UI_Test` binary
(`MapCanvas_UI_Test.cpp` + `MapCanvas_Render_UI_Test.cpp` + `MapCanvas_View_UI_Test.cpp` +
`MapCanvas_Picking_UI_Test.cpp`), which stays a fast correctness suite:
- NEW `src/ui/MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp` — `main()`, the scenario loop, the
  basis-tag header print, the three timed operations, the structural sanity `Check()`s (§3 below).
  If synthetic-instance generation grows past the size ceilings (Constitution §1.5: soft 100/hard
  150 lines, functions ≤40 lines), split a `MapCanvas_IconLayer_MicrobenchmarkScenarios_UI_Test.cpp`
  sibling for it — exact split is an implementation call, not locked here, mirroring STEP53 §0's
  own "exact per-file line boundaries are an implementation call" language.
- MODIFIED `CMakeLists.txt` — one new `add_sangen_test(MapCanvas_IconLayer_Microbenchmark_UI_Test
  src/ui/MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp ...)` entry, alongside the existing
  `MapCanvas_UI_Test` block.
- ⚠️ **Open call, flagged not decided here:** whether this binary runs in the default `ctest -C
  Debug` "all green" gate every time, or is opt-in/labeled separately so a synthetic 600k-instance
  run doesn't inflate the normal test-suite wall-clock cost on every CI pass. This is a build
  ownership decision (whoever owns `CMakeLists.txt`/CI conventions), not invented here. Either way,
  this binary asserts **zero** `Check()` failures on timing — only on the structural invariants in
  §3, so it is safe to include in the gate regardless of which way that call goes.

### 2. The three timed operations, at N ∈ {100,000, 300,000, 600,000}, both scenarios
Two visibility scenarios per N, per §14.9's own wording:
- **0%-culled** — the synthetic view rect encloses all N instances (worst case: nothing is culled
  by the AABB/`SpatialGrid` stage, every instance reaches the vertex-write stage).
- **~5%-visible** — the synthetic view rect is sized/positioned against a known synthetic world
  layout so approximately 5% of the N instances survive the cull (typical zoomed-in case).

That is 3 N-values x 2 scenarios x 3 operations = 18 timed measurements minimum, each its own
`std::chrono::steady_clock` start/stop pair, mirroring `Placement_Symmetry_PROC_Test.cpp`'s pattern
exactly (one pair per timed call, not a shared/reused clock):

1. **SIMD world->screen transform + AABB cull + compaction** — build a synthetic instance array
   (positions randomized within a known world bound, seeded deterministically so runs are
   reproducible run-to-run) and drive it through STEP53's **actual, real, landed** §1 candidate-list
   build path (composing STEP47's `WorldToPreviewPixel`, the per-layer AABB test, whatever
   compaction STEP53 shipped) — call the real function, never a hand-rolled stand-in. "SIMD" here
   names what §14.9/STEP53's Backend policy call "the scalar baseline that [this] benchmark will
   compare against" — this ticket does not add a SIMD backend (see Out-of-scope); it measures
   whatever STEP53 actually shipped, scalar or otherwise.
2. **Bulk `ImDrawList::PrimReserve` + raw vertex/index write** — the real `FlushIconLayerBucket`-
   shaped call from STEP53 §3, run inside a headless imgui frame (`ImGui::CreateContext()`/
   `NewFrame()`/`Render()`, mirroring `MapCanvas_Render_UI_Test.cpp`). Start the clock after
   `NewFrame()` returns and stop it before `Render()`, isolating the write loop itself from
   per-frame imgui overhead unrelated to this pass.
3. **Naive per-instance `ImDrawList::AddImage()`** — the explicitly-rejected approach STEP53 §0
   forbids from ever appearing in `MapCanvas_IconLayer_Draw_UI.cpp`. Written **only inside this test
   file**, as a throwaway comparison path, never touching production code — same instance count,
   same headless-frame technique as (2), so the two are an apples-to-apples comparison. This is the
   number that confirms/quantifies the "30-60ms risk" §14.9's Fix section cites as the reason
   `AddImage` is forbidden; it exists to be measured, not adopted.

### 3. Reporting + sanity checks (not the deliverable itself — see Verify)
- One `std::printf` line per measurement: operation name, N, visibility scenario, elapsed ms.
- One basis-tag header block printed once at the top of the run (Constitution §7): machine
  identification, build configuration, run date. Auto-detecting CPU model portably is its own scope
  creep (new dependency for one printed string) — this ticket prints whatever is cheaply available
  (e.g. a `#ifdef NDEBUG` build-config string, the compile date) and leaves the machine-identity
  field as a manually-recorded note by whoever runs it, exactly as this ticket's own basis tag
  below is manually recorded rather than auto-detected.
- Structural `Check()`s (loose, boolean, never gating on elapsed time — mirrors
  `Placement_Symmetry_PROC_Test.cpp`'s own "not a formal benchmark harness" framing):
  - Operation 1's surviving-candidate count at 0%-culled equals N; at ~5%-visible falls within a
    loose `[0, 0.10 * N]` band (a design target, not a guaranteed exact 5%, since the exact count
    depends on the synthetic layout).
  - Operation 2's emitted vertex/index counts equal exactly `quadCount * 4` / `quadCount * 6`
    (mirrors STEP53's own acceptance-test wording — catches an accidental double-reserve or a
    regression toward per-instance calls).
  - Operation 3 draws the same visible-instance count via `AddImage` as operation 2 drew via bulk
    write, at matching N/scenario (keeps the comparison apples-to-apples).
  - None of the above ever assert a threshold on the printed millisecond numbers.

## Files touched
- NEW `src/ui/MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp` (plus an optional
  `..._Scenarios_UI_Test.cpp` sibling if size ceilings force a split — coder's call).
- MODIFIED `CMakeLists.txt` — one new `add_sangen_test(...)` entry.

## Layer & accuracy class
UI, test-only. No shipped production code — this file is never linked into the application binary,
same as every other `_Test.cpp` in this codebase. No production accuracy-class claim applies; the
code under test (STEP53's real functions) keeps whatever accuracy class STEP53 already declared
(Visual).

## Backend policy
CPU-only. This ticket does not add, evaluate, or choose a SIMD/GPU backend for the transform step —
it measures the scalar baseline STEP53 shipped, per STEP53's own Backend policy: "No SIMD/
dispatch-backend decision is in this ticket's scope: Phase 3.3's microbenchmark decides whether the
transform step later gets a SIMD backend; this ticket ships the scalar baseline that benchmark will
compare against." That decision (whether a SIMD backend is warranted) is this ticket's *output*
informing a future ticket, not a change this ticket itself makes.

## ARCH rules invoked
- §14.9 — names this exact benchmark, its N-values, its two visibility scenarios, and states it is
  what turns the placeholder budget into a ratified constant.
- §14.13 item 2 — the open item this ticket's measured numbers resolve (the constant-update itself
  stays a separate follow-up, see Out-of-scope).
- Constitution §7 — the basis-tag law this entire ticket exists to satisfy: STEP53's placeholder
  becomes ratified only once accompanied by a benchmark-backed estimate with a stated basis (this
  ticket's job), not before.
- Constitution §1.5 — size ceilings apply to the new test file(s) same as any other.

## Explicit out-of-scope
- **Updating `OverlayRenderingSettings::visibleInstanceBudget`'s default value itself** (STEP53's
  placeholder, `MapCanvas_IconLayer_UI.h`) — that is a small, separate follow-up ticket this
  ticket's measured numbers feed into. Measuring and ratifying-the-constant are two different
  changes, reviewed separately; this ticket produces the number, it does not edit the constant.
- Adding a SIMD-accelerated transform backend for operation 1 — this ticket measures the existing
  scalar baseline only. If the measured number shows SIMD is needed to comfortably clear the frame
  budget at 600k, that is a new ticket for the Compute Optimization Expert, not invented here.
- A generalized, reusable benchmark framework — scope is this one measurement, using the one
  existing precedent pattern found in §0 (`std::chrono` + `printf` + loose `Check()`); no new test
  infrastructure beyond what these 18 measurements need.
- CPU/machine auto-detection tooling for the basis-tag header — manually recorded by whoever runs
  the benchmark (§3's open call), not new portable-detection code.
- Any change to STEP53's shipped production files (`MapCanvas_IconLayer_Cull_UI.cpp`,
  `..._Budget_UI.cpp`, `..._Draw_UI.cpp`, `..._Cache_UI.cpp`) — this ticket only calls their already
  -landed public surface from a test translation unit. If STEP53's landed API does not expose what
  this benchmark needs to drive operations 1-2 directly, that gap is flagged back to STEP53/ARCH,
  not silently worked around here.
- Whether this binary is included in the default `ctest` gate or run on-demand/labeled separately —
  flagged as an open build-ownership call in §1, not decided here.

## Solution — performance estimate (basis)
This ticket's own solution — the harness — is trivial engineering cost: a few hundred lines of test
code exercising already-shipped STEP53 code paths, no new production code, no new external
dependency. **REASONED-PLACEHOLDER basis tag** (Constitution §7): 18 measurements plus the
synthetic instance-array allocations at up to 600k instances should complete in low seconds of
total wall-clock run time, bounded well under any CI timeout concern given
`Placement_Symmetry_PROC_Test.cpp`'s own precedent (`worstMillis < 5000.0` at far smaller N).
**This estimate is about the cost of running the harness itself** — wholly distinct from the
numbers the harness produces, which are this ticket's actual deliverable and get their own basis
tag (machine, build configuration, run date) recorded at report time, per Constitution §7's "with
basis" requirement.

## Verify
Acceptance test: the new binary builds and runs to completion, emitting the basis-tag header
followed by all 18 measurement lines (operation x N x scenario, wall-clock ms), with **zero**
`Check()` failures on the structural correctness assertions in §3 — never on the timing numbers
themselves (there is no pass/fail bar on speed; the numbers are the deliverable, printed for a
human to read and carry into the separate follow-up ticket that updates STEP53's placeholder
constant). No regression: `MapCanvas_UI_Test` (the existing correctness binary) stays green,
unmodified beyond whatever STEP53 itself already required — this ticket adds a new, separate
binary, it does not touch the existing one.
