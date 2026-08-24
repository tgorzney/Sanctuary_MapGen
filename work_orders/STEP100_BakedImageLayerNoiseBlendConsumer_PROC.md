# STEP100 — `NoiseBlendStage` reads a baked layer's stored image instead of regenerating it

**Layer:** PROC, DATA. **Domain:** `NoiseBlendStage` (`src/proc/NoiseBlend_*_PROC.*`),
new `Data::BakedLayerImage`. **Sequence:** depends on STEP99 (`Params::Layer::bBaked`/
`layerIdentifier`). Accuracy class: Exact (CPU path only — see Explicit out-of-scope).

## Root problem
`NoiseBlendStage::GenerateLayerNoiseCpu` (`src/proc/NoiseBlend_Noise_PROC.cpp`)
unconditionally runs FastNoiseLite for every flattened layer, every time its
structural hash goes stale, with no way to say "don't — read the frozen pixels
instead." This is the direct cause of the reported bug: nothing in PROC can
distinguish "regenerate this layer" from "this layer's height IS the map I just
imported."

## Architecture call this ticket makes (grounded in existing precedent, not invented)
Baked pixel data is DATA, never PARAMS (`MASKING_SPEC.md` §1.7: loaded TGA pixels
are `Data::FloatField`, never PARAMS). The exact precedent to mirror already
ships: `Data::StratumArt` (`src/data/StratumArt_DATA.h`) — pixels an importer
brings in are loaded input, never part of the recipe, owned by
`GenerationAssembler` as `std::vector<Data::StratumArt> stratumArt`, index-aligned
to `Params::Stratum`, populated by IO, read by PROC through an accessor.

`Params::Layer`s are NOT a fixed 0..8 array like strata — they're a dynamic,
reorderable `std::vector<Layer>` nested inside `std::vector<GeoLayer>` — so a flat
position-indexed cache (like `NoiseBlendStage`'s own private `cachedRawNoiseCpu`,
which is legitimately wiped and rebuilt on ANY stack-shape change) would silently
reattach a baked layer's pixels to the WRONG layer after a reorder. This is why
STEP99 added `Params::Layer::layerIdentifier` (stable, travels with the struct
through copy/move/reorder) instead of relying on position. This is the one
load-bearing design call in this sequence beyond what's already ratified; it is a
narrow, mechanical extension of the `StratumArt` precedent, not a new pattern, but
it has not been separately ARCH-ratified — flagged for the ARCH Expert's
visibility, not blocking (the shape is small and reversible).

## Solution — shape
```cpp
// src/data/BakedLayerImage_DATA.h — new file
#pragma once
#include <vector>
#include "FloatField_DATA.h"

namespace SanmapGen { namespace Data {

struct BakedLayerImage {
    int        layerIdentifier = -1;   // Params::Layer::layerIdentifier this belongs to
    FloatField image;                  // 0..1 normalized height contribution
};

// Linear scan — layer counts are tens, not thousands.
inline const FloatField* FindBakedLayerImage(const std::vector<BakedLayerImage>& images,
                                             int layerIdentifier) {
    if (layerIdentifier < 0) return nullptr;
    for (const BakedLayerImage& entry : images)
        if (entry.layerIdentifier == layerIdentifier) return &entry.image;
    return nullptr;
}

}} // namespace SanmapGen::Data
```
`GenerationAssembler_PIPELINE.h`: new member `std::vector<Data::BakedLayerImage>
bakedLayerImages;`, declared alongside `stratumArt` (before `mapFields`, same
"loaded art the stages read" grouping), plus an accessor:
```cpp
std::vector<Data::BakedLayerImage>& BakedLayerImages() { return bakedLayerImages; }
```
`GenerationAssembler_PIPELINE.cpp`'s constructor threads it into `noiseBlendStage`'s
init:
```cpp
noiseBlendStage(recipeSettings.geometry, recipeSettings.layerStack, mapFields, bakedLayerImages),
```
`NoiseBlend_PROC.h`: constructor gains the 4th param; new private reference member
`const std::vector<Data::BakedLayerImage>& bakedLayerImages;` (declared alongside
`geometry`/`layerStack`/`mapFields`).

**No change to `LayerKernelConfiguration` or any `.glsl` kernel.** The baked/live
decision is resolved entirely inside the CPU-only `GenerateLayerNoiseCpu` — which
already has `layerStack` as a stage member and can consult the ORIGINAL
`Params::Layer` for a flat index exactly like `ComputeStructuralNoiseHash` already
does — so the shared std430 layout and the GPU speed path are untouched. This
deliberately defers GPU support for baked layers (see Explicit out-of-scope).

