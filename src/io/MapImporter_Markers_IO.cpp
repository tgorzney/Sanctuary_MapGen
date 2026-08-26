// MapImporter_Markers_IO.cpp — the top-level `.sanmap` `markers` dictionary -> `recipe.markers`.
// Layer: IO. The exact inverse of MapExporter_Markers_IO.cpp. `markers` -> `MarkerInstanceGroup.
// transforms`, two levels of name-keyed JSON objects (ENTITY_AUTHORING_PARAMS_SPEC finding 2) — a
// file-local `ReadNameKeyedObject` walker, kept file-local per the IO Architecture Expert ruling
// applied from Step 2 (promote only if a THIRD domain later needs the identical shape; `chains`
// does not, since it is an object-of-arrays, not an object-of-objects).
//
// `MarkerGroups` -> `recipe.markerLayers` (STEP60_MarkerInstanceLayer_PARAMS), a plain array walk,
// same shape as `ReadPropGroupsJson` (`MapImporter_Props_IO.cpp`). `ReadMarkerGroupsJson` MUST run
// before `ReadMarkersJson`: the `layerIndex` range-clamp validates against
// `outRecipe.markerLayers.size()`, which `ReadMarkerGroupsJson` populates.
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// One name-keyed JSON object -> one vector, the folded-in dictionary key becoming ItemType::name.
// A non-object entry still yields a default-constructed (named) item instead of aborting the whole
// domain — same shape as MapImporter_Armies_IO.cpp's file-local helper of the same name.
template <typename ItemType, typename ReadOneItemFunction>
void ReadNameKeyedObject(const nlohmann::json& parent, const char* key, std::vector<ItemType>& outItems,
                         ReadOneItemFunction ReadOneItem) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    outItems.clear();
    for (const auto& [name, valueJson] : parent[key].items()) {
        ItemType item;
        item.name = name;
        if (valueJson.is_object()) ReadOneItem(valueJson, item);
        outItems.push_back(item);
    }
}

// `position`/`rotation`/`scale` are nested {x,y,z}/{x,y,z,w} objects, same shape as UnitTransform's
// JSON, but written into the COMPOSED `markerTransform.transform` member (MarkerTransform composes
// InstancedTransform, it does not flatten it). `positionZ` inverts the export-side flip (finding 4):
// `positionZ = mapSize - jsonZ - 1`.
void ReadMarkerTransformJson(const nlohmann::json& json, Params::MarkerTransform& markerTransform,
                             int mapSize) {
    Params::InstancedTransform& transform = markerTransform.transform;
    if (json.contains("position") && json["position"].is_object()) {
        const nlohmann::json& position = json["position"];
        ReadJsonFloat(position, "x", transform.positionX);
        ReadJsonFloat(position, "y", transform.positionY);
        float jsonPositionZ = static_cast<float>(mapSize) - transform.positionZ - 1.0f;
        if (ReadJsonFloat(position, "z", jsonPositionZ))
            transform.positionZ = static_cast<float>(mapSize) - jsonPositionZ - 1.0f;
    }
    if (json.contains("rotation") && json["rotation"].is_object()) {
        const nlohmann::json& rotation = json["rotation"];
        // WATCH-ROTATION-FLIP: rotation round-trips verbatim, no coordinate transform (finding 5,
        // same ruling as Step 2) — UNCONFIRMED whether this is correct. If in-game testing shows a
        // placed marker facing the wrong direction, this is where the fix goes. See
        // STEP3_MarkersChains_IO.md.
        ReadJsonFloat(rotation, "x", transform.rotationX);
        ReadJsonFloat(rotation, "y", transform.rotationY);
        ReadJsonFloat(rotation, "z", transform.rotationZ);
        ReadJsonFloat(rotation, "w", transform.rotationW);
    }
    if (json.contains("scale") && json["scale"].is_object()) {
        const nlohmann::json& scale = json["scale"];
        ReadJsonFloat(scale, "x", transform.scaleX);
        ReadJsonFloat(scale, "y", transform.scaleY);
        ReadJsonFloat(scale, "z", transform.scaleZ);
    }
    ReadJsonText(json, "alias", markerTransform.alias);
    ReadJsonInteger(json, "layerIndex", markerTransform.layerIndex);
    // Correction 16 (STEP68): no range to validate — 0 is always legal, any positive value
    // accepted as-is (unlike layerIndex/radialSymmetryRepeatCount, which clamp on import).
    ReadJsonInteger(json, "symmetryGroupIdentifier", markerTransform.symmetryGroupIdentifier);
    // STEP114: absent key (legacy files) keeps the struct default (empty = type default) — no
    // validation needed, any string is legal (same posture as alias).
    ReadJsonText(json, "iconNameOverride", markerTransform.iconNameOverride);
}

