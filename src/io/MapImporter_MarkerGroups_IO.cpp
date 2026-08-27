// MapImporter_MarkerGroups_IO.cpp — the top-level `MarkerGroups` array -> `recipe.markerLayers`
// (STEP60_MarkerInstanceLayer_PARAMS). Layer: IO. Split out of MapImporter_Markers_IO.cpp (STEP124)
// once that file's own line count — already over ARCH §1.5's 150-line hard ceiling before this
// ticket (162 lines, unremediated since STEP115 first flagged it) — would have crossed further
// with this ticket's own additions (MarkerTypeName here; the InstanceIdentifier legacy-backfill
// counter threaded through the `markers`-dictionary side that stays behind). Mirrors
// MapImporter_MarkerLayerReconcile_IO.cpp's own split-out-of-this-exact-file precedent (STEP115).
// `ReadMarkerGroupsJson` MUST still run before `ReadMarkersJson` (MapImporter_Recipe_IO.h's own
// header comment, unchanged) — this split changes which TRANSLATION UNIT defines the function, not
// the call order in MapImporter_ParseDocument_IO.cpp.
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

// `MarkerGroups` — a plain array walk, same shape as `ReadPropGroupsJson`'s `{r,g,b,a}` read
// (STEP60_MarkerInstanceLayer_PARAMS). `layerId` legacy-backfills by array index — a file with no
// `"Id"` key on an entry lands it on the vector position it was read at.
void ReadMarkerGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("MarkerGroups") || !document["MarkerGroups"].is_array()) return;
    outRecipe.markerLayers.clear();
    for (const nlohmann::json& layerJson : document["MarkerGroups"]) {
        Params::MarkerInstanceLayer layer;
        layer.layerId = static_cast<int>(outRecipe.markerLayers.size());   // legacy-backfill default
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
            ReadJsonInteger(layerJson, "RadialSymmetryRepeatCount", layer.symmetry.radialSymmetryRepeatCount);
            ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
            ReadJsonBoolean(layerJson, "Hidden", layer.bHidden);   // STEP144 — absent (pre-existing
                                                                    // file) leaves the struct default,
                                                                    // false, i.e. visible.
            ReadJsonBoolean(layerJson, "GridSnapEnabled", layer.bGridSnapEnabled);
            ReadJsonFloat(layerJson, "GridSnapSizeWorldUnits", layer.gridSnapSizeWorldUnits);
            ReadJsonBoolean(layerJson, "ColorOverrideEnabled", layer.bColorOverrideEnabled);
            ReadJsonBoolean(layerJson, "SymmetryEnabled", layer.bSymmetryEnabled);
            ReadJsonInteger(layerJson, "ParentBundleIdentifier", layer.parentBundleIdentifier);
            ReadJsonText(layerJson, "MarkerTypeName", layer.markerTypeName);   // NEW, STEP124/ARCH §19.13
        }
        outRecipe.markerLayers.push_back(layer);
    }
}

} // namespace Io
} // namespace SanmapGen
