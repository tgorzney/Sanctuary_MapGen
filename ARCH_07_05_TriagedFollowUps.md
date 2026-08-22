[← ARCH index](ARCH.md) · [§7 ARCH_07_M3Resolutions](ARCH_07_M3Resolutions.md) · SanGen ARCH §7.5. **Only the ARCH Expert writes this file.**

### 7.5 Triaged follow-ups — NOT in scope for the M3-2/M3-8 rework
Two defects were raised alongside §7.2. Both are real; neither is resolvable inside the
Mask/stage-order rework, and neither blocks it. They are recorded here so they are not
lost and so no coder "fixes" them opportunistically.

**(a) Volume fraction is not surface exposure.** `Erosion_Field_PROC.cpp::WriteThicknessToFields`
writes `ticks / totalTicks` — a **volume fraction** — while `LAYER_SYSTEM_SPEC` defines the
exported stratum mask as **surface exposure** ("topmost layer with thickness > 0, soft
blend"). These are different quantities: thin topsoil over deep bedrock reads ~0% under
volume fraction but should visually cover the surface.
*Reframing under §7.2:* this is **no longer a defect in Erosion.** Volume fraction is
exactly what `materialProportions` is now defined to mean, so Erosion's write-back is
correct. What is *missing* is the volume → surface-exposure derivation, and its home is the
Mask stage (the stage that turns physical proportion into visible weight).

**(b) The thickness stack is not persistent DATA.** Each sim stage reconstructs
`thickness = height × proportion` on entry and collapses back to a proportion on exit,
which discards buried stratigraphy across stage boundaries. `FUTURE_SIM_TYPES_SPEC` expects
sims to consume ordered thickness columns as their native representation.

**These two are one problem.** Surface exposure cannot be derived from proportions alone,
because a proportion vector carries no stratigraphic **order** — you cannot know which
stratum is on top. (a) is therefore blocked on (b): the ordered thickness column must be
persistent DATA before true surface exposure can be computed.

**Ruling — deferred to its own ARCH ruling and work order, in M6 (§6):**
- Do **not** attempt either fix in the M3-2/M3-8 rework.
- Until then, the Mask stage consumes `materialProportions` as its exposure approximation,
  and this approximation is documented at the call site as such.
- **The seam is safe to build against now.** When the thickness stack lands, Mask's input
  changes from "the proportion field" to "the surface-exposure field derived from the
  stack" — same shape (9 × `FloatField`, 0..1), same consumer, same kernel. The Mask
  kernel does not change; only its input binding does.
- Anyone raising this again: it needs a DATA-shape ruling (ordered thickness columns:
  layout, fixed-point width, memory cost at 4096², and which stage owns the stack across
  stage boundaries), not a patch.

