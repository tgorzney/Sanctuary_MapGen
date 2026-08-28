// MapImporter_PropGroups_IO.cpp — the top-level `PropGroups` array -> `recipe.propLayers`.
// Layer: IO. Split out of MapImporter_Props_IO.cpp (ARCH §20) once full field parity with
// `MarkerInstanceLayer` (mirrors `MapImporter_MarkerGroups_IO.cpp`'s own split-out-of-Markers
// precedent, STEP124, for the identical file-size reason) pushed that file's line count up.
// `ReadPropGroupsJson` MUST still run before `ReadPropsJson` (MapImporter_Props_IO.cpp's own header
// comment, unchanged) — this split changes which translation unit defines the function, not the
// call order in MapImporter_ParseDocument_IO.cpp.
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"   // radialSymmetryRepeatCountMinimum/Maximum

namespace SanmapGen {
namespace Io {

// `PropGroups` — a plain array walk, full field parity with `ReadMarkerGroupsJson`
// (MapImporter_MarkerGroups_IO.cpp) plus the Prop-only `PropTypeName` (ARCH §20).
void ReadPropGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("PropGroups") || !document["PropGroups"].is_array()) return;
    outRecipe.propLayers.clear();
    for (const nlohmann::json& layerJson : document["PropGroups"]) {
        Params::PropInstanceLayer layer;
        layer.layerId = static_cast<int>(outRecipe.propLayers.size());   // legacy-backfill default
        if (layerJson.is_object()) {
            ReadJsonText(layerJson, "Name", layer.name);
            if (layerJson.contains("Color") && layerJson["Color"].is_object()) {
                const nlohmann::json& color = layerJson["Color"];
                ReadJsonFloat(color, "r", layer.color[0]);
                ReadJsonFloat(color, "g", layer.color[1]);
                ReadJsonFloat(color, "b", layer.color[2]);
                ReadJsonFloat(color, "a", layer.color[3]);
            }
            ReadJsonFloat(layerJson, "IconScale", layer.iconScale);
            ReadJsonInteger(layerJson, "Id", layer.layerId);
            ReadJsonBoolean(layerJson, "SymmetryUseGlobal", layer.symmetry.bSymmetryUseGlobal);
            ReadJsonInteger(layerJson, "SymmetryMask", layer.symmetry.symmetryMask);
            ReadJsonIntegerClamped(layerJson, "RadialSymmetryRepeatCount",
                                  Params::radialSymmetryRepeatCountMinimum,
                                  Params::radialSymmetryRepeatCountMaximum,
                                  layer.symmetry.radialSymmetryRepeatCount);
            ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
            ReadJsonBoolean(layerJson, "Hidden", layer.bHidden);
            ReadJsonBoolean(layerJson, "GridSnapEnabled", layer.bGridSnapEnabled);
            ReadJsonFloat(layerJson, "GridSnapSizeWorldUnits", layer.gridSnapSizeWorldUnits);
            ReadJsonBoolean(layerJson, "ColorOverrideEnabled", layer.bColorOverrideEnabled);
            ReadJsonBoolean(layerJson, "SymmetryEnabled", layer.bSymmetryEnabled);
            ReadJsonInteger(layerJson, "ParentBundleIdentifier", layer.parentBundleIdentifier);
            ReadJsonText(layerJson, "PropTypeName", layer.propTypeName);
        }
        outRecipe.propLayers.push_back(layer);
    }
}

} // namespace Io
} // namespace SanmapGen
