// LayerEditor_BakedImage_UI.cpp — ApplyBakedImageAction's one body. Layer: UI.
// See LayerEditor_BakedImage_UI.h for the contract; this file owns the one IO dependency it needs.
#include "LayerEditor_BakedImage_UI.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../io/MapImporter_IO.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include <cstddef>
#include <utility>

namespace SanmapGen {
namespace Ui {
namespace {

// Import RAW: reads the picked file straight into a fresh Data::BakedLayerImage keyed on the
// layer's (possibly just-assigned) identifier. False, layer untouched, when Io::MapImporter
// refuses the file (Constitution §6 — the reason lands in `result.debugLog`, not surfaced by this
// return value; the picker's own extension fence already keeps most bad paths from reaching here).
bool ApplyImportRawAction(const LayerEditorAction& action, Params::LayerStack& layerStack,
                          Params::Layer& layer, Pipeline::GenerationAssembler& generationAssembler) {
    const int vertexSize = generationAssembler.Fields().VertexSize();
    Data::FloatField loaded;
    const Io::MapImportOptions options;   // caller-tunable safety limits, same defaults RunOpenSanmap uses
    Io::MapImportResult result;
    if (!Io::MapImporter::LoadRawHeightmapIntoField(action.importRawPath, vertexSize, loaded,
                                                    options, result))
        return false;
    if (layer.layerIdentifier < 0) layer.layerIdentifier = Params::NextLayerIdentifier(layerStack);
    Data::BakedLayerImage& image =
        Data::FindOrAddBakedLayerImage(generationAssembler.BakedLayerImages(), layer.layerIdentifier);
    image.image = std::move(loaded);
    layer.bakedImagePath = action.importRawPath;
    layer.bBaked = true;
    return true;
}

// Identity-safe live-noise lookup (STEP151 fix): finds `layer` by POINTER IDENTITY in
// NoiseBlend's own `CachedFlatLayerPointers()`, built in the SAME `PrepareRun()` call as
// `CachedRawNoiseCpu()` -- the two are provably index-for-index in lockstep. Deliberately NOT a
// freshly recomputed `layerStack.GetFlatLayers()` (silently reattaches to the wrong layer after a
// stack reorder) and deliberately NOT `layerIdentifier`-keyed (every never-baked layer shares the
// -1 sentinel, so an identifier-keyed lookup is ambiguous exactly for a first bake). Returns
// `CachedRawNoiseCpu().size()` -- an out-of-range index -- when `layer` is not in the cache
// (NoiseBlend has not run against this exact stack shape yet, or it changed since); callers refuse
// rather than guess.
std::size_t FindLiveNoiseSlot(const Params::Layer& layer,
                              Pipeline::GenerationAssembler& generationAssembler) {
    const std::vector<const Params::Layer*>& cachedPointers =
        generationAssembler.NoiseBlend().CachedFlatLayerPointers();
    for (std::size_t index = 0; index < cachedPointers.size(); ++index)
        if (cachedPointers[index] == &layer) return index;
    return generationAssembler.NoiseBlend().CachedRawNoiseCpu().size();
}

// The one place that actually overwrites a Data::BakedLayerImage's pixels with live noise -- used
// by the toggle's genuine-first-bake path and by Refresh Bake, never by anything else (STEP151).
// False, nothing written, when the identity lookup above refuses.
bool SnapshotLiveNoiseOverImage(Params::LayerStack& layerStack, Params::Layer& layer,
                                Pipeline::GenerationAssembler& generationAssembler) {
    const std::size_t slot = FindLiveNoiseSlot(layer, generationAssembler);
    const std::vector<Data::FloatField>& liveNoise = generationAssembler.NoiseBlend().CachedRawNoiseCpu();
    if (slot >= liveNoise.size()) return false;   // NoiseBlend has not run against this stack yet
    if (layer.layerIdentifier < 0) layer.layerIdentifier = Params::NextLayerIdentifier(layerStack);
    Data::BakedLayerImage& image =
        Data::FindOrAddBakedLayerImage(generationAssembler.BakedLayerImages(), layer.layerIdentifier);
    image.image = liveNoise[slot];   // snapshot -- a value copy, not a reference into the cache
    return true;
}

// Bake toggle: NEVER destroys data (STEP151 -- the human hit the old unconditional-overwrite bug
// first-hand). Off -> On reuses an existing snapshot verbatim (import, or an earlier bake) and
// only ever snapshots live noise for a genuine first-ever bake; On -> Off is unchanged. Bake
// fails, layer untouched, only when there is nothing live to snapshot for a genuine first bake.
bool ApplyBakeToggleAction(Params::LayerStack& layerStack, Params::Layer& layer,
                           Pipeline::GenerationAssembler& generationAssembler) {
    if (layer.bBaked) {           // Unbake: resume live generation from the SAME still-present
        layer.bBaked = false;     // noise recipe fields (not one-way) -- bakedImagePath stays as
        return true;              // metadata; the next Run() re-rolls the layer's own noise.
    }
    // A stable identifier can only already carry content if it was assigned before THIS call
    // (FindBakedLayerImage refuses a negative key outright) -- a genuine first bake, still at -1,
    // can never mistake this branch for its own, and this path writes nothing at all.
    const Data::FloatField* existingImage =
        Data::FindBakedLayerImage(generationAssembler.BakedLayerImages(), layer.layerIdentifier);
    if (existingImage != nullptr && existingImage->Width() > 0) {
        layer.bBaked = true;      // reuse verbatim -- Bake/Unbake/Bake never destroys the original
        return true;
    }
    if (!SnapshotLiveNoiseOverImage(layerStack, layer, generationAssembler)) return false;
    layer.bBaked = true;          // bakedImagePath stays empty: sourced from live noise, not a file.
    return true;
}

// Refresh Bake (STEP151): the ONLY action allowed to deliberately overwrite an existing snapshot.
// Only meaningful on an unbaked layer with a live recipe -- `NoiseType::None` has nothing to
// refresh from (the same predicate STEP152's `HasActiveProceduralLayer()` needs; do not duplicate
// it if that lands first). Does not itself flip `bBaked` either way.
bool ApplyRefreshBakeAction(Params::LayerStack& layerStack, Params::Layer& layer,
                            Pipeline::GenerationAssembler& generationAssembler) {
    if (layer.bBaked || layer.noiseType == Params::NoiseType::None) return false;
    return SnapshotLiveNoiseOverImage(layerStack, layer, generationAssembler);
}

} // namespace

bool ApplyBakedImageAction(const LayerEditorAction& action, Params::LayerStack& layerStack,
                           Pipeline::GenerationAssembler& generationAssembler) {
    if (!LayerEditorActionNamesLayer(layerStack, action.geoLayerIndex, action.layerIndex)) return false;
    Params::Layer& layer =
        layerStack.geoLayers[static_cast<std::size_t>(action.geoLayerIndex)]
                  .layers[static_cast<std::size_t>(action.layerIndex)];

    if (action.kind == LayerEditorActionKind::ImportRawRequested)
        return ApplyImportRawAction(action, layerStack, layer, generationAssembler);
    if (action.kind == LayerEditorActionKind::BakeToggleRequested)
        return ApplyBakeToggleAction(layerStack, layer, generationAssembler);
    if (action.kind == LayerEditorActionKind::RefreshBakeRequested)
        return ApplyRefreshBakeAction(layerStack, layer, generationAssembler);
    return false;
}

} // namespace Ui
} // namespace SanmapGen
