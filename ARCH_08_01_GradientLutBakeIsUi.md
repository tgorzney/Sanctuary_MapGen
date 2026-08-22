[← ARCH index](ARCH.md) · [§8 ARCH_08_M4Resolutions](ARCH_08_M4Resolutions.md) · SanGen ARCH §8.1. **Only the ARCH Expert writes this file.**

### 8.1 The gradient LUT bake is `UI`, not `PROC` (corrects §5.4)
**Ruling: the color-ramp LUT bake lives at `src/ui/GradientLut_UI.h/.cpp`.
`Gradient_PROC` is retired as a name and never existed as a file.**

§5.4's original "gradient LUT bake → `Gradient_PROC`" was written before §6.1 and §7.4
existed, and it is wrong under both:

- **PROC has a definition of done that a color ramp cannot satisfy.** §6.1 requires every
  PROC unit to be a CPU + GPU pair, parity-checked, wired into `PIPELINE` + `Dispatch_SYS`,
  with a declared accuracy class and declared DATA inputs/outputs. A 256-entry color LUT
  has no GPU twin worth writing, no DAG node, and produces no DATA field any stage reads.
- **It is not a pipeline stage.** §7.4 enumerates the stage order exhaustively; a color
  ramp is not in it and must not be smuggled in.
- **It is presentation, by the layer definitions.** Constitution §1: PROC is *applied
  processors* (terrain synthesis, erosion, masking, placement); UI *"composites/samples
  baked results."* Turning a designer-chosen ramp into a sampled table is colorization —
  the definitional UI job.
- **§3.2's "UI never simulates" is not violated.** That rule forbids the UI re-deriving a
  *simulated quantity* (slope, flow, rule filtering) — the shadow-sim, hit-list #4. A LUT
  is built from PARAMS alone; it reads no DATA field and duplicates no stage. Building a
  presentation resource from settings is not simulating.
- **Direction check (§3.1).** UI → PARAMS is legal and downward. The reverse would not be:
  nothing in PROC may include a `_UI` header, so if a future **bake/export** path ever
  needs to sample a designer ramp, the LUT builder moves to `MATH` (a pure ramp→table
  function, `GradientLut_MATH`) and UI calls it there. It does **not** become PROC. As of
  today no PROC stage consumes a color ramp (Bake composites stratum art, not ramps), so
  `UI` is correct and is the least-privilege home.

`PreviewComposite_UI` (M4-3) owns the GPU upload of the baked LUT via `GpuResource_SYS`;
`GradientLut_UI` stays pure CPU and sandbox-testable with no GL.

