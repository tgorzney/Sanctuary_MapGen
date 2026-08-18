// MapImporter_Markers_IO.cpp — the top-level `.sanmap` `markers` dictionary -> `recipe.markers`.
// Layer: IO. The exact inverse of MapExporter_Markers_IO.cpp. `markers` -> `MarkerInstanceGroup.
// transforms`, two levels of name-keyed JSON objects (ENTITY_AUTHORING_PARAMS_SPEC finding 2) — a
// file-local `ReadNameKeyedObject` walker, kept file-local per the IO Architecture Expert ruling
// applied from Step 2 (promote only if a THIRD domain later needs the identical shape; `chains`
// does not, since it is an object-of-arrays, not an object-of-objects).
#include "JsonPrimitives_IO.h"
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
        // Rotation round-trips verbatim, no coordinate transform (finding 5, same ruling as Step 2).
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
}

void ReadMarkerInstanceGroupJson(const nlohmann::json& json, Params::MarkerInstanceGroup& group,
                                 int mapSize) {
    ReadJsonBoolean(json, "resource", group.bResource);
    ReadNameKeyedObject(json, "transforms", group.transforms,
                        [mapSize](const nlohmann::json& transformJson, Params::MarkerTransform& markerTransform) {
                            ReadMarkerTransformJson(transformJson, markerTransform, mapSize);
                        });
}

} // namespace

void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("markers") || !document["markers"].is_object()) return;
    const int mapSize = outRecipe.geometry.mapSize;   // already populated from top-level `width`
                                                       // before this is called — see the "Critical
                                                       // wiring correction" note in MapImporter_IO.cpp.
    ReadNameKeyedObject(document, "markers", outRecipe.markers,
                        [mapSize](const nlohmann::json& groupJson, Params::MarkerInstanceGroup& group) {
                            ReadMarkerInstanceGroupJson(groupJson, group, mapSize);
                        });
}

} // namespace Io
} // namespace SanmapGen
