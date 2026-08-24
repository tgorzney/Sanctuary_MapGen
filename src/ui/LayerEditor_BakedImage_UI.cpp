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

// Bake toggle: freezes the CURRENT live noise output on the way in, resumes live generation
// (from the still-present recipe) on the way back out. Bake fails, layer untouched, only when the
// layer cannot be found in the stack's own flattened view (disabled layer/group) — there is
// nothing live to snapshot. Unbake never fails.
bool ApplyBakeToggleAction(Params::LayerStack& layerStack, Params::Layer& layer,
                           Pipeline::GenerationAssembler& generationAssembler) {
    if (layer.bBaked) {           // Unbake: resume live generation from the SAME still-present
        layer.bBaked = false;     // noise recipe fields (not one-way) -- bakedImagePath stays as
        return true;              // metadata; the next Run() re-rolls the layer's own noise.
    }
    const std::vector<const Params::Layer*> flatLayers = layerStack.GetFlatLayers();
    std::size_t flatIndex = flatLayers.size();
    for (std::size_t index = 0; index < flatLayers.size(); ++index)
        if (flatLayers[index] == &layer) { flatIndex = index; break; }
    if (flatIndex >= flatLayers.size()) return false;   // a disabled layer has nothing live to bake
    const std::vector<Data::FloatField>& liveNoise = generationAssembler.NoiseBlend().CachedRawNoiseCpu();
    if (flatIndex >= liveNoise.size()) return false;    // NoiseBlend has not run against this stack yet
    if (layer.layerIdentifier < 0) layer.layerIdentifier = Params::NextLayerIdentifier(layerStack);
    Data::BakedLayerImage& image =
        Data::FindOrAddBakedLayerImage(generationAssembler.BakedLayerImages(), layer.layerIdentifier);
    image.image = liveNoise[flatIndex];   // snapshot -- a value copy, not a reference into the cache
    layer.bBaked = true;                  // bakedImagePath stays empty: sourced from live noise,
    return true;                          // not a file.
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
    return false;
}

} // namespace Ui
} // namespace SanmapGen
