# LAYER_SYSTEM_SPEC — the height/material layer & simulation system

The intended v2 design of SanGen's terrain layer system, authored with the owner.
Goal: a **non-destructive creative workflow** combined with **physically-correct**
procedural generation. This supersedes how the current (mangled) code models
layers; it is the design the ARCH and work-orders should target.

## Core idea: two representations
- **Author in height** (creative, non-destructive): designers build with height
  layers + blend modes + Min/Max envelopes.
- **Simulate in thickness** (physically correct): before sims run, the authored
  stack is baked into **per-material thickness columns**; simulations move
  material (mass-conserving); the result is read back as final height + stratum
  masks.

## The material palette (unchanged — keep as-is)
9 stratums; each **stratum index** owns its identity: textures (albedo/normal/
mask), the shader height, and **soil physics** (hardness, friction, cohesion,
capacityMult, absorptionRate) — today's `StratumSettings` keyed by index. A
material layer points at a stratum index and inherits all of it. Sims look up
soil physics by a column's current stratum. **Do not change this.** (In v2 this is
the one `Params::Stratum` type — ARCH §7.1.)

## Layer hierarchy
- **GeoLayer** = a group/container. Owns its own **blend mode** and **erode-below**
  option, and a **mode** (see combine, below). Each GeoLayer's *height field* is
  computed independently (cheap, parallelizable), then GeoLayers apply **in
  order** onto the shared material stack.
- Inside a GeoLayer, an ordered stack of **two layer types**:
  1. **Material layer** (design/author) — a height source: procedural noise OR
     imported texture (imported = levels/contrast/brightness + blend + Min/Max
     only). Assigned a StratumIndex (its material). **Min/Max = thickness
     envelope** (floor/cap on how much of this material exists) *and* the initial
     distribution mask. Has a blend mode. Can be **baked/unbaked**.
  2. **Simulation layer** — a single physics pass that runs on the accumulated
     stack *beneath it*. It adds no material; it moves/erodes what is already
     there, reading each column's per-material soil physics. **One sim layer per
     sim type** (hydraulic droplet, thermal/talus, fluvial/river, glacial,
     snow-melt), added in any order. Order matters (bedrock → thermal → dirt →
     hydraulic → snow → snow-melt). Differential hardness is automatic (soft
     erodes fast, hard slow → spires emerge, sediment piles against them).

  *v2 change:* erosion moves OUT of the material layer (today `NoiseLayer.Erosion`)
  and becomes its own Simulation layer type. A single sim pass acts across all
  materials beneath it (a river cuts dirt AND the rock under it).

## Volume / final height (additive-thickness model)
- Each material has a **thickness field** (≥0) per pixel; the sim maintains and
  moves it (mass-conserving).
- Final height `H(x) = bedrock_base(x) + Σ thickness_i(x)`.
- Erosion subtracts thickness from the top material (rate ∝ erodibility of that
  material) and deposits it elsewhere as sediment. Thickness→0 exposes the layer
  below. This is what makes "erode dirt, reveal rock, pile sediment on the spire"
  real physics rather than a trick.

## Baking
Flatten anything — one layer, a GeoLayer, or the whole stack — into a single
imported-equivalent layer (a height field + its baked material). Uses: a complex
build as the **unmasked layer-0 base**; a **frozen intermediate base** so
expensive sims don't recompute each frame; iteration speed (freeze → tweak cheap
layers → unfreeze). A baked layer is just an image (cheap to reuse). An imported
texture is simply a pre-baked material layer.