`NoiseBlend_Noise_PROC.cpp`'s `GenerateLayerNoiseCpu(layerIndex)`:
```cpp
void NoiseBlendStage::GenerateLayerNoiseCpu(std::size_t layerIndex) {
    const std::vector<const Params::Layer*> flatLayers = layerStack.GetFlatLayers();
    Data::FloatField& target = cachedRawNoiseCpu[layerIndex];
    if (layerIndex < flatLayers.size() && flatLayers[layerIndex]->bBaked) {
        const Data::FloatField* baked =
            Data::FindBakedLayerImage(bakedLayerImages, flatLayers[layerIndex]->layerIdentifier);
        if (baked == nullptr) { target.Fill(0.0f); return; }   // baked, no image yet — safe flat
                                                                 // fallback (Constitution §6),
                                                                 // never a crash
        const int vertexSize = target.Width();
        if (baked->Width() == vertexSize && baked->Height() == vertexSize) {
            target = *baked;                                    // exact copy, common case
        } else {                                                 // map resized since baking —
            const float scaleX = (baked->Width()  - 1) / static_cast<float>(vertexSize - 1);
            const float scaleY = (baked->Height() - 1) / static_cast<float>(vertexSize - 1);
            for (int y = 0; y < vertexSize; ++y)
                for (int x = 0; x < vertexSize; ++x)
                    target.Set(x, y, baked->SampleBilinear(x * scaleX, y * scaleY));
        }
        return;
    }
    // ...existing FastNoiseLite path, unchanged...
}
```
`NoiseBlend_PROC.cpp`'s `HashLayerStructure` gains two lines so a bake/unbake
toggle or an identifier change correctly invalidates/reuses the two-level cache:
```cpp
seed = HashInteger(seed, layer.bBaked ? 1 : 0);
seed = HashInteger(seed, layer.layerIdentifier);
```
`bakedImagePath` is deliberately NOT hashed here — it is IO/UI pass-through
metadata PROC never reads; only `bBaked`/`layerIdentifier` (which govern what
`GenerateLayerNoiseCpu` actually does) belong in the structural hash.

**Reshape/blend need no change at all.** `ReshapeLayerValue`/`ApplyLayerToHeight`
already implement LAYER_SYSTEM_SPEC's "imported = levels/contrast/brightness +
blend + Min/Max only" for free, as long as a baked layer's density fields stay at
their struct defaults (`landDensity=0.5, mountainDensity=0, plateauDensity=0,
rampDensity=0` — with these defaults `ReshapeLayerValue` reduces to
`shaped = raw`, a pure pass-through) — IO/UI (STEP101/102) must leave those fields
at their `Params::Layer` defaults when constructing a baked layer; PROC enforces
nothing extra.

**New public accessor for STEP102's "Bake a live noise layer" path** (freezing a
layer's CURRENT computed output, not importing a new image):
```cpp
// NoiseBlend_PROC.h, public section
const std::vector<Data::FloatField>& CachedRawNoiseCpu() const { return cachedRawNoiseCpu; }
```
So `LayerEditor_Group_UI.cpp`'s Bake action (STEP102) can snapshot
`assembler.NoiseBlend().CachedRawNoiseCpu()[flatIndex]` into a new
`Data::BakedLayerImage` before flipping `bBaked = true`.

## Explicit out-of-scope
- **The GPU kernel twin.** A baked layer forces the CPU path —
  `NoiseBlendStage` must refuse/fall back to CPU when any flattened layer has
  `bBaked == true` (a documented dispatch constraint, not a crash; wiring this
  refusal into `RunOnGpu()`/`IsGpuAvailable()` is this ticket's job, the GPU
  kernel itself is not). Uploading per-layer baked textures to a GPU resource is
  Compute Optimization Expert territory.
- **`Data::BakedLayerImage` producers** (populating the vector) — STEP101 (import
  decomposition) and STEP102 (Import RAW / Bake UI actions).
- Any GeoLayer-level or whole-stack bake.
- ARCH ratification of the `layerIdentifier`-keyed DATA cache pattern itself —
  flagged above; this ticket proceeds on the `StratumArt` precedent without
  blocking on a formal ruling.

## Files touched
- `src/data/BakedLayerImage_DATA.h` (new)
- `src/pipeline/GenerationAssembler_PIPELINE.h`
- `src/pipeline/GenerationAssembler_PIPELINE.cpp`
- `src/proc/NoiseBlend_PROC.h`
- `src/proc/NoiseBlend_PROC.cpp`
- `src/proc/NoiseBlend_Noise_PROC.cpp`

## Acceptance test
A `LayerStack` with one `bBaked=true` layer (`layerIdentifier=0`) and a matching
`Data::BakedLayerImage{0, <known pattern>}` handed to `NoiseBlendStage`: after
`RunOnCpu()`, `mapFields.heightfield` reproduces the known pattern exactly (same
resolution) and bilinear-resampled (map resized). A second `RunOnCpu()` with
nothing changed is a no-op (cache-skip, matching the existing cache-skip test
posture in `NoiseBlend_PROC_Test.cpp`). Toggling `bBaked` off on a layer that ALSO
has live noise settings resumes generating noise identical to a never-baked layer
with the same settings (proves "not one-way"). A `bBaked=true` layer with no
matching `layerIdentifier` in `bakedLayerImages` produces a flat (all-zero)
contribution, never a crash. `RunOnGpu()` on a stack containing a baked layer
falls back to CPU.

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green, zero
unrelated test files edited.
