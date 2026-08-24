# STEP105 — Import synthesizes ONE flattened baked height layer; stratum masks feed `ImportedMaskMode`, not height decomposition

**Layer:** IO. **Domain:** `MapImporter_HeightmapDecomposition_IO.{h,cpp}`, `MapImporter_Fields_IO.cpp`, `MapImporter_IO.{h,cpp}`, `Pipeline::GenerationAssembler::StratumArt()`, `Params::Stratum::importedMaskMode`. **Sequence:** REVISES already-shipped STEP101 (commit `5dbc1ff`, `work_orders/STEP101_BakedHeightmapPerStratumDecomposition_IO.md`) and its test (`MapImporter_HeightmapDecomposition_IO_Test.cpp`). Depends on the already-shipped, already-tested `Params::ImportedMaskMode` / `Data::StratumArt` / `Mask_Prepare_PROC.cpp` / `Mask_Merge_PROC.{h,glsl}` machinery — confirmed real and untouched by this ticket.

## Root problem
STEP101's `DecomposeBakedHeightmapIntoLayers` (`src/io/MapImporter_HeightmapDecomposition_IO.cpp`) reads a genuinely single-channel, single grayscale `heightmap.raw` and algebraically re-derives ONE `Params::Layer` **per non-empty stratum** by multiplying the whole heightfield by that stratum's `materialProportions` weight cell-by-cell (`ComputeStratumBakedImage`). This sums back to the original exactly, but it is not a real decomposition — the source never had independent per-material height — and none of the resulting layers is independently editable the way a real material layer should be (editing "stratum 3's baked layer" secretly just scales a fraction of the same shared heightmap). The human's correction: height import should synthesize exactly ONE baked layer holding the flattened heightmap as-is; per-stratum mask art is a wholly separate concern that belongs on the already-shipped `Params::Stratum::importedMaskMode` (`Disabled`/`ProceduralStart`/`StaticOverride`) path (`src/params/Stratum_PARAMS.h`, consumed by `src/proc/Mask_Prepare_PROC.cpp` and `src/proc/Mask_Merge_PROC.h`), which today is never fed by the importer (`Data::StratumArt::importedMask`, `src/data/StratumArt_DATA.h`, is populated nowhere in `src/io/`).

## Fix

