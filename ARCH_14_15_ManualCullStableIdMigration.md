[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.15. **Only the ARCH Expert writes this file.**

### 14.15 Manual props/decals cull-path stable-id migration (human ruling, responds to the confirmed InstanceId game-load test)

**Correction to §14.13 item 3's own stale closing line.** That line claimed "Work-Orders A and B
remain unscheduled implementation." Both have since shipped, confirmed by direct read, not by the
sequence doc:
- **Work-Order A (stable id field):** `Params::PropInstanceLayer`/`DecalInstanceLayer` both carry
  `int layerId = -1;` (`src/params/PropInstance_PARAMS.h:30-31`), derive-on-create is wired
  (`PropsTab_Manual_UI.cpp`/`PropsTab_ManualDecals_UI.cpp`), and the wire `"Id"` key round-trips
  (`MapExporter_Props_IO.cpp`/`MapImporter_Props_IO.cpp` and the Decal siblings) — matches STEP56 verbatim.
- **Work-Order B (PROC resolution + correlation column):** `src/proc/Placement_Manual_PROC.cpp`
  exists, is declared on `PlacementStage` (`Placement_PROC.h:64`), and is called unconditionally
  first in `PlacementStage::RunScatter` (`Placement_PROC.cpp:50`, "no dependency on rule configs or
  derived fields"). `Data::PlacementInstances::manualLayerId` exists as a real SoA column
  (`PlacementInstances_DATA.h:28`), threaded through `Append`/`Get`/`Reserve`/`Clear`, and is
  covered by its own acceptance test (`Placement_Manual_PROC_Test.cpp`).

Both are real, shipped, and correct as written. §14.13 item 3's design-closed status stands
unchanged — only its "unscheduled" framing was wrong.

**The human ruling this section records.** `MapCanvas_IconLayer_CullManual_UI.cpp`'s
`ResolvePropsManual`/`ResolveDecalsManual` migrate their prop/decal sub-layer membership test from
raw positional `layerIndex`-vs-`subLayerArrayIndex` matching to a stable-id (`manualLayerId`-concept)
match. The positional approach was a deliberate, flagged exception, not a permanent one (the file's
own header comment: "this is a documented, flagged coder choice, not a silent one"); now that the
stable-id standard is confirmed/de-risked (the InstanceId game-load test, `SANMAP_FORMAT_SPEC.md`),
the human ruled the exception should be retired in favor of the standard rather than preserved
alongside it.

**Staleness finding — confirmed real, not hypothetical.** The literal instruction "read
`manualLayerId` off `Data::PlacementInstances`" was checked against two pieces of real code and
both confirm a genuine timing hazard, worse than "one frame behind":
1. `PlacementStage::ComputeParameterHash()` (`Placement_Hash_PROC.cpp:109-129`) — the exact hash
   PIPELINE's dirty-hash DAG uses to decide whether Placement reruns — hashes geometry, symmetry,
   water, the stage constants, and the four **procedural** rule families
   (`markerRuleLayers`/`propRules`/`unitRules`/`decalRules`). It does **not** hash
   `recipe.props`, `recipe.decals`, `recipe.propLayers`, or `recipe.decalLayers` — the manual-
   authoring data `ResolveManualPropsAndDecals()` copies through.
2. No manual-authoring UI mutation site sets any PIPELINE dirty flag either — `PropsTab_Manual_UI.cpp`
   and `PropsTab_ManualDecals_UI.cpp` contain zero references to any dirty-flag mechanism (grepped,
   confirmed empty).

Net effect: `RunScatter()` re-copies `recipe.props`/`recipe.decals` into
`Data::PlacementInstances` fresh every time it runs, but nothing ties a manual add/move/reorder-
onto-a-different-layer to a rerun. `Data::PlacementInstances::manualLayerId`/position for manual
instances only refreshes incidentally, when something **else** dirties Placement (a procedural rule
edit, geometry change). Reading it directly from the cull path would reintroduce exactly the
"cached PROC output needs stability; live UI redraw doesn't" problem the file's own header comment
already named as the reason the positional/live approach was chosen — potentially stale for an
entire editing session, not one frame.

**Resolution ruled: migrate the match key, not the read source.** Do not point the cull path at
`Data::PlacementInstances`. Adding `recipe.props`/`recipe.decals`/`recipe.propLayers`/
`recipe.decalLayers` to `ComputeParameterHash()` so a manual edit dirties Placement was considered
and rejected: it would force a full `RunScatter()` — `BuildRuleConfigurations`, `BuildDerivedFields`,
and every enabled procedural rule's `ScatterRule()` — on every manual-prop drag frame, which
directly violates the Tier C/C2 "zero GPU recompute... zero DAG/dirty-hash involvement" cost model
`ARCH_14_08_DirtyFlagTiers.md` already ratified for exactly this class of interaction, and
Constitution §3's maximum-performance law. Instead: keep reading live PARAMS as today (zero
staleness, zero DAG coupling, unchanged Tier C/C2 cost), but resolve the **same** id value
`Placement_Manual_PROC.cpp` computes, live, and compare on it instead of on raw `layerIndex`.

**Single source of truth for the id-resolution formula — promote it to PARAMS.**
`Placement_Manual_PROC.cpp`'s private `ResolveManualLayerId` overloads (a two-line bounds-checked
array lookup with a `-1` sentinel, `Placement_Manual_PROC.cpp:15-22`) are pure functions of
`Params::PropInstanceLayer`/`DecalInstanceLayer` — no PROC-specific behavior. Duplicating that
formula a second time inside the UI cull path (as `TemplateIdentifierFromBlueprintPath` duplicates
a five-line mechanical string op locally, per that file's own comment) would let the "official"
PROC-baked copy and the UI's live cull resolution drift out of sync on a future edit to one but not
the other. Ruled: two small inline free functions move to `src/params/PropInstance_PARAMS.h`,
beside the structs they read:
```cpp
inline int ResolvePropInstanceLayerId(int layerIndex, const std::vector<PropInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}
inline int ResolveDecalInstanceLayerId(int layerIndex, const std::vector<DecalInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}
```
`Placement_Manual_PROC.cpp:15-22` calls these instead of keeping its own private overloads (thin
wrapper or direct call — no behavior change either way, coder's choice).

**Exact call sites in `MapCanvas_IconLayer_CullManual_UI.cpp`:**
- `ResolvePropsManual` (lines 80-100): before the `for (const Params::PropInstanceGroup& group ...)`
  loop, resolve once per call: `const int targetLayerId = Params::ResolvePropInstanceLayerId(subLayerArrayIndex, input.recipe->propLayers);`.
  Replace line 92's `if (propTransform.layerIndex != subLayerArrayIndex) continue;` with
  `if (Params::ResolvePropInstanceLayerId(propTransform.layerIndex, input.recipe->propLayers) != targetLayerId) continue;`.
- `ResolveDecalsManual` (lines 102-119): identical shape against `input.recipe->decalLayers` and
  `Params::ResolveDecalInstanceLayerId`, replacing line 111's equivalent comparison.
- File header comment (lines 9-12): update to record that the positional exception is retired,
  replaced by the stable-id match resolved live against PARAMS (not read from `Data::
  PlacementInstances`) — cite this section.

**No new UI→DATA coupling introduced.** `Data::PlacementInstances::manualLayerId` itself is
untouched by this ruling and stays exactly as shipped; it currently has zero consumers besides its
own writer and test (confirmed: no reference anywhere outside `Placement_Manual_PROC.cpp`,
`PlacementInstances_DATA.h`/`PlacementInstance_DATA.h`, and `Placement_Manual_PROC_Test.cpp`). This
ruling does not create or rely on a first consumer of that column — the cull path's `manualLayerId`
match is computed independently, live, against PARAMS.

**Dispatchable as-is.** Narrow, two call sites, exact before/after text, one small shared-helper
placement decision (ruled above, not left to the coder). No further design pass needed.
