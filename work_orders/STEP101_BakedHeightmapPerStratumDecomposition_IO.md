# STEP101 — Import decomposes a baked heightmap + stratum masks into per-stratum baked layers

**Layer:** IO. **Domain:** `MapImporter::LoadBakedFields` (`src/io/
MapImporter_Fields_IO.cpp`), `Params::MapRecipe::layerStack`. **Sequence:** depends
on STEP99 (fields) + STEP100 (`Data::BakedLayerImage`,
`GenerationAssembler::BakedLayerImages()`). This is the ticket that fixes the
reported bug end-to-end — STEP99/100 make the mechanism exist; this makes import
actually use it.

## Root problem
Confirmed by direct read: when `Io::MapImporter::LoadSanmap` or
`RunSelectiveMigrationImport` calls `LoadBakedFields`, the result lands only in
`Data::MapFields` — `recipe.layerStack` is left exactly as `ReadHeightmapStackJson`
parsed it (empty, for a genuine externally-authored `.sanmap` with no SanGen
`HeightmapStack` section). Nothing then tells `NoiseBlendStage` to preserve what
was just loaded — hence the bug.

## Two real gaps found while verifying "not already real"
1. **CONFIRMED REAL, fixed by this ticket:** `LoadStratumMaskTga` writes the
   imported TGAs into `outFields.surfaceStratumWeights[...]`. Per `MASKING_SPEC.md`
   §1.5/§1.6, IO must seed `materialProportions` from the imported masks (a
   physical-field approximation), never `surfaceStratumWeights` (Mask-stage-
   exclusive output, §1.1/§1.3 rule 1 — "IO loads a field; it never simulates").
   This is a real, load-bearing violation of the single-writer rule, not just a
   naming nit. **Fix:** retarget `LoadStratumMaskTga`'s writes from
   `outFields.surfaceStratumWeights[weightIndex]` to
   `outFields.materialProportions[weightIndex]`, with a comment citing
   MASKING_SPEC §1.5. This ticket's own per-stratum decomposition (below) then
   reads `materialProportions` as its mask input, consistent with the fix.
2. **CONFIRMED REAL, flagged, NOT fixed here (separate, cross-cutting, needs its
   own coordinated ticket):** the TGA channel-to-array-index mapping in
   `WriteStratumMaskTga`/`LoadStratumMaskTga` treats array index 8 as "the
   implicit BASE stratum" and array indices 0..7 as strata "1-8." This
   contradicts every other consumer in the codebase, which treats array index ==
   the literal, unshifted stratum index — confirmed by `Application_Recipe_UI.cpp`
   (`baseLayer.stratumIndex = 0`, direct), by `NoiseBlend_Blend_PROC.cpp`
   (`materialProportions[configuration.stratumIndex]`, direct), and by
   `LAYER_SYSTEM_SPEC.md`'s own ratified prose ("Stratum 0 = the always-present
   base (no mask). Strata 1-8 get masks" — array index 0 = stratum 0 = no mask
   file, matching the natural reading, not the shifted one). This is a standing,
   pre-existing, cross-cutting defect (affects every `.sanmap` stratum-mask round
   trip, not just this feature) — recording it here, routed to the IO Architecture
   Expert / Format Expert for its own ticket, verified against a real shipped
   map's actual TGA bytes. **This ticket's own new code uses the CORRECT
   (unshifted, direct-index) convention** — if today's loader is genuinely
   off-by-one, that will surface as a visible, attributable symptom in testing
   (e.g. stratum 0's decomposed layer reads stratum 1's mask), not silently
   compound.

## Solution — the decomposition
`LoadBakedFields`'s signature changes to take a non-const
`Params::MapRecipe& recipe` (it must now be able to inject synthesized layers) and
a new `std::vector<Data::BakedLayerImage>& outBakedLayerImages` parameter. Both
existing call sites already hold a non-const recipe at that scope — verify and
adjust exactly at implementation time.