### 1. Height: collapse `DecomposeBakedHeightmapIntoLayers` to a single layer
Delete `ComputeStratumBakedImage` and the per-stratum loop in both branches. The FRESH-SYNTHESIS branch mints exactly one `Params::Layer` at its default `stratumIndex == 0` (`src/params/Layer_PARAMS.h` already defaults to 0 — the base/no-mask stratum, `LAYER_SYSTEM_SPEC`'s "Stratum 0 = the always-present base" — so this needs no new field, just dropping the per-stratum split); its image is `fields.heightfield` copied verbatim, not masked:

```cpp
void DecomposeBakedHeightmapIntoLayers(Params::MapRecipe& recipe, Data::MapFields& fields,
                                       std::vector<Data::BakedLayerImage>& outBakedLayerImages) {
    const int vertexSize = fields.VertexSize();
    if (vertexSize < 2) return;

    if (!recipe.layerStack.geoLayers.empty()) {                 // RE-HYDRATION: unchanged gate
        for (Params::GeoLayer& group : recipe.layerStack.geoLayers)
            for (Params::Layer& layer : group.layers) {
                if (!layer.bBaked || !layer.bakedImagePath.empty()) continue;
                Data::BakedLayerImage bakedImage;
                bakedImage.layerIdentifier = layer.layerIdentifier;
                bakedImage.image = fields.heightfield;           // STEP105: verbatim, no per-stratum mask
                outBakedLayerImages.push_back(std::move(bakedImage));
            }
        return;
    }

    Params::GeoLayer importedGroup;
    importedGroup.name      = "Imported Bake";
    importedGroup.mode      = Params::GeoLayerMode::Material;
    importedGroup.blendMode = Params::HeightBlendMode::Add;

    Params::Layer layer;                                         // stratumIndex stays its default (0)
    layer.bBaked          = true;
    layer.layerIdentifier = Params::NextLayerIdentifier(recipe.layerStack);
    importedGroup.layers.push_back(layer);

    Data::BakedLayerImage bakedImage;
    bakedImage.layerIdentifier = layer.layerIdentifier;
    bakedImage.image = fields.heightfield;
    outBakedLayerImages.push_back(std::move(bakedImage));

    recipe.layerStack.geoLayers.push_back(std::move(importedGroup));
}
```
`MapImporter_HeightmapDecomposition_IO.h`'s header comment ("One baked `Params::Layer` per non-empty stratum") gets rewritten to describe the single-layer contract; the two-branch (fresh-synthesis / re-hydration) shape stays, only the per-stratum body inside each is deleted.

**Migration hazard to flag, not fix here:** a `.sanmap` already exported under STEP101's per-stratum scheme (multiple `bBaked` layers, distinct `stratumIndex`, no `bakedImagePath`, under one `Material`-mode "Imported Bake" `GeoLayer`) hits the RE-HYDRATION branch on next open. After this ticket, EVERY one of those old layers re-derives as the FULL heightfield verbatim, and `GeoLayerMode::Material` with `blendMode == Add` sums its layers — re-opening such a file would sum N copies of the same height. Given STEP101 shipped hours ago on this branch with no real authored content yet, this ticket does not add migration/dedup logic; it adds a `result.Warn(...)` in the re-hydration branch when more than one existing `bBaked`/empty-`bakedImagePath` layer with a distinct `stratumIndex` is found under the same GeoLayer, so a stale file is at least attributable, not silently double-summed. Route an actual fix to the IO Architecture Expert only if a real file surfaces.

### 2. Stratum masks: feed `Data::StratumArt::importedMask`, not height splitting
`LoadStratumMaskTga` (`src/io/MapImporter_Fields_IO.cpp`) already decodes each TGA's four BGRA channels into `outFields.materialProportions[weightIndex]` (STEP101 gap-1's already-fixed, still-correct write — confirmed still load-bearing: `src/proc/Mask_Apply_PROC.cpp` reads `mapFields.materialProportions[stratum]` directly as the procedural-weight input to `ResolveStratumCell`, so **this write does not change**). Add a second, parallel destination fed by the same decoded byte buffer: the TGA's NATIVE resolution (no `sampleSize`/`vertexSize` clipping — `Data::StratumArt::importedMask`'s own contract is "any resolution... the Mask stage resamples it bilinearly," `MASKING_SPEC` §1.8's "Resample inconsistency... unified here, bilinear only"):

```cpp
bool LoadStratumMaskTga(const std::string& filePath, int firstWeightIndex, int sampleSize,
                        Data::MapFields& outFields, std::vector<Data::StratumArt>& outStratumArt,
                        const MapImportOptions& options, MapImportResult& result) {
    // ...unchanged header/dimension validation...
    if (outStratumArt.size() < static_cast<std::size_t>(Data::MapFields::stratumCount))
        outStratumArt.resize(static_cast<std::size_t>(Data::MapFields::stratumCount));

    // Widen the loop bounds to the TGA's own fileWidth/fileHeight (not copySize) so the native-
    // resolution write below is never cropped; the existing vertexSize-clipped materialProportions
    // write stays gated on column/row < sampleSize exactly as before.
    for (int row = 0; row < fileHeight; ++row) {
        const std::size_t fileRow = static_cast<std::size_t>(fileHeight - 1 - row);
        for (int column = 0; column < fileWidth; ++column) {
            const std::size_t pixelStart = tgaHeaderByteSize + (fileRow * fileWidth + column) * 4u;
            for (int channel = 0; channel < 4; ++channel) {
                const int weightIndex = firstWeightIndex + bgraWeightOrder[channel];
                if (weightIndex >= Data::MapFields::stratumCount) continue;
                const float value = static_cast<float>(bytes[pixelStart + channel]) * (1.0f / 255.0f);
                if (column < sampleSize && row < sampleSize)
                    outFields.materialProportions[weightIndex].Set(column, row, value);   // unchanged
                outStratumArt[weightIndex].importedMask.Set(column, row, value);          // STEP105: native res
            }
        }
    }
    // ...
}
```
(`outStratumArt[weightIndex].importedMask` needs `.Resize(fileWidth, fileHeight, 0.0f)` once per weight index before the loop — first write into that stratum's field.)

`LoadBakedFields` (`MapImporter_Fields_IO.cpp`) and `MapImporter::LoadBakedFields`/`LoadSanmap` (`src/io/MapImporter_IO.h`) gain a new nullable `std::vector<Data::StratumArt>* outStratumArt` parameter, exactly mirroring `outBakedLayerImages`'s existing caller-owned/nullable posture. `MapImporter_IO.cpp`'s `LoadSanmap` call site threads it into both `LoadStratumMaskTga` calls.

### 3. Default `importedMaskMode` — only when genuinely unconfigured
Per-stratum, not per-document: use the SAME rule `Params::MapRecipe::strata`'s own header comment already documents ("Shorter than `MapFields::stratumCount` is legal — strata past the end run on their defaults"). When `LoadBakedFields` finds a non-empty imported mask for stratum `s` and `recipe.strata.size() <= s` (this stratum has never been explicitly configured by ANY document section — confirmed `.sanmap`'s `StratumGenerationSettings`/`stratumLayers` DOES round-trip `ImportedMaskMode` explicitly, `src/io/MapImporter_StratumLayers_IO.cpp`, `src/io/MapImporter_Recipe_IO.cpp` — so a genuine SanGen-authored round-tripped map already has an explicit, sized entry and must NOT be touched), grow `recipe.strata` to include index `s` and set `.importedMaskMode = Params::ImportedMaskMode::StaticOverride`. `StaticOverride` is the correct default because it "replaces with the stored art and therefore is NOT slope-gated — it is locked to what the artist shipped" (`Mask_Merge_PROC.h`), i.e. the ONLY mode that reproduces the imported mask exactly, matching the height layer's own exact-reproduction goal. `Disabled`/`ProceduralStart` remain fully available afterward as the designer's own per-stratum toggle — this default only fires once, at first import of a genuinely unconfigured stratum.

