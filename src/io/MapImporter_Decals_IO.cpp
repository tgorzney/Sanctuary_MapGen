// MapImporter_Decals_IO.cpp — the top-level `.sanmap` `decals`/`DecalGroups` JSON ->
// `recipe.decals`/`recipe.decalLayers`. Layer: IO. The exact inverse of MapExporter_Decals_IO.cpp.
//
// Called from `ParseSanmapJsonText` — see MapImporter_Props_IO.cpp's header note; the same posture
// applies here. `ReadDecalGroupsJson` MUST run before `ReadDecalsJson` when both are used, for the
// same `layerIndex` range-clamp reason.
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// `position`/`rotation`/`scale` are nested {x,y,z}/{x,y,z,w} objects, `layerIndex` a flattened
// sibling — written into the COMPOSED `decalTransform.transform` member. `positionZ` inverts the
// export-side flip (finding 3): `positionZ = mapSize - jsonZ - 1`.
void ReadDecalTransformJson(const nlohmann::json& json, Params::DecalTransform& decalTransform, int mapSize) {
    Params::InstancedTransform& transform = decalTransform.transform;
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
        // WATCH-ROTATION-FLIP: rotation round-trips verbatim, no coordinate transform (finding 4,
        // same ruling as Steps 2/3) — UNCONFIRMED whether this is correct. If in-game testing shows
        // a placed decal facing the wrong direction, this is where the fix goes. See
        // STEP4_PropsDecals_IO.md.
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
    ReadJsonInteger(json, "layerIndex", decalTransform.layerIndex);
}

// ARCH §12 / Constitution §6: an out-of-range layerIndex is a loud, logged clamp to 0, applied
// PER INSTANCE while walking `transforms`.
void ClampDecalLayerIndex(Params::DecalTransform& decalTransform, std::size_t decalLayerCount,
                          MapImportResult& result) {
    if (decalTransform.layerIndex >= 0
        && static_cast<std::size_t>(decalTransform.layerIndex) < decalLayerCount)
        return;
    result.Warn("Decal transform layerIndex " + std::to_string(decalTransform.layerIndex)
               + " is out of range against " + std::to_string(decalLayerCount)
               + " DecalGroups entries; clamped to 0.");
    decalTransform.layerIndex = 0;
}

void ReadDecalInstanceGroupJson(const nlohmann::json& json, Params::DecalInstanceGroup& group,
                                int mapSize, std::size_t decalLayerCount, MapImportResult& result) {
    ReadJsonText(json, "blueprintPath", group.blueprintPath);
    if (!json.contains("transforms") || !json["transforms"].is_array()) return;
    for (const nlohmann::json& transformJson : json["transforms"]) {
        if (!transformJson.is_object()) continue;
        Params::DecalTransform decalTransform;
        ReadDecalTransformJson(transformJson, decalTransform, mapSize);
        ClampDecalLayerIndex(decalTransform, decalLayerCount, result);
        group.transforms.push_back(decalTransform);
    }
}

} // namespace

void ReadDecalsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!document.contains("decals") || !document["decals"].is_array()) return;
    const int mapSize = outRecipe.geometry.mapSize;
    const std::size_t decalLayerCount = outRecipe.decalLayers.size();
    outRecipe.decals.clear();
    for (const nlohmann::json& groupJson : document["decals"]) {
        if (!groupJson.is_object()) continue;
        Params::DecalInstanceGroup group;
        ReadDecalInstanceGroupJson(groupJson, group, mapSize, decalLayerCount, result);
        outRecipe.decals.push_back(group);
    }
}

// `DecalGroups` — a plain array walk, same `{r,g,b,a}` Color shape as `ReadArmyColorJson`
// (STEP2_ArmiesAreas_IO), reused verbatim (finding 5).
void ReadDecalGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("DecalGroups") || !document["DecalGroups"].is_array()) return;
    outRecipe.decalLayers.clear();
    for (const nlohmann::json& layerJson : document["DecalGroups"]) {
        Params::DecalInstanceLayer layer;
        layer.layerId = static_cast<int>(outRecipe.decalLayers.size());   // legacy-backfill default
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
            ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
        }
        outRecipe.decalLayers.push_back(layer);
    }
}

} // namespace Io
} // namespace SanmapGen
