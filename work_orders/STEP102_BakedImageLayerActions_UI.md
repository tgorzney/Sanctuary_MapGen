# STEP102 — Wire Import RAW / Bake buttons to the new baked-layer PARAMS/DATA

**Layer:** UI, IO (one small exposed primitive). **Domain:** `LayerEditor_Action_UI.h`,
`LayerEditor_Group_UI.cpp`, `LayerEditor_UI.cpp`. **Sequence:** depends on STEP99
(fields) + STEP100 (`Data::BakedLayerImage`, `NoiseBlendStage::CachedRawNoiseCpu()`)
+ STEP101 (import path, so a full open->generate loop is testable end to end).
Closes the SCOPE NOTE in `LayerEditor_Action_UI.h` and `LayerEditor_UI.h`'s
"detected, never applied" note.

## Root problem
`ApplyLayerEditorAction` explicitly refuses `ImportRawRequested`/
`BakeToggleRequested` — confirmed by `LayerEditor_Signals_UI_Test.cpp`'s own
assertion that both are "reported, never applied." STEP99/100/101 now give both
actions a real PARAMS/DATA target.

## Solution
**New small IO primitive** (expose an existing, already-tested private function —
minimal footprint): `LoadHeightmapRaw` (`MapImporter_Fields_IO.cpp`, currently
anonymous-namespace-private) becomes a public static method on `MapImporter`,
`LoadRawHeightmapIntoField(filePath, vertexSize, outField, options, result)` —
same body, generalized to write into a caller-supplied `Data::FloatField&` instead
of always `outFields.heightfield` (its one existing call site, `LoadBakedFields`,
passes `outFields.heightfield` as before — no behavior change there).

**Apply logic** — a new function, `LayerEditor_BakedImage_UI.h`/`.cpp` (new files;
`LayerEditor_Action_UI.h` stays IO-free and header-only per its own contract —
this is a NEW UI file, not an edit to that one, since it's the first thing in
this family that legitimately needs IO, matching how `LayerEditor_Erosion_UI.h`
already reaches `Pipeline::GenerationAssembler` for a cross-cutting concern with
no PARAMS home):
```cpp
// Called from LayerEditor_UI.cpp's frame-signal handling, alongside
// ApplyLayerEditorFrameSignals, when signals.action.kind is ImportRawRequested
// or BakeToggleRequested — both need generationAssembler, which DrawLayerEditor
// already threads through.
bool ApplyBakedImageAction(const LayerEditorAction& action, Params::LayerStack& layerStack,
                           Pipeline::GenerationAssembler& generationAssembler) {
    if (!LayerEditorActionNamesLayer(layerStack, action.geoLayerIndex, action.layerIndex))
        return false;
    Params::Layer& layer = layerStack.geoLayers[action.geoLayerIndex].layers[action.layerIndex];

    if (action.kind == LayerEditorActionKind::ImportRawRequested) {
        const int vertexSize = generationAssembler.Fields().VertexSize();
        Data::FloatField loaded;
        Io::MapImportOptions options;   // caller-tunable safety limits, same defaults RunOpenSanmap uses
        Io::MapImportResult result;
        if (!Io::MapImporter::LoadRawHeightmapIntoField(action.importRawPath, vertexSize, loaded,
                                                        options, result))
            return false;   // refused, logged (Constitution §6) — layer untouched
        if (layer.layerIdentifier < 0) layer.layerIdentifier = NextLayerIdentifier(layerStack);
        Data::BakedLayerImage& image = FindOrAddBakedLayerImage(
            generationAssembler.BakedLayerImages(), layer.layerIdentifier);
        image.image = std::move(loaded);
        layer.bakedImagePath = action.importRawPath;
        layer.bBaked = true;
        return true;
    }
    if (action.kind == LayerEditorActionKind::BakeToggleRequested) {
        if (!layer.bBaked) {   // Bake: snapshot the CURRENT live noise output
            const std::vector<const Params::Layer*> flat = layerStack.GetFlatLayers();
            std::size_t flatIndex = flat.size();
            for (std::size_t i = 0; i < flat.size(); ++i) if (flat[i] == &layer) { flatIndex = i; break; }
            if (flatIndex >= flat.size()) return false;
            if (layer.layerIdentifier < 0) layer.layerIdentifier = NextLayerIdentifier(layerStack);
            Data::BakedLayerImage& image = FindOrAddBakedLayerImage(
                generationAssembler.BakedLayerImages(), layer.layerIdentifier);
            image.image = generationAssembler.NoiseBlend().CachedRawNoiseCpu()[flatIndex];
            layer.bBaked = true;         // bakedImagePath stays empty — sourced from live noise,
        } else {                        // not a file
            layer.bBaked = false;        // Unbake: resume live generation from the SAME
        }                                // still-present noise recipe fields (not one-way)
        return true;
    }
    return false;
}
```
`FindOrAddBakedLayerImage` — small helper alongside `FindBakedLayerImage`
(STEP100), inserts a fresh zero-initialized entry when the identifier isn't found
yet.