This logic lives in `LoadBakedFields`'s tail, after both `LoadStratumMaskTga` calls, iterating the strata that just gained a non-empty `outStratumArt[s].importedMask`.

### 4. `materialProportions` — confirmed still needed, unchanged
Traced live: `Mask_Apply_PROC.cpp` passes `mapFields.materialProportions[stratum]` straight into `ResolveStratumCell` as the gated-procedural half of the merge (`MASKING_SPEC` Part 1's `surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)`), independent of `ImportedMaskMode`/`StaticOverride`'s own separate `storedArt_s` input. STEP101 gap-1's fix (retargeting `LoadStratumMaskTga`'s write from `surfaceStratumWeights` to `materialProportions`) stays exactly as shipped — this ticket only ADDS the parallel native-resolution write into `Data::StratumArt::importedMask`, it does not touch or retarget the existing one.

### Plumbing (mechanical, mirrors `outBakedLayerImages` exactly)
`src/ui/FilesTab_UI.h`, `FilesTab_Actions_UI.cpp` (`RunOpenSanmap`), `FilesTab_MigrationImport_Actions_UI.cpp` (`RunSelectiveMigrationImport`) each gain a `std::vector<Data::StratumArt>* outStratumArt = nullptr` parameter threaded alongside `outBakedLayerImages`, same nullable/caller-owned/move-on-success posture as `RunOpenSanmap`'s existing scratch-then-commit pattern (STEP103). The real caller additionally passes `&assembler.StratumArt()` (`GenerationAssembler_PIPELINE.h`, already exposed, already sized to `stratumCount` in the constructor).

## Files touched
- `src/io/MapImporter_HeightmapDecomposition_IO.h` — header comment rewritten (single-layer contract)
- `src/io/MapImporter_HeightmapDecomposition_IO.cpp` — `ComputeStratumBakedImage` deleted; both branches collapse to one layer
- `src/io/MapImporter_Fields_IO.cpp` — `LoadStratumMaskTga` gains `outStratumArt` param + native-res write; `LoadBakedFields` gains `outStratumArt` param + default-`importedMaskMode` logic
- `src/io/MapImporter_IO.h` (signatures: `LoadBakedFields`, `LoadSanmap`), `src/io/MapImporter_IO.cpp` (call site)
- `src/ui/FilesTab_UI.h`, `src/ui/FilesTab_Actions_UI.cpp`, `src/ui/FilesTab_MigrationImport_Actions_UI.cpp` (plumb `outStratumArt` alongside `outBakedLayerImages`)
- `src/io/MapImporter_HeightmapDecomposition_IO_Test.cpp` — acceptance assertions rewritten (below)