New function, called from `LoadBakedFields`'s tail right after it finishes
populating `outFields`:
```cpp
// MapImporter_Fields_IO.cpp, new
void DecomposeBakedHeightmapIntoLayers(Params::MapRecipe& recipe, Data::MapFields& fields,
                                       std::vector<Data::BakedLayerImage>& outBakedLayerImages) {
    // Only when there is no live recipe to trust — a genuine externally-authored
    // map, exactly the reported bug's scenario. A SanGen-authored map's own
    // HeightmapStack recipe already reproduces the baked art; injecting synthetic
    // layers on top of a real recipe would double-apply the height.
    if (!recipe.layerStack.geoLayers.empty()) return;
    const int vertexSize = fields.VertexSize();
    if (vertexSize < 2) return;

    Params::GeoLayer importedGroup;
    importedGroup.name      = "Imported Bake";
    importedGroup.mode      = Params::GeoLayerMode::Material;
    importedGroup.blendMode = Params::HeightBlendMode::Add;

    int nextIdentifier = 0;   // fresh recipe — NextLayerIdentifier(recipe.layerStack) == 0
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        Data::BakedLayerImage image;
        image.image.Resize(vertexSize, vertexSize, 0.0f);
        bool bAnyContribution = false;
        for (int y = 0; y < vertexSize; ++y) {
            for (int x = 0; x < vertexSize; ++x) {
                float weight;
                if (stratum == 0) {
                    float coveredByOthers = 0.0f;
                    for (int other = 1; other < Data::MapFields::stratumCount; ++other)
                        coveredByOthers += fields.materialProportions[other].Get(x, y);
                    weight = 1.0f - coveredByOthers;
                    if (weight < 0.0f) weight = 0.0f;
                } else {
                    weight = fields.materialProportions[stratum].Get(x, y);
                }
                if (weight > 0.0001f) bAnyContribution = true;
                image.image.Set(x, y, fields.heightfield.Get(x, y) * weight);
            }
        }
        if (stratum != 0 && !bAnyContribution) continue;   // skip empty strata — stratum 0 is
                                                             // mandatory
        Params::Layer layer;                                // every OTHER field stays default —
        layer.stratumIndex     = stratum;                   // that's what makes ReshapeLayerValue
        layer.bBaked            = true;                      // an identity pass-through (STEP100)
        layer.layerIdentifier   = nextIdentifier++;
        image.layerIdentifier   = layer.layerIdentifier;
        importedGroup.layers.push_back(layer);
        outBakedLayerImages.push_back(std::move(image));
    }
    recipe.layerStack.geoLayers.push_back(std::move(importedGroup));
}
```
Called from `LoadBakedFields`'s tail:
```cpp
if (bHeightmapLoaded)
    DecomposeBakedHeightmapIntoLayers(recipe, outFields, outBakedLayerImages);
```
The caller (`FilesTab_Actions_UI.cpp`/`FilesTab_MigrationImport_Actions_UI.cpp`)
passes `assembler.BakedLayerImages()` (STEP100), the same way it already passes
`*fields` for `assembler.Fields()`. This requires threading
`Pipeline::GenerationAssembler&` (or its two members) one level deeper into
`RunOpenSanmap`/`RunSelectiveMigrationImport` and `FilesTab_UI.h`'s
`RunFilesTabAction` signature — a mechanical plumbing change, same shape as the
existing `Data::MapFields* fields` parameter those functions already carry.

**Re-import/re-hydration (not just first import):** for a layer with `bBaked ==
true` and `bakedImagePath.empty()` found ALREADY PRESENT in `recipe.layerStack`
(e.g. re-opening a `.sanmap` SanGen itself exported after this feature shipped —
its `HeightmapStack` is no longer empty, so the synthesis gate above skips), the
SAME per-stratum formula re-derives that layer's image from the freshly-reloaded
`fields.materialProportions[layer.stratumIndex]` — call the per-layer body of the
loop above for those layers too, keyed by their EXISTING `stratumIndex`/
`layerIdentifier` rather than minting new ones. Fold this into
`DecomposeBakedHeightmapIntoLayers` as a second branch (existing baked layers →
re-derive by `stratumIndex`; no existing layers at all → synthesize fresh) rather
than a separate function, so there is one source of truth for the per-stratum
formula.

## Explicit out-of-scope
- **The TGA channel/array-index off-by-one** (gap 2 above) — flagged, not fixed.
- **`bakedImagePath`-sourced re-hydration** (loading an arbitrary external file
  back into the DATA cache on re-open) — that path only exists for STEP102's
  Import RAW action; this ticket's decomposed layers never set `bakedImagePath`.
- **`Data::StratumArt::importedMask`** — a different mechanism (Mask-stage
  `ImportedMaskMode` merge input), not populated by `MapImporter` today by any
  code path this investigation found; unrelated to this ticket, not touched.
- GeoLayer-level/whole-stack baking; the recursive-GeoLayer redesign; Unified
  cross-band erosion; the M6 ordered-thickness-column DATA persistence;
  `FUTURE_SIM_TYPES_SPEC`.

## Files touched
- `src/io/MapImporter_Fields_IO.cpp`
- `src/io/MapImporter_IO.h` (signature)
- `src/io/MapImporter_IO.cpp` (call site)
- `src/ui/FilesTab_Actions_UI.cpp`, `src/ui/FilesTab_MigrationImport_Actions_UI.cpp`,
  `src/ui/FilesTab_UI.h` (plumbing `Data::BakedLayerImage` vector through)

## Acceptance test
Import a synthetic `.sanmap` (no `HeightmapStack` section) with a known
`heightmap.raw` + one-stratum `stratums_1_4.tga` (all-white R channel = stratum 1
fully covering). After import: `recipe.layerStack.geoLayers` has one GeoLayer with
(at most) 2 `Layer`s (stratum 0 and stratum 1, since strata 2-8 are empty),
`bBaked==true` on both, distinct `layerIdentifier`s; `assembler.BakedLayerImages()`
has 2 matching entries. Running `assembler.Run()` once reproduces the ORIGINAL
imported heightfield exactly (proving the fix — this is the acceptance bar for the
reported bug). Re-importing the SAME `.sanmap` a second time after it round-trips
through export (STEP99's IO) does not duplicate layers and reproduces the same
heightfield. `materialProportions[1]` after import matches the TGA's R channel
(0..1); `surfaceStratumWeights` is untouched by import (stays at
`MapFields::Resize`'s zero-fill) until the pipeline's own Mask stage runs.

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green, zero
unrelated test files edited.
