[← ARCH index](ARCH.md) · SanGen ARCH §4. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 4. Dispatch contract (Constitution §4, resolved)

Replaces every ad-hoc `UseGPUx` bool. `PIPELINE` sets a `DispatchPolicy` per stage;
`Dispatch_SYS` reads it and runs the kernel on the resolved backend.

### 4.1 The policy object
```
enum class ComputeBackend    { Cpu, Gpu, Automatic }
enum class GenerationContext { Preview, Output }
enum class AccuracyClass     { Exact, Accurate, Visual }

struct DispatchPolicy {
    ComputeBackend previewBackend;
    ComputeBackend outputBackend;
    AccuracyClass  previewAccuracy;
    AccuracyClass  outputAccuracy;
    bool           bDeterministic;   // forces Cpu + portable transcendentals + ordered reductions
}
```
`Cpu`/`Gpu` are kept as standard acronyms (naming §1.1).

### 4.2 Per-stage defaults (Preview → Output)
| Stage | Preview | Output |
| --- | --- | --- |
| Noise · Blend · Mask · Thermal | Gpu / Visual | Cpu / Accurate |
| Erosion · Flow / Accumulation | Gpu / Visual | **Cpu / Exact** (shapes terrain + pathing) |
| Placement | Cpu / Accurate | **Cpu / Exact** (spacing, markers) |
| Bake · Albedo · preview color | Gpu / Visual | Gpu / Visual (decorative, determinism-exempt) |

These are defaults; §8 tweakability lets any stage be overridden per project. In the
Output context they are further constrained by §4.6.

### 4.3 Backend resolution (how `Dispatch_SYS` picks)
1. `bDeterministic` set and the stage is in the **Exact chain** (§4.6) → **Cpu**,
   portable transcendental + ordered-reduction path (`DETERMINISM_SPEC`).
2. else the stage's `previewBackend`/`outputBackend` for the active context.
3. else the global backend setting.
4. `Automatic` → fastest **legal** backend for the declared accuracy class given data
   residency (no needless Cpu↔Gpu copies). Same accuracy class ⇒ backends must agree
   within that class's tolerance; a backend that cannot meet the class is not legal.

### 4.4 Idle escalation (Preview context)
During interaction the preview runs its fast path (Gpu/Visual). When input goes idle,
`PIPELINE` re-runs the affected stages at **Output** accuracy and swaps the result in —
so scrubbing stays fast but the settled image is truth. WYSIWYG holds because the
preview *samples* that bake; it never re-simulates (§3.2). Idle escalation is only
sound because stages are re-runnable (§3.4.2).

### 4.5 Determinism scope
`bDeterministic` makes only the **Exact-class, gameplay-authoritative** outputs
bit-identical across machines (heightmap incl. erosion, flow, placement/markers,
collidable props). Visual-class outputs stay on the fast Gpu path and may differ per
machine. Experimental until the cross-machine bit-exact gate passes (`DETERMINISM_SPEC`).

### 4.6 Exact-chain closure (Output context only)
An Exact result cannot be produced from an input that was computed non-reproducibly.
Therefore, in the **Output** context:

> The **Exact chain** is the transitive set of stages whose output is read — directly or
> through intermediate stages — by any stage declared **Exact**. Every stage in the Exact
> chain is dispatched on the backend and code path its Exact consumer requires. When
> `bDeterministic` is set, the **whole Exact chain** runs Cpu / portable / ordered — not
> only the stages whose own declared class is Exact.

- A stage's declared `AccuracyClass` still describes **its own product**. Exact-chain
  membership is a **dispatch** property that `PIPELINE` computes from the DAG; no stage
  computes it, and no stage hardcodes it.
- **Visual-only consumers never pull a producer into the chain.** Bake reads everything
  and is Visual, so it constrains nothing.
- **Preview has no Exact chain.** The preview product is Visual by definition and every
  stage takes its fast path; the guarantee is honored on the §4.4 idle escalation to
  Output.
- Practical effect on §4.2: in Output, everything except Bake is already on `Cpu`, so
  this rule costs **no backend changes** — it only tightens the class label (and thus the
  code path selected under `bDeterministic`) for Noise/Blend, Mask, and Thermal, which
  all feed Exact consumers.
- An edge may be exempted (producer stays lax for one specific consumer) only via a
  documented **work-order exception** (Constitution §7) proving that consumer's Exact
  decision does not depend on that input's exactness.
