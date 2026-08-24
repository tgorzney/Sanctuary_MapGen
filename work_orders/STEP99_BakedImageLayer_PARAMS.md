# STEP99 — Baked/image-source layer fields on `Params::Layer`

**Layer:** PARAMS. **Domain:** `Params::Layer`. **Sequence:** first of four
(STEP99 PARAMS → STEP100 PROC → STEP101 IO → STEP102 UI), closing the gap
`ARCH_05_GodObjectDismemberment.md` §5.2 opened ("image-bake state -> a bake
concern in PROC") and never gave a home, and the dormant UI affordances named
in `src/ui/LayerEditor_Action_UI.h`'s SCOPE NOTE.

## Root problem
`Params::Layer` (`src/params/Layer_PARAMS.h`) has no field that says "this
layer's height comes from a stored image, not live noise" — confirmed absent by
direct read, matching `LayerEditor_Action_UI.h`'s own comment. Without it,
neither PROC (STEP100) nor IO (STEP101) has anything to key off, and the two
dormant UI actions (`ImportRawRequested`/`BakeToggleRequested`) stay permanently
reported-never-applied.

Per `LAYER_SYSTEM_SPEC.md` ("Baking"): baking is NOT one-way — a baked
procedural layer keeps its recipe. A layer that started as live noise, gets
baked, and is later unbaked must resume generating from its still-present
noise fields. An imported/decomposed layer (STEP101) has no noise recipe to
fall back to; unbaking it degenerates to `NoiseType::None` (flat) — not a
special case, the existing default behavior of an unset noise layer already.

## Solution — shape
```cpp
// Layer_PARAMS.h — added members
bool bBaked = false;          // frozen: PROC (STEP100) reads a stored image instead of
                               // regenerating live noise for this layer. NOT one-way — see
                               // above. Meaningless combined with a fresh, never-baked layer:
                               // bBaked==true with no matching Data::BakedLayerImage entry
                               // (layerIdentifier not found) degrades to flat, never a crash
                               // (Constitution §6) — STEP100.
std::string bakedImagePath;   // pass-through metadata: the on-disk 16-bit RAW file this
                               // layer's baked pixels came from (Import RAW, picked through
                               // LayerEditor_Group_UI.cpp's existing picker). EMPTY for a
                               // layer baked by the .sanmap per-stratum import decomposition
                               // (STEP101) — its source is the map's own already-loaded
                               // Textures/ payload, not a standalone file. Never read by PROC
                               // (ARCH: PROC has no IO dependency) — IO/UI resolve it into the
                               // DATA cache (STEP100's Data::BakedLayerImage) at load/bake time.
int layerIdentifier = -1;     // stable identity — the key into the PIPELINE-owned
                               // Data::BakedLayerImage cache (STEP100), so a baked layer's
                               // frozen pixels survive reorder/duplicate/insert elsewhere in
                               // the stack (Params::Layer is a plain value inside
                               // std::vector<Layer>; reorder/copy moves this struct, but a
                               // side-cache keyed by flat stack POSITION would not survive it —
                               // see STEP100). -1 = unassigned/never baked. Derive-on-create,
                               // NOT a persisted counter — same posture `STEP56` ratified for
                               // `PropInstanceLayer::layerId`/`DecalInstanceLayer::layerId`,
                               // spelled in full here (`layerIdentifier`, not `layerId`) per
                               // ARCH's own recorded correction for that abbreviation.
```
Derive-on-create helper, same family as `NextPropLayerId` (`STEP56` precedent),
declared here so STEP102 has a fixed target signature to implement against; the
call sites themselves are STEP102's, since they require IO — UI already depends
on IO, PARAMS does not and must not:
```cpp
inline int NextLayerIdentifier(const Params::LayerStack& layerStack) {
    int maximumId = -1;
    for (const Params::GeoLayer& group : layerStack.geoLayers)
        for (const Params::Layer& layer : group.layers)
            maximumId = std::max(maximumId, layer.layerIdentifier);
    return maximumId + 1;
}
```

**Duplicate hazard, flagged for STEP102 to close:** `LayerEditorActionKind::
DuplicateLayer`'s existing handler copies the whole `Params::Layer` struct
verbatim, including `layerIdentifier` — two layers would then share one cache
key. STEP102 must reset the duplicate's `layerIdentifier = -1` and
`bBaked = false` (keep `bakedImagePath` and every noise field verbatim — a
duplicate of a baked NOISE layer should resume live generation, matching "not
one-way"; a duplicate of a pure-image layer with no noise recipe degenerates to
flat until re-baked, which is correct, not a regression).

## IO round-trip (both directions, same posture as STEP66)
`MapExporter_HeightmapStack_IO.cpp`'s `BuildLayerJson` gains:
```cpp
json["Baked"]           = layer.bBaked;
json["BakedImagePath"]  = layer.bakedImagePath;
json["LayerIdentifier"] = layer.layerIdentifier;
```
`MapImporter_HeightmapStack_IO.cpp`'s `ReadLayerJson` gains the mirrored three
reads (`ReadJsonBoolean`/`ReadJsonText`/`ReadJsonInteger`, same helpers every
other field in that function already uses). A document written before this
ticket has none of the three keys; the existing `ReadJson*` helpers already
leave the struct's default (`false`/empty/`-1`) untouched when a key is absent
— no explicit legacy-backfill branch needed (unlike STEP56's `layerId`, which
needed backfill because array POSITION was the only prior identity;
`layerIdentifier` has no such prior meaning to reconstruct from).

## Explicit out-of-scope
- Reading `bBaked`/`bakedImagePath`/`layerIdentifier` anywhere in PROC — STEP100.
- Populating/loading the actual pixel data — STEP100 (DATA shape) + STEP101 (IO
  producer) + STEP102 (UI producer).
- Wiring the UI buttons — STEP102.
- `GeoLayer`-level or whole-stack baking (LAYER_SYSTEM_SPEC names both as valid
  bake scopes) — this ticket and its dependents are PER-LAYER only.
- The recursive-GeoLayer-nesting redesign, the Unified sim-mode cross-band erosion
  mode, the ordered-thickness-column DATA persistence (ARCH §7.5/M6), any
  `FUTURE_SIM_TYPES_SPEC` work — untouched, unrelated.

## Files touched
- `src/params/Layer_PARAMS.h`
- `src/io/MapExporter_HeightmapStack_IO.cpp`
- `src/io/MapImporter_HeightmapStack_IO.cpp`

## Acceptance test
A `Params::Layer` with `bBaked=true`, non-default `bakedImagePath`, and a non-`-1`
`layerIdentifier` round-trips exactly through `HeightmapStack` JSON (extend
`MapImporter_IO_Test.cpp`'s existing layer round-trip fixture). A document with
none of the three keys present leaves a freshly constructed `Layer`'s defaults
(`false`/empty/`-1`) untouched. `NextLayerIdentifier` on an empty stack returns
`0`; on a stack containing identifiers `{0, 2}` (across two different GeoLayers)
returns `3`.

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green, zero
unrelated test files edited.
