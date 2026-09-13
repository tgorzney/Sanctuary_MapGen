[← ARCH index](ARCH.md) · [§23 ARCH_23_ProceduralStratumMaskFilterChain](ARCH_23_ProceduralStratumMaskFilterChain.md) · SanGen ARCH §23.4. **Only the ARCH Expert writes this file.**

### 23.4 `ImportedMaskMode` stays a fixed trailing step, outside the filter chain — CONFIRMED

**Confirmed as proposed, no correction.** The design doc's §2 defaults to keeping
`ImportedMaskMode`'s stored-art merge (`MASKING_SPEC.md` §1.5: `Disabled`/`ProceduralStart`/
`StaticOverride`) as a fixed step that always runs *after* the whole `maskFilters` chain, rather
than becoming expressible as one of the chain's own blend-mode entries. This is architecturally
correct, for the same reason `MASKING_SPEC.md` §1.2 already gives for why Mask "does the multiply
and emits the combined field directly" instead of emitting a bare gate: `ImportedMaskMode` is not
a multiplicative/blend operation in the general sense the chain's `Math::BlendMode` vocabulary
covers.

- `StaticOverride` **replaces** the procedural result outright and is explicitly **not
  slope-gated** — `MASKING_SPEC.md` §1.5's own words, restated as a pinned invariant in
  `Stratum_PARAMS.h`'s header comment. If `StaticOverride` became one more step inside
  `maskFilters`, its position relative to the rest of the chain would become author-orderable —
  and a chain author could then insert filters *after* a `StaticOverride` step, silently
  re-gating art the invariant says must never be gated. Keeping the merge as a fixed step that
  always runs last, structurally, is what makes "not slope-gated" a guarantee rather than a
  convention an author could accidentally violate by reordering.
- `ProceduralStart`'s additive semantics (`clamp(procedural + stored)`) already assumes
  "procedural" means the *fully resolved* output of everything upstream of the merge — i.e. the
  entire chain's folded result, not an arbitrary intermediate step. Splicing the merge into the
  middle of the chain would make "procedural" mean something different depending on where in the
  chain the splice landed, which has no sensible reading.

**Binding shape, unchanged from `MASKING_SPEC.md` §1.2/§1.3, now restated against the chain:**
```
running       = materialProportions[s]
for each enabled filter in stratum.maskFilters (left to right):
    running = <fold step, §23.5>
procedural_s  = running                              // the chain's final output
surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)   // fixed, last
```
Mask remains pure and idempotent under this shape (ARCH §3.4.2, `MASKING_SPEC.md` §1.3 item 2):
`Merge` is still a pure function of `procedural_s`, the stored-art `Data::FloatField`, and
`importedMaskMode_s`, run exactly once, in exactly one place, with no cross-stratum reads.