**Duplicate-layer fix** (flagged by STEP99): `ApplyLayerEditorAction`'s
`DuplicateLayer` branch resets the copy's identity:
```cpp
Params::Layer duplicatedLayer = group.layers[static_cast<std::size_t>(action.layerIndex)];
duplicatedLayer.layerIdentifier = -1;
duplicatedLayer.bBaked = false;
```
(`bakedImagePath` and every noise field stay copied verbatim — see STEP99's
reasoning.)

**Wiring into `LayerEditor_UI.cpp`:** wherever it currently calls
`ApplyLayerEditorFrameSignals`, add a fourth step, AFTER that call (so index
shifts from Add/Duplicate/Delete are already resolved): if `signals.action.kind`
is `ImportRawRequested`/`BakeToggleRequested`, call `ApplyBakedImageAction`.

## Explicit out-of-scope
- GeoLayer-level bake button (LAYER_SYSTEM_SPEC names it; not built here — this
  ticket is per-layer only, matching STEP99-101).
- Any new picker UI beyond the existing `DrawFilePathPicker` call
  (`LayerEditor_Group_UI.cpp`) — reused unchanged.
- GPU backend behavior for a stack containing a baked layer — STEP100 already
  forces CPU; this ticket does not add a UI warning/indicator for that (a
  reasonable follow-up, not required for the reported bug).

## Files touched
- `src/io/MapImporter_IO.h`, `src/io/MapImporter_Fields_IO.cpp` (expose
  `LoadRawHeightmapIntoField`)
- `src/ui/LayerEditor_BakedImage_UI.h` (new), `.cpp` (new)
- `src/ui/LayerEditor_Action_UI.h` (duplicate-layer identity reset)
- `src/ui/LayerEditor_UI.cpp` (wiring)
- `src/data/BakedLayerImage_DATA.h` (`FindOrAddBakedLayerImage` helper)

## Acceptance test
Import RAW on a selected layer with a real 16-bit RAW file: the layer's `bBaked`
becomes true, `bakedImagePath` matches the picked path, a matching
`Data::BakedLayerImage` appears in `generationAssembler.BakedLayerImages()`, and
the NEXT `assembler.Run()` reproduces that file's contents in
`mapFields.heightfield` at that layer's position in the additive stack. Bake on a
live noise layer snapshots its CURRENT `CachedRawNoiseCpu()` output; a subsequent
parameter edit to that layer's noise settings does NOT change `mapFields.heightfield`
until Unbake is pressed, at which point it resumes live generation and DOES
reflect the edit. Duplicate on a baked layer produces an unbaked, distinct-identity
copy (extends `LayerEditor_Signals_UI_Test.cpp`'s existing duplicate coverage).
Import RAW with a rejected extension or an unreadable file leaves the layer
untouched and logs a reason (Constitution §6).

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green, zero
unrelated test files edited. This is the final ticket in the sequence — after it
lands, opening `Pandemonium Isthmus.sanmap` must show its real terrain
immediately on open.
