# STEP151 — Bake toggle must never destroy data; add a real "Refresh Bake" action

**Layer:** UI + PROC. **Domain:** `src/ui/LayerEditor_BakedImage_UI.cpp`, `LayerEditor_Action_UI.h`,
`LayerEditor_Signals_UI.h`, `src/proc/NoiseBlend_PROC.h`, `NoiseBlend_Prepare_PROC.cpp`.
**Sequence:** depends on STEP150 (touches the same `LayerEditor_BakedImage_UI.cpp` /
`LayerEditor_Group_UI.cpp` region) — do not start until STEP150 is committed. Ratified from a
design consult with the Generator Expert; this ticket encodes that consult's decisions directly,
do not re-derive the design.

## Root problem (confirmed by direct code reading)
`ApplyBakeToggleAction` (`LayerEditor_BakedImage_UI.cpp:40-59`) has two compounding bugs, and the
human hit the real-world consequence: *"I clicked Bake/Unbake and it messed up the heightmap and I
was unable to undo."*

1. The simple toggle **unconditionally overwrites** `image.image` with a fresh live-noise snapshot
   every time it bakes, even when a stable entry already exists for that `layerIdentifier` (an
   import-decomposition layer, or an earlier deliberate freeze). Unbake-then-rebake silently
   destroys the original pixels with whatever the pipeline happens to be producing right now.
2. The snapshot source, `generationAssembler.NoiseBlend().CachedRawNoiseCpu()[flatIndex]`, is
   matched by **position** — `flatIndex` is recomputed now from `layerStack.GetFlatLayers()`, with
   no guarantee that position matches how `NoiseBlend` last populated its cache. A stack reorder
   between the layer's last `NoiseBlend::Run()` and the click can snapshot the wrong layer's noise.

## Fix — split the single verb into two, per the ratified design

### 1. Bake toggle (existing button/affordance) — redefined to never destroy data
- **Off → On:** call `Data::FindOrAddBakedLayerImage` as today. If the returned entry is
  **freshly created** (`image.image.Width() == 0` — `Data::FloatField`'s default ctor guarantees
  this; verify against the real current `FloatField_DATA.h` before relying on it), this is a
  genuine first-ever bake: snapshot live noise exactly as today (using the corrected identity
  lookup below). If the entry **already has content** (`Width() > 0` — an import, or a prior
  snapshot), **reuse it verbatim — write nothing.** This alone is what makes toggling Bake/Unbake/
  Bake on the imported heightmap layer always restore the original image.
- **On → Off:** unchanged — `bBaked = false`, cache and `bakedImagePath` left untouched.

### 2. New "Refresh Bake" / "Re-snapshot" action — the only path that deliberately overwrites
- A new `LayerEditorActionKind` (add to `LayerEditor_Action_UI.h`) alongside the existing
  `BakeToggleRequested`, wired the same way through `LayerEditor_Signals_UI.h`.
- Only meaningful, and only shown/enabled, while the layer is **unbaked** and
  `layer.noiseType != Params::NoiseType::None` (a layer with no live recipe has nothing to refresh
  from — this reuses the exact same predicate STEP152's `HasActiveProceduralLayer()` needs, don't
  invent a second one).
- Performs exactly what today's single toggle does on the overwrite path: capture
  `liveNoise[matchedIndex]` (via the corrected identity lookup below) and overwrite `image.image`,
  then leave `bBaked` as it found it (this action does not itself toggle bake state — verify with
  whoever finalizes the UI affordance whether it should also set `bBaked = true` immediately after
  refreshing, since a refreshed-but-still-unbaked snapshot is a slightly odd intermediate state;
  default to leaving `bBaked` unchanged unless the UI Expert's placement of this button implies
  otherwise).
- UI placement/exact button label is not specified here — read how STEP150 shipped the Bake/Unbake
  row affordance and add this as a natural sibling action (e.g. a small button only visible/enabled
  in the unbaked+has-recipe state), consistent with that shape.

### 3. Identity-safe snapshot lookup (fixes the position-matching bug)
- Add `NoiseBlendStage::cachedFlatLayerPointers` (`std::vector<const Params::Layer*>`) to
  `NoiseBlend_PROC.h`, populated in `NoiseBlend_Prepare_PROC.cpp::PrepareRun()` from the **same**
  local `flatLayers` that function already uses to build `layerConfigurations`/`cachedRawNoiseCpu`
  — persist it instead of discarding it, don't recompute separately. Expose
  `const std::vector<const Params::Layer*>& CachedFlatLayerPointers() const`.
- `ApplyBakeToggleAction` (and the new Refresh action) look up `&layer` in
  `CachedFlatLayerPointers()` to find the matching slot in `CachedRawNoiseCpu()` — NOT a freshly
  recomputed `layerStack.GetFlatLayers()`. Both vectors are built together in the same
  `PrepareRun()` call, so they are provably in lockstep; no staleness race is possible.
- If `&layer` is not found in the cached pointer list (stack reordered/edited since `NoiseBlend`
  last ran, or it hasn't run yet against this stack), refuse the action exactly as today's "NoiseBlend
  has not run against this stack yet" refusal does — never fall back to guessing a slot.
- This is deliberately pointer/object-identity keying, not `layerIdentifier`-keying: a layer's
  first-ever bake has `layerIdentifier == -1` (every never-baked layer shares that sentinel), so an
  identifier-keyed lookup into the noise cache is ambiguous exactly when it matters most. Pointer
  identity into a same-call-built parallel array delivers the actual reorder-proof guarantee
  `layerIdentifier` was invented for, without that ambiguity. Keep it this way — do not substitute a
  literal `layerIdentifier` key without re-confirming the first-bake case works.

## Explicit out-of-scope
- The Layer Disable toggle and the Erosion/Thermal/FlowAccumulation "active procedural layer" gate
  — STEP152, separate ticket (this ticket's `NoiseType::None` predicate is shared with it, don't
  duplicate the helper if STEP152 lands first; check for `HasActiveProceduralLayer()` on
  `Params::LayerStack` before writing a local equivalent).
- Any UI diagnostic/warning text — out of scope here beyond making the new action's
  enabled/disabled state itself communicate availability (e.g. a disabled/greyed button when not
  applicable is sufficient; no new tooltip text required).

## Acceptance test
Extend the real existing test coverage for `ApplyBakeToggleAction`/`ApplyImportRawAction` (find it —
likely a `LayerEditor_BakedImage_UI_Test.cpp` or similar; STEP102 added the original function, its
own ticket should name the test file). Cover: (1) baking an import-decomposition layer, unbaking,
re-baking reproduces the exact original pixels bit-for-bit; (2) baking a plain procedural layer for
the first time snapshots its current live noise; (3) Refresh Bake overwrites an existing baked image
only when explicitly invoked, never via the simple toggle; (4) a stack reorder between `NoiseBlend`
runs and a bake click is refused rather than silently snapshotting the wrong layer (construct this
via the identity-pointer mismatch, not a guess).

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.