## GeoLayers: composition bands & sim mode
GeoLayers are **composition / feature bands** and a first-class part of the
workflow (e.g. band 1 = seafloor→sea level; band 2 = plains/hills masked to start
at band 1's max height; band 3 = mountains/valleys). Each is a full material stack
+ sims, composited with its own blend mode + mask, and can **derive from the band
below** (duplicate + tweak). Payoff: **change the seed → a whole new map that keeps
the same structure** (river/mountain style preserved), because composition and
relationships stay fixed and only the noise re-rolls. Bands can also be fully
independent for more randomness.

**Per-GeoLayer role:**
- **Material** — owns material columns; contributes strata.
- **Shaper** — height-only: *Subtract/lower* shaves the column top (exposes deeper
  strata); *Add/raise* deposits to the new height (added stratum = the GeoLayer's
  material, or base).

**The exposure rule is unified.** Whether layers are inside one GeoLayer or across
GeoLayers, the surface material at a pixel = the topmost material by height, soft
transitions. So stratum masks fall out the same way at both levels — there is no
separate "merge algorithm."

**Design rule:** materials that must **erode and reveal each other** (dirt→rock
spire) go in the **same GeoLayer's stack**; use **separate GeoLayers** for
**composition bands / seed-stable structure**.

**Sim mode (global toggle):**
- **Separate** (default) — each GeoLayer sims on its own stack, then composite.
  Fast, parallel, bands independent. No cross-band erosion (an upper band can't dig
  into a lower band's buried material).
- **Unified** — all Material GeoLayers flatten into one combined column stack, then
  the sim layers run across the whole thing. Physically-correct cross-band erosion
  and reveal; heavier, couples bands, less parallel. (This is what the old
  "multi-Material merge" question resolves to — an opt-in mode, not an always-on.)
- Natural workflow: iterate in **Separate** (responsive preview), final-bake in
  **Unified** (accurate output) — maps onto the Preview vs Output contexts (§9).
- Middle ground, only if ever needed: a per-GeoLayer **"sim group" tag** (GeoLayers
  sharing a tag unify together) — a flat field, **not** a new grouping tier.

## Stratum mask output
- After sims, each pixel's surface material = topmost layer with thickness > 0;
  transitions are a **soft blend**, not a hard pick. This is **surface exposure**.
- Each stratum's mask channel = summed surface-exposure of every layer sharing
  that StratumIndex. **Stratum 0 = the always-present base (no mask).** Strata
  1–8 get masks.
- **On-disk format** (from the game/resources): heightmap `heightmap.raw` 16-bit
  grayscale `(N+1)²`; masks in two 32-bit RGBA files — `stratums_1_4.tga`
  (R=1,G=2,B=3,A=4) and `stratums_5_8.tga` (R=5…A=8), ~2× map res (≤4096). These
  are the code's `PendingSplat14Path` / `PendingSplat58Path`.
- **Shader relationship:** each stratum has a height channel; the shader does
  height-blending (`stratumHeight × mask`, higher stratum index minus lower) to
  sharpen transitions. So the mask says "how much this material shows"; the shader
  decides the crisp boundary.

### Where this lands in the v2 field model (ARCH §7.2)
The exported stratum masks are **surface weights**, and they are produced by the
**Mask stage**, which writes `surfaceStratumWeights[0..8]`. The physical field the sims
own is `materialProportions[0..8]` — how much of each stratum is in the column. These are
two different DATA fields with two different single writers; see `MASKING_SPEC` Part 1.

> **Known gap — surface exposure is not yet implemented (ARCH §7.5).** Today the sims
> collapse the thickness stack back to a **volume fraction** (`ticks / totalTicks`) on exit
> and the Mask stage consumes that as a stand-in for exposure. Volume fraction ≠ surface
> exposure: thin topsoil over deep bedrock reads ~0% coverage under volume fraction when it
> should visually cover the surface. The true derivation needs the **ordered** thickness
> column — a proportion vector carries no stratigraphic order, so you cannot tell which
> stratum is on top — and the stack is currently reconstructed and discarded at each stage
> boundary rather than persisted as DATA. Both are deferred to a single M6 DATA-shape
> ruling + work order; do not patch either inside a mask or sim work-order. The interface
> is stable across the fix (9 × `FloatField`, 0..1), so the Mask kernel will not change —
> only its input binding.

## `HeightmapStack` IO shape (companion note, work-order SPEC-4)
`.sanmap` schema v3's `HeightmapStack` (`SANMAP_FORMAT_SPEC` Correction 3) round-trips
the model on this page **exactly as designed — confirmed, not redesigned**: a flat,
ordered `LayerStack` of `GeoLayer` (composition bands), each a flat, ordered stack of
`Layer` (Material or Simulation type). **Neither level nests or groups.** `GeoLayer`
cannot currently contain another `GeoLayer` — the recursive-`GeoLayer` grouping this
page's "GeoLayers: composition bands & sim mode" section gestures toward (arbitrary
nesting) is a **future, separate** internal redesign, out of scope for `SPEC-4` and
unbuilt today. `SimulationGrouping` (the Separate/Unified sim-mode toggle, "Sim mode"
above) nests inside `HeightmapStack` in the schema, matching this page's model of it as
a `LayerStack`-level setting, not a per-`GeoLayer` one.

**Named gap, carried forward unfixed by SPEC-4:** a real shipped map's
`GeoLayers.Layers[]` carries per-layer `MinHeight`/`MaxHeight`/`MinSlope`/`MaxSlope`
gates. v2's current `Layer` PARAMS has **no equivalent field** — dropped in the v1→v2
port, not merely unbuilt. `SPEC-4` carries v2's current `Layer` field set through the
schema unchanged and defers the fix to the same future internal-layer-redesign
conversation named above.

**New, in scope now (SPEC-4 Correction 4):** `GeoLayer` and `Layer` each gain a local
`bSymmetryUseGlobal` + `symmetryMask` override — the same override pattern already live
on every placement rule type (`PLACEMENT_SCATTER_SPEC`). This is additive to the model
on this page and does not change layer/stack structure.

**Forward reference:** when the recursive-`GeoLayer` grouping design lands, it must
extend `HeightmapStack`'s IO shape to match — a versioned schema change (a new
`SanGenVersion`), not a silent reinterpretation of the existing flat shape.

## Open / advanced items
- `tint_geometry.tga` channel layout — the resource is login-walled; owner to
  supply.
- The exact stratum assigned by an Add/raise Shaper (contributing layer's
  material vs base) — confirm.
- **Persistent ordered thickness columns as DATA** (layout, fixed-point width, memory
  cost at 4096², ownership across stage boundaries) + the surface-exposure derivation
  built on them — ARCH §7.5, scheduled M6. `FUTURE_SIM_TYPES_SPEC` depends on this.
- *(Resolved: multi-Material-GeoLayer combining = the Unified sim-mode toggle above.)*
- **The shared Group/Layer/LayerType hierarchy for the four scatter Stacks**
  (`MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack`) is a *separate* deferred
  design from the recursive-`GeoLayer` grouping above — `SANMAP_FORMAT_SPEC`
  Correction 7 flags that `HeightmapStack`'s shape (flat, physics-ordered, exactly
  two layer types) is likely not the same DATA shape as those four Stacks'
  (organizational, arbitrary grouping). Do not conflate the two future designs.