// ARCH §12 / Constitution §6: an out-of-range layerIndex is a loud, logged clamp to 0, applied PER
// INSTANCE while walking `transforms` — different instances in the same file can carry different
// out-of-range values, so this cannot be a single file-scope check. Identical shape to
// `ClampPropLayerIndex` (`MapImporter_Props_IO.cpp`).
// ARCH_12_ManualPropDecalLayers.md: "a missing layerIndex key on an older/foreign file degrades for
// free to 0 (the field's own default)" — 0 is therefore never out-of-range, even against zero
// declared `MarkerGroups` entries (every hand-authored/legacy `markers` group with no manual layers
// at all defaults every transform's layerIndex to 0). Only a genuinely out-of-range POSITIVE or
// negative value clamps and warns.
void ClampMarkerLayerIndex(Params::MarkerTransform& markerTransform, std::size_t markerLayerCount,
                           MapImportResult& result) {
    if (markerTransform.layerIndex == 0) return;
    if (markerTransform.layerIndex > 0
        && static_cast<std::size_t>(markerTransform.layerIndex) < markerLayerCount)
        return;
    result.Warn("Marker transform layerIndex " + std::to_string(markerTransform.layerIndex)
               + " is out of range against " + std::to_string(markerLayerCount)
               + " MarkerGroups entries; clamped to 0.");
    markerTransform.layerIndex = 0;
}

void ReadMarkerInstanceGroupJson(const nlohmann::json& json, Params::MarkerInstanceGroup& group,
                                 int mapSize, std::size_t markerLayerCount, MapImportResult& result) {
    ReadJsonBoolean(json, "resource", group.bResource);
    ReadNameKeyedObject(json, "transforms", group.transforms,
                        [mapSize, markerLayerCount, &result](const nlohmann::json& transformJson,
                                                             Params::MarkerTransform& markerTransform) {
                            ReadMarkerTransformJson(transformJson, markerTransform, mapSize);
                            ClampMarkerLayerIndex(markerTransform, markerLayerCount, result);
                        });
}

} // namespace

// `MarkerGroups` — a plain array walk, same shape as `ReadPropGroupsJson`'s `{r,g,b,a}` read
// (STEP60_MarkerInstanceLayer_PARAMS). `layerId` legacy-backfills by array index — a file with no
// `"Id"` key on an entry lands it on the vector position it was read at, the same convention
// `ReadPropGroupsJson`'s own STEP56-era `Id` retrofit will use once that ticket lands.
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
            // Correction 16. No range to validate (free integers, same tolerance as the per-rule/
            // MarkersStack tiers) — absent keys keep SymmetrySetting's own defaults.
            ReadJsonBoolean(layerJson, "SymmetryUseGlobal", layer.symmetry.bSymmetryUseGlobal);
            ReadJsonInteger(layerJson, "SymmetryMask", layer.symmetry.symmetryMask);
            ReadJsonInteger(layerJson, "RadialSymmetryRepeatCount", layer.symmetry.radialSymmetryRepeatCount);
            ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
            ReadJsonBoolean(layerJson, "GridSnapEnabled", layer.bGridSnapEnabled);
            ReadJsonFloat(layerJson, "GridSnapSizeWorldUnits", layer.gridSnapSizeWorldUnits);
            ReadJsonBoolean(layerJson, "ColorOverrideEnabled", layer.bColorOverrideEnabled);
            ReadJsonInteger(layerJson, "ParentBundleIdentifier", layer.parentBundleIdentifier);
        }
        outRecipe.markerLayers.push_back(layer);
    }
}

void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!document.contains("markers") || !document["markers"].is_object()) return;
    const int mapSize = outRecipe.geometry.mapSize;   // already populated from top-level `width`
                                                       // before this is called — see the "Critical
                                                       // wiring correction" note in MapImporter_IO.cpp.
    const std::size_t markerLayerCount = outRecipe.markerLayers.size();
    ReadNameKeyedObject(document, "markers", outRecipe.markers,
                        [mapSize, markerLayerCount, &result](const nlohmann::json& groupJson,
                                                             Params::MarkerInstanceGroup& group) {
                            ReadMarkerInstanceGroupJson(groupJson, group, mapSize, markerLayerCount, result);
                        });
}

} // namespace Io
} // namespace SanmapGen
