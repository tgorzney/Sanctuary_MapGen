# Work-Order M0-8 — `Dispatch_SYS` (dispatch contract + backend resolution)

*Schema-valid per Constitution §7. Milestone M0 (Foundation) — SYS group. Executor:
SanGen Coder. Status: implemented + verified (ALL PASS, warning-clean).*

## Title
The single CPU/GPU dispatch vocabulary, backend resolver, and router seam.

## Root problem
Backend selection today is N ad-hoc `UseGPUx` booleans branched at scattered call
sites, some orphaned, some silently no-op. ARCH §4 mandates one `DispatchPolicy` and one
resolution rule that every stage goes through — no stage reads a raw toggle.

## Target files
- `src/sys/Dispatch_SYS.h`
- `src/sys/Dispatch_SYS_Test.cpp`

## Layer & accuracy class
`SYS`. Pure logic (this *is* the backend selector).

## Backend policy
N/A — the mechanism itself.

## ARCH rules invoked
- §1.1 fully-spelled names; §1.2 `_SYS` suffix; §1.5 ceiling (60-line header).
- §4 the whole dispatch contract (enums, `DispatchPolicy`, resolution order).
- §3 SYS knows *how* to run a kernel, not *which* stages exist — the per-stage default
  policies are built by PIPELINE, not here.

## Solution
Defines `ComputeBackend {Cpu,Gpu,Automatic}`, `GenerationContext {Preview,Output}`,
`AccuracyClass {Exact,Accurate,Visual}`, `DataResidency {OnCpu,OnGpu,Either}`, the
`DispatchPolicy` struct, and:
- `ResolveBackend(policy, context, globalDefault, residency)` implementing §4.3:
  (1) deterministic + Exact → Cpu; (2) the stage's per-context backend; (3) Automatic →
  global; (4) still Automatic → residency (Cpu if OnCpu, else Gpu).
- `Dispatch(kernel, policy, context, globalDefault, residency)` — the templated router
  seam: resolve, then call `kernel.RunOnCpu()` / `RunOnGpu()`; returns the backend used.

## Performance estimate (with basis)
Resolution is a handful of enum comparisons — negligible per dispatch; the win is
architectural (one seam, no scattered toggles) (*basis: op count*).

## Lossy alternative
N/A.

## Acceptance test (`Dispatch_SYS_Test.cpp`) — PASSED
Full resolution matrix: defaults (Preview→Gpu, Output→Cpu); deterministic forces Cpu for
an Exact output stage even with `outputBackend=Gpu`; a Visual stage is exempt; Automatic
stage falls to the global default; Automatic+Automatic resolves by residency
(OnCpu→Cpu, OnGpu/Either→Gpu). The router runs the resolved backend exactly once (mock
kernel). **Verified in sandbox: ALL PASS, clean under `-Wall -Wextra`.**

## Out of scope (explicit)
- The actual GL kernel execution + resource management → `GpuResource_SYS` (next; the
  concrete `Kernel` with a GPU `RunOnGpu()` lives there).
- Per-stage default `DispatchPolicy` construction → PIPELINE.
- Runtime CPU-feature detection for backend legality (add when a backend can be
  genuinely unavailable at runtime).

## Open (from M0-1, still pending)
Test-file convention not yet ratified in the ARCH — route to ARCH Expert.
