[← ARCH index](ARCH.md) · [§23 ARCH_23_ProceduralStratumMaskFilterChain](ARCH_23_ProceduralStratumMaskFilterChain.md) · SanGen ARCH §23.2. **Only the ARCH Expert writes this file.**

### 23.2 `BlendMode` relocates to MATH; `Ui::PreviewBlendMode` becomes an alias

**Ruled: relocate, don't duplicate, don't leave in UI.** `PreviewBlendMode`
(`PreviewComposite_Settings_UI.h:35-38`) is a `Ui`-layer type today; `PROC` may never depend on
`UI` (§3.1's dependency table — the reverse direction, `UI → PROC`, is also forbidden, but a
`PROC`-owned Mask stage `#include`-ing a `_UI.h` header is the specific violation at issue here).
Reusing the vocabulary is exactly right (the design doc's own framing); reusing the header is not
legal. Of the three options the design doc posed (relocate, mirror two independently-defined
enums, or something else), **relocate** is correct, not mirror: a hand-kept-in-sync duplicate
enum is precisely the "two rival structs carrying the same shape" failure mode `MASKING_SPEC.md`
Part 2 already names as a real, shipped defect class (the double-remap history) — this pack does
not repeat that shape for a brand-new feature when a clean relocation is available.

**Where it goes: `MATH`, not a new shared PARAMS/PIPELINE type.** The relevant arithmetic already
lives in `PreviewComposite_Color_UI.h`'s `CombineChannel` — and that function's own header comment
already states it is "pure functions of already-sampled values: no DATA, no GL, no field sampling."
It takes two `float`s and an enum and returns a `float`; nothing about it is UI-specific, it is
simply homed in a UI file for historical reasons (built as part of the preview compositor before
any other consumer existed). Constitution §1 defines `MATH` as "pure, stateless math"; `CombineChannel`
is exactly that, and ARCH §3.5's signature test (zero `Params::`-typed parameters, zero `Data::`-
typed parameters) confirms it: `float CombineChannel(float destination, float source, BlendMode
blendMode)` closes over nothing but its own arguments.

**Binding shape for the coder ticket:**
- New file `src/math/BlendMode_MATH.h`:
  - `enum class BlendMode { Replace, AlphaBlend, Add, Multiply, Maximum, Minimum, Subtract,
    Divide, Overlay, Screen, SoftLight, HardLight };` (moved verbatim, same order — order is
    load-bearing per the existing GPU `#define` generation, §1 below).
  - `enum : int { kBlendModeCount = 12 };`
  - `inline float CombineChannel(float destination, float source, BlendMode blendMode) { ... }`
    — the function body moves verbatim from `PreviewComposite_Color_UI.h`. Keeping the name
    `CombineChannel` (not renaming to something color-agnostic) preserves ARCH §1.4's CPU/GPU
    kernel-pairing-by-shared-name convention against the existing `.glsl` twin
    (`combineChannel` in `PreviewComposite_Color_UI.glsl`) — a pure relocation, not a rewrite.
- `PreviewComposite_Settings_UI.h`: `PreviewBlendMode` becomes `using PreviewBlendMode =
  Math::BlendMode;` (a type alias, not a new enum) — every existing call site
  (`PreviewBlendMode::Add`, `kPreviewBlendModeCount`, the `previewBlendModeNames[]` display table,
  the GPU `#define` generator) keeps compiling unchanged, because a type alias to a scoped enum
  carries its enumerators through the alias name in C++. `kPreviewBlendModeCount` may either stay
  as its own `= 12` constant (now describing the aliased type) or itself become `using
  kPreviewBlendModeCount = Math::kBlendModeCount`-equivalent (a `static_assert` verbatim, left to
  the coder ticket to pick the lower-diff form).
- `PreviewComposite_Color_UI.h`: deletes its local `CombineChannel` definition, `#include`s
  `../math/BlendMode_MATH.h`, and calls `Math::CombineChannel(...)` at the one call site inside
  `BlendPreviewColor`. `BlendPreviewColor`, `SplatSurfaceStrata`, `PackRgba8`, etc. are UI-specific
  (they operate on `PreviewColor`, a UI-owned struct) and stay exactly where they are — only the
  single-scalar blend-mode arithmetic and the enum move.
- The Mask stage's new filter-chain evaluator (`§23.5`) references `Math::BlendMode` and calls
  `Math::CombineChannel` directly — legal under `PROC`'s existing `MATH` dependency, no new
  exception needed.

**Not ruled here, left to the Compute Optimization Expert (per the design doc's own §4 routing):**
whether a shared `.glsl` include for `combineChannel`'s expression is worth factoring out so
`PreviewComposite_Color_UI.glsl` and any future Mask GPU kernel don't hand-duplicate the same
switch — a GPU-shader-dispatch-shape question, not a `MATH` header question, and not blocking
this ruling.