**Not touched:** `src/proc/Mask_Prepare_PROC.cpp`, `Mask_Merge_PROC.{h,glsl}`, `Mask_Apply_PROC.cpp` — the consumer side is already shipped and correct; this is purely an IO-side producer fix. `src/io/MapExporter_*` — export already reads `surfaceStratumWeights` (the Mask stage's resolved output, which under `StaticOverride` already equals the stored art verbatim), so the round-trip needs no exporter change.

## ARCH rules invoked
- Single-writer rule — `materialProportions` and `Data::StratumArt::importedMask` are both LOADED/physical fields IO may write; `surfaceStratumWeights` stays the Mask stage's sole, exclusive output (unchanged, re-affirmed, not violated by the new write).
- "modes/thresholds -> PARAMS, loaded pixels -> DATA" (`StratumArt_DATA.h`) — governs why imported mask pixels land in `Data::StratumArt`, never on `Params::Stratum`.
- "no rival per-stratum settings type" — the default-mode logic writes `recipe.strata[s].importedMaskMode` on the ONE existing `Params::Stratum`, mints nothing new.
- `LAYER_SYSTEM_SPEC` "Stratum 0 = the always-present base" — justifies the single baked layer's default `stratumIndex == 0`.
- `MASKING_SPEC` §1.5 (merge-mode semantics), §1.6 (`materialProportions` vs `surfaceStratumWeights` consumers), §1.8 (bilinear-only resampling, the reason `importedMask` must be stored at native resolution, not clipped).
- Constitution §6 — validate/default/log; the per-stratum default-mode rule never clobbers an explicit document value, and a truncated/undersized TGA still degrades gracefully (unchanged from STEP101).

## Explicit out-of-scope
- The TGA channel/array-index off-by-one (STEP101 gap 2) — still flagged, still not fixed here.
- `bakedImagePath`-sourced re-hydration (STEP102's Import RAW / Bake actions) — untouched.
- Albedo texel loading into `Data::StratumArt` (a separate sanpack-ingestion loader) — untouched.
- Any change to `Mask_Prepare_PROC.cpp`/`Mask_Merge_PROC.{h,glsl}`/`Mask_Apply_PROC.cpp` — already correct, zero PROC changes.
- Migrating/deduping a `.sanmap` already exported under STEP101's per-stratum scheme — flagged above as a `result.Warn(...)`-only mitigation, real fix routed elsewhere if it ever surfaces.
- GeoLayer-level/whole-stack baking, the recursive-GeoLayer redesign, Unified cross-band erosion, the M6 ordered-thickness-column DATA persistence, `FUTURE_SIM_TYPES_SPEC` (carried over from STEP101, still out of scope).

## Acceptance test (revises `MapImporter_HeightmapDecomposition_IO_Test.cpp`)
`WriteSyntheticExternalMap`'s fixture is unchanged (small non-trivial heightfield + one fully-covering stratum-1 mask). Assertions change to:
- `loadedRecipe.layerStack.geoLayers.size() == 1` and `TotalLayerCount() == 1` (was 2) — exactly one baked layer, `stratumIndex == 0`.
- `bakedLayerImages.size() == 1`; its image equals `loadedFields.heightfield` cell-for-cell (not the old masked-by-materialProportions comparison).
- `assembler.Run()` still reproduces the ORIGINAL imported heightfield (same acceptance bar as STEP101; tolerance may tighten since there is no longer a mask-multiply rounding step in the height path).
- NEW: `assembler.StratumArt()[1].HasImportedMask()` is true after import; its `importedMask` dimensions equal the TGA's native `fileWidth`/`fileHeight` (not `vertexSize`-clipped).
- NEW: `loadedRecipe.strata.size() > 1` and `loadedRecipe.strata[1].importedMaskMode == Params::ImportedMaskMode::StaticOverride` (the new sensible default).
- `materialProportions[1]` assertion unchanged (still matches the TGA's R channel — the existing write path is untouched).
- `surfaceStratumWeights[1]` stays untouched immediately after import (unchanged); NEW: after `assembler.Run()`, `surfaceStratumWeights[1]` reproduces the imported mask (proving `ImportedMaskMode::StaticOverride` actually consumes what import fed it, within the Mask stage's own bilinear-resample tolerance) — the real proof this ticket's second half works end-to-end.
- `CheckReimportAfterRoundTripDoesNotDuplicate`: updated to assert 1 layer (not 2) both times, and that the SECOND import's `strata[1].importedMaskMode` still reads `StaticOverride` — proving the round-tripped, now-EXPLICIT document value is read back, not re-defaulted a second time (exercises the "don't clobber an explicit choice" rule).

## Verify
Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green outside the one revised test file; zero unrelated test files edited. No manual/interactive testing — verification is the automated test binary plus a direct re-read of the edited files, per standing project policy.
