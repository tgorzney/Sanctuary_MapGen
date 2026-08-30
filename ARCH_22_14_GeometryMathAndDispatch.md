[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.14. **Only the ARCH Expert writes this file.**

### 22.14 Geometry MATH placement and CPU-only dispatch policy — ratifies the Compute Optimization Expert's advisory

Ratifies `work_orders/DESIGN_NavmeshBlockerGeometryMath_R1.md` in full and performs the one ruling
that document explicitly stated was "out of my authority."

**1. MATH placement confirmed as designed.** Extend `src/math/RigidTransformPivot_MATH.h` with
`RotateVectorByQuaternion`/`TransformPointByRigidTransform`/`InverseTransformPointByRigidTransform`/
`InverseTransformPlaneByRigidTransform`; new `src/math/TrianglePlaneIntersection_MATH.h`
(`ClassifyTriangleAgainstPlane`/`TrianglePlaneIntersectionSegment`). Both are zero-`Params::`-in-
signature, matching §3.5's mechanical MATH/PARAMS/PROC placement rule exactly — no judgment call
needed; the design's own reasoning already applies that rule correctly.

**2. The non-uniform-scale plane-transform correctness nuance — RULED: implement the general
(inverse-transpose) form unconditionally**, not gated behind first confirming whether placed props
ever carry non-uniform scale (the Format doc's own open Q7). The general form costs nothing extra
per the Compute doc's own analysis, and both Constitution §6's "never trust structure blindly" and
this pack's standing aversion to silent-correctness traps behind an unconfirmed assumption argue for
the unconditional general path over a conditional fast path gated on an unverified premise.

**3. Dispatch policy RULED: CPU-only, no GPU dispatch, for the new mask-generation PROC stage(s)**
(`ARCH_22_12_MaskGenerationAlgorithmAndScope.md`). This adopts the Compute doc's own reasoning as
binding law, not merely advisory: a one-shot batch workload at author/export time, not a per-frame
or resident-grid pass; embarrassingly parallel per-instance (`OPTIMIZATION_PILLARS` pillar 12 —
thread-pool partitioning is the correct first lever); upload/readback/shader-setup overhead would
plausibly dominate for any realistic per-map instance count given the workload's size and frequency.
**This is a rough-estimate basis (Constitution §7), not a benchmark** — a future measured regression
toward GPU dispatch, if real per-map instance/triangle counts ever justify it, needs its own
benchmark-backed proposal, not an assumption carried forward from this ruling.

**4. Determinism finding confirmed, corroborating rather than duplicating
`ARCH_22_13_BakedArtifactStorageAndDeterminism.md` point 2.** Standard `std::sqrt`/hardware FMA and
ordinary transcendentals are legal in these new MATH primitives; no portable-minimax-transcendental
tax applies, because the feature never enters the Deterministic regeneration surface at all
(§22.13's own, more detailed ruling is the binding one; this point records that the Compute doc
reached the same conclusion independently, from a different angle, before §22.13 settled it).

**5. Recorded, not fixed by this ruling.** `sangen_arch_pack/specs/MATH_SIMD_SPEC.md` still
describes the retired `core/math/Sanmath_*.h` stub files, not the current `src/math/*_MATH.h` set —
a real staleness the Compute doc flagged in passing, out of scope for this ratification. A future
housekeeping pass should refresh it.
