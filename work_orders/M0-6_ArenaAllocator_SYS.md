# Work-Order M0-6 — `ArenaAllocator_SYS` (linear bump allocator)

*Schema-valid per Constitution §7. Milestone M0 (Foundation) — SYS group. Executor:
SanGen Coder. Status: implemented + verified (ALL PASS under ASan/UBSan).*

## Title
Linear "bump" scratch allocator with O(1) allocate and whole-arena reset.

## Root problem
The hot generation passes churn transient scratch buffers (per-cell/per-layer
temporaries, thread-local working sets), thrashing the general allocator. The
optimization pillars call for an **arena + thread-local buffers**; none exists today.

## Target files
- `src/sys/ArenaAllocator_SYS.h`
- `src/sys/ArenaAllocator_SYS_Test.cpp`

## Layer & accuracy class
`SYS`. Accuracy N/A (memory primitive).

## Backend policy
CPU. **Not thread-safe by design** — each worker gets its own arena (pairs with the
thread pool, next work-order). Never grows, never throws.

## ARCH rules invoked
- §1.1 fully-spelled names (`Allocate`, `usedOffset`, `alignedBase`).
- §1.2 `_SYS` suffix, `src/sys/`.
- §1.5 ceiling — 71-line header.
- Optimization pillars (arena / thread-local buffers).

## Solution
Reserve one contiguous block up front (over-allocated by 64 bytes and aligned to a
64-byte boundary). `Allocate(size, alignment)` rounds the offset up to a power-of-two
alignment (≤64) and bumps it; returns `nullptr` if it would exceed capacity (offset
left unchanged on failure). `AllocateArray<T>(count)` is the typed convenience.
`Reset()` reclaims the whole arena in O(1). Move-only (copy deleted) so ownership is
unambiguous.

## Performance estimate (with basis)
`Allocate` is a mask + add + compare (a handful of instructions); `Reset` is a single
store. Orders of magnitude cheaper than `new`/`delete` for many small transient
allocations, and keeps scratch contiguous for cache locality (*basis: instruction
count; rough-estimate*).

## Lossy alternative
None (memory primitive).

## Acceptance test (`ArenaAllocator_SYS_Test.cpp`) — PASSED
Alignment honored (32-byte requests are 32-aligned); allocations don't overlap; typed
`float[16]` is writable; over-capacity returns `nullptr` and leaves `BytesUsed()`
unchanged; `Reset()` reclaims; exact-fill of 128 then one more byte returns `nullptr`.
**Verified in sandbox under AddressSanitizer + UBSan: ALL PASS, no memory/UB errors.**

## Out of scope (explicit)
- Thread-safe / concurrent arena (per-worker arenas instead).
- Growable arena and a scoped marker/rewind sub-allocation API (add only if a pass
  needs them).
- `ThreadPool_SYS` that will own the per-thread arenas — next work-order.

## Open (from M0-1, still pending)
Test-file convention not yet ratified in the ARCH — route to ARCH Expert.
