# Work-Order M0-7 — `ThreadPool_SYS` (persistent workers + parallel-for)

*Schema-valid per Constitution §7. Milestone M0 (Foundation) — SYS group. Executor:
SanGen Coder. Status: implemented + verified (ALL PASS, ThreadSanitizer clean).*

## Title
Persistent worker pool with a blocking `ParallelFor`.

## Root problem
The generation passes spin up threads per call (`Gen_Noise` uses `std::future`/`thread`
per chunk). Repeated thread creation/teardown is costly and unpredictable. A persistent
pool reused across passes removes that overhead.

## Target files
- `src/sys/ThreadPool_SYS.h`
- `src/sys/ThreadPool_SYS_Test.cpp`

## Layer & accuracy class
`SYS`. Accuracy N/A. Chunk partition depends only on `(range, workerCount)`, not on
runtime scheduling — safe for the deterministic path when the per-index work is
independent (no cross-index reduction).

## Backend policy
CPU. Pool size 0 → sized to `hardware_concurrency`; a tiny range or empty pool runs
inline on the caller.

## ARCH rules invoked
- §1.1 fully-spelled names; §1.2 `_SYS` suffix; §1.5 ceiling (95-line header).
- Optimization pillars (fixed thread partitioning; `DETERMINISM_SPEC` ordered/stable
  partitioning).

## Solution
N persistent workers drain a task queue (mutex + condition variable). `ParallelFor`
splits `[begin,end)` into `workerCount` fixed chunks, enqueues them, and blocks until
all complete. Completion is a `remaining` counter **guarded entirely by the done-mutex**,
with the notify performed under that lock.

## Concurrency-safety note (caught in verification)
The first cut decremented `remaining` atomically and only then locked the mutex to
notify — a notify-after-decrement race where the waiter could wake and destroy the
stack-local mutex/condition-variable before the notifying worker finished touching them.
**ThreadSanitizer flagged it**; fixed by guarding `remaining` under the done-mutex so the
notify completes before the waiter can reacquire and return.

## Performance estimate (with basis)
Eliminates per-call thread creation (~tens of microseconds per thread); for a pipeline
of many small parallel passes this is the dominant saving. `ParallelFor` dispatch is a
handful of enqueues + one wait (*basis: thread-create cost vs enqueue cost; rough-
estimate*).

## Lossy alternative
None.

## Acceptance test (`ThreadPool_SYS_Test.cpp`) — PASSED
100k-element parallel write is correct; every index of a 50k range is visited exactly
once (atomic counter); a pool is reused 200× with correct results; empty / single-
element / inline edge cases hold. **Verified in sandbox: correctness ALL PASS and
ThreadSanitizer ALL PASS (no data races).**

## Out of scope (explicit)
- Work-stealing / dynamic load balancing beyond even chunks.
- A general `Enqueue`-returning-`future` async API (add when a pass needs fire-and-
  forget).
- Binding a per-worker `ArenaAllocator` (compose at the call site or a later helper).
- NUMA affinity / thread pinning.

## Open (from M0-1, still pending)
Test-file convention not yet ratified in the ARCH — route to ARCH Expert.
