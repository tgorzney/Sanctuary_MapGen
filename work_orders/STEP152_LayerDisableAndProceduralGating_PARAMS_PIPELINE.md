# STEP152 — Layer Disable toggle (separate from visibility) + skip sim when nothing is procedural

**Layer:** PARAMS + PIPELINE + IO + UI. **Domain:** `src/params/Layer_PARAMS.h`,
`GeoLayer_PARAMS.h`, `LayerStack_PARAMS.h`, `src/pipeline/GenerationAssembler_Stages_PIPELINE.cpp`,
`src/io/MapImporter_HeightmapStack_IO.cpp`, `MapExporter_HeightmapStack_IO.cpp`,
`src/ui/LayersTab_UI.h`/`.cpp`, `LayerEditor_UI.cpp`/`LayerEditor_Group_UI.cpp`,
`DraggableListWidget_UI.h`.
**Sequence:** depends on STEP150 (shares `LayerEditor_Group_UI.cpp`/`DraggableListWidget_UI.h`) and
STEP151 (shares `LayerEditor_BakedImage_UI.cpp`'s `NoiseType::None` predicate — reuse, don't
duplicate). Do not start until both are committed. Ratified from a design consult with the
Generator Expert; this ticket encodes that consult's decisions directly.

## Root problem
Two related gaps, both confirmed by direct code reading and both from the human's own explicit
design intent: *"I think we need a disable toggle, separate from the view toggle. Disable would
remove from preview and not do ANY calculations for the layer, it is as if the layer did not exist"*
and *"no procedural calculations should be done except once, including Slope, Flow etc. unless there
is an active procedural layer."*

1. `Params::Layer::bEnabled`/`Params::GeoLayer::bEnabled` is currently dual-purpose: it drives BOTH
   the UI visibility eye-icon AND `Params::LayerStack::GetFlatLayers()`'s generation-inclusion
   filter (every downstream PROC stage only sees what survives this filter). There is no field that
   separates "hide from preview" from "skip generation calculation entirely."
2. Erosion/Thermal/FlowAccumulation (`GenerationAssembler_Stages_PIPELINE.cpp:68-85`) run
   unconditionally per their own dirty-hash with zero check for "does any active procedural layer
   even exist." This ran even when every layer was baked/disabled, redoing expensive simulation for
   no visible effect.

## Fix

### 1. New `bDisabled` field, both levels
Add to `Params::Layer` (`Layer_PARAMS.h`) and `Params::GeoLayer` (`GeoLayer_PARAMS.h`), matching
existing naming precedent (`bLocked`, `bEnabled` siblings):
```cpp
bool bDisabled = false;   // generation-inclusion ONLY. True = as if the layer/group did not exist:
                          // excluded from GetFlatLayers() and therefore every PROC stage. Independent
                          // of bEnabled (below), which stays UI-only.
```
`bEnabled` itself is NOT renamed or removed — it keeps driving the visibility eye-icon exactly as
today, in both `LayersTab_UI.cpp` and `LayerEditor_UI.cpp`/`LayerEditor_Group_UI.cpp`. It simply
loses its one PROC consumer (see next point), becoming pure UI-facing metadata — the same posture
`Layer::name` already documents for itself (`Layer_PARAMS.h:14-16`, "no stage consumes it").

### 2. `GetFlatLayers()` change (`LayerStack_PARAMS.h`)
Move the generation-inclusion gate from `bEnabled` to `bDisabled`:
```cpp
for (const GeoLayer& group : geoLayers) {
    if (group.bDisabled) continue;
    for (const Layer& layer : group.layers)
        if (!layer.bDisabled) flat.push_back(&layer);
}
```
Verify the real current loop shape before editing — this is a one-condition swap, not a rewrite.
Before landing this, grep every real `.bEnabled` reference on `Layer`/`GeoLayer` across the tree and
confirm (as the design consult already did) that nothing outside `GetFlatLayers()`, the two UI draw
sites, `LayersTab_UI.h`'s toggle appliers, and the two IO round-trip files reads it — if a new
`.bEnabled` consumer has appeared since, stop and flag it rather than silently changing behavior
underneath it.

### 3. `HasActiveProceduralLayer()` helper (`LayerStack_PARAMS.h`)
```cpp
// True when the flattened stack has at least one layer that will actually be live-computed this
// run: generation-included, not frozen, and not the always-flat NoiseType::None sentinel (an
// unbaked recipe-less layer contributes nothing and must not count as "active").
bool HasActiveProceduralLayer() const {
    for (const Layer* layer : GetFlatLayers())
        if (!layer->bBaked && layer->noiseType != NoiseType::None) return true;
    return false;
}
```

### 4. Gate Erosion/Thermal/FlowAccumulation (`GenerationAssembler_Stages_PIPELINE.cpp`)
Wrap only these three stages' `run` closures — Mask/Placement/Bake and NoiseBlend itself stay
unconditional (their required inputs, e.g. `materialProportions` seeded by NoiseBlend and `slope`
self-computed by Mask, degrade to well-defined values, not garbage, when the sim block is skipped —
already confirmed by the design consult):
```cpp
AddStage("Erosion", full, [this] { return erosionStage.ComputeParameterHash(); },
         [this] { if (recipe.layerStack.HasActiveProceduralLayer()) erosionStage.Run(); });
AddStage("Thermal", full, [this] { return thermalStage.ComputeParameterHash(); },
         [this] { if (recipe.layerStack.HasActiveProceduralLayer()) thermalStage.Run(); });
AddStage("FlowAccumulation", full, [this] { return flowAccumulationStage.ComputeParameterHash(); },
         [this] { if (recipe.layerStack.HasActiveProceduralLayer()) flowAccumulationStage.Run(); });
```
`ComputeParameterHash()` for all three stays untouched — the dirty-hash bookkeeping keeps working
exactly as before; only the stage body becomes a cheap early-return "nothing to do" when gated.

**Ratified decision on a known tradeoff (do not re-litigate):** `LAYER_SYSTEM_SPEC.md`'s baking
rationale frames a frozen layer as letting expensive sims skip *redundant* recompute while still
running on top of a frozen base. This gate goes further: once the LAST active procedural layer is
baked/disabled, Erosion/Thermal/Flow stop running entirely, even for a designer who froze their only
layer specifically to tune erosion against a fixed base. The human's own words — twice, unprompted —
were unambiguous ("no procedural calculations... unless there is an active procedural layer"; "If
baked was toggled again, no procedural calculation would be done") and this ticket is ratified to
match them exactly. This is a known, accepted behavior change from the older spec framing, not an
oversight.

### 5. IO round-trip
Add `bDisabled` to both `MapImporter_HeightmapStack_IO.cpp` and `MapExporter_HeightmapStack_IO.cpp`,
following the **exact same pattern** already used for `bEnabled`'s `"Enabled"` JSON key in those same
files (additive field, defaults to `false` for an older document missing the key, no `SanGenVersion`
bump — matching every other additive-field precedent in this IO layer). Do not invent a new pattern.

### 6. UI affordance for `bDisabled`
Read how STEP150 shipped the Bake/Unbake per-row header affordance on `DraggableListWidget_UI.h`
before writing this — it added some form of optional per-row custom action button. This ticket needs
a SECOND independent per-row toggle-affordance (Disable, on top of Bake/Unbake and the existing
visibility/lock/delete strip) on the same Layer/GeoLayer rows. Extend whatever STEP150 actually
shipped to support this — either generalize to a small ordered list of optional custom
buttons/toggles, or add one more dedicated optional slot — whichever fits the real shipped shape
with least churn. `DraggableListWidget_UI.h` is shared by Markers/Props/Decals/Units too: leaving
the new slot(s) unset must not change their rendering or behavior at all — verify this holds before
finishing. Wire the Disable affordance to write `layer.bDisabled`/`group.bDisabled` through the same
kind of action-recording path `BakeToggleRequested` uses (`LayerEditor_Action_UI.h`/
`LayerEditor_Signals_UI.h`), and make sure `LayersTab_UI.cpp`'s own row rendering (the OTHER surface
that draws these rows, per `LayersTab_UI.h`'s `ApplyGeoLayerListSignal`/`ApplyLayerListSignal`) gets
the same affordance — this needs to work from both UI surfaces, not just the Layer Editor.

### 7. Minimal diagnostics
- When `!layerStack.HasActiveProceduralLayer()`, show a short status line near the Erosion/Thermal/
  Flow settings (or wherever those settings are edited) noting they're currently skipped because
  nothing procedural is active. Exact wording/placement is your call — keep it one line, no modal,
  consistent with STEP150's "Baked — procedural settings hidden" precedent line.
- On a `bDisabled` layer/group row, a short inline note (e.g. next to the name, or as the row's
  disabled/greyed visual state) making it visually obvious it's excluded from generation — don't
  rely on the affordance icon alone being self-explanatory.

## Explicit out-of-scope
- The bake-toggle data-safety fix and the new "Refresh Bake" action — STEP151, must already be
  landed (this ticket's `HasActiveProceduralLayer()` reuses STEP151's `NoiseType::None` predicate;
  check it isn't duplicated).
- Any change to Mask, Placement, or Bake stage gating — they stay unconditional.
- A general-purpose "N optional buttons" redesign of `DraggableListWidget_UI.h` beyond what's needed
  to fit this one additional affordance.

## Acceptance test
Extend `LayerStack_PARAMS_Test.cpp` (or wherever `GetFlatLayers()` is currently tested — find the
real file): a disabled layer/group is excluded from `GetFlatLayers()` while remaining visible/
enabled has no effect on it; `HasActiveProceduralLayer()` returns false when every layer is
baked-or-disabled-or-recipe-less and true when at least one isn't. Extend the pipeline-stage test
coverage (find the real existing test around `GenerationAssembler_Stages_PIPELINE.cpp` — likely a
`GenerationAssembler_PIPELINE_Test.cpp`): Erosion/Thermal/FlowAccumulation's `Run()` is NOT invoked
when `HasActiveProceduralLayer()` is false, and IS invoked when true, via a real `assembler.Run()`
pass (not a mock). Extend IO round-trip tests for `bDisabled` symmetric with `bEnabled`'s existing
coverage. Extend the UI action-wiring test (same file STEP150 extended) for the new Disable
affordance.

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.
