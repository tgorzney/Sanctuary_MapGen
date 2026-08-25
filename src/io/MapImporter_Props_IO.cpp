// MapImporter_Props_IO.cpp — the top-level `.sanmap` `props`/`PropGroups` JSON ->
// `recipe.props`/`recipe.propLayers`. Layer: IO. The exact inverse of MapExporter_Props_IO.cpp.
//
// Called from `ParseSanmapJsonText` (MapImporter_IO.cpp), live-wired by
// STEP5_PropsDecalsValidation_UI.
//
// `props` is a plain ARRAY (finding 1) — a plain array walk, no name-keyed-object helper (that
// pattern doesn't apply here; `PropInstanceGroup`/`PropTransform` have no folded-in name).
// `ReadPropGroupsJson` MUST run before `ReadPropsJson` when both are used: the `layerIndex`
// range-clamp (ARCH §12) validates against `outRecipe.propLayers.size()`, which `ReadPropGroupsJson`
// populates.
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// `position`/`rotation`/`scale` are nested {x,y,z}/{x,y,z,w} objects, `layerIndex` a flattened
// sibling — written into the COMPOSED `propTransform.transform` member (PropTransform composes
// InstancedTransform, it does not flatten it). `positionZ` inverts the export-side flip (finding 3):
// `positionZ = mapSize - jsonZ - 1`.
void ReadPropTransformJson(const nlohmann::json& json, Params::PropTransform& propTransform, int mapSize) {
    Params::InstancedTransform& transform = propTransform.transform;
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
        // a placed prop facing the wrong direction, this is where the fix goes. See
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
    ReadJsonInteger(json, "layerIndex", propTransform.layerIndex);
}

// ARCH §12 / Constitution §6: an out-of-range layerIndex is a loud, logged clamp to 0, applied
// PER INSTANCE while walking `transforms` — different instances in the same file can carry
// different out-of-range values, so this cannot be a single file-scope check.
void ClampPropLayerIndex(Params::PropTransform& propTransform, std::size_t propLayerCount,
                         MapImportResult& result) {
    if (propTransform.layerIndex >= 0
        && static_cast<std::size_t>(propTransform.layerIndex) < propLayerCount)
        return;
    result.Warn("Prop transform layerIndex " + std::to_string(propTransform.layerIndex)
               + " is out of range against " + std::to_string(propLayerCount)
               + " PropGroups entries; clamped to 0.");
    propTransform.layerIndex = 0;
}

void ReadPropInstanceGroupJson(const nlohmann::json& json, Params::PropInstanceGroup& group,
                               int mapSize, std::size_t propLayerCount, MapImportResult& result) {
    ReadJsonText(json, "blueprintPath", group.blueprintPath);
    ReadJsonBoolean(json, "Reclaimable", group.bReclaimable);
    if (!json.contains("transforms") || !json["transforms"].is_array()) return;
    for (const nlohmann::json& transformJson : json["transforms"]) {
        if (!transformJson.is_object()) continue;
        Params::PropTransform propTransform;
        ReadPropTransformJson(transformJson, propTransform, mapSize);
        ClampPropLayerIndex(propTransform, propLayerCount, result);
        group.transforms.push_back(propTransform);
    }
}

} // namespace

void ReadPropsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!document.contains("props") || !document["props"].is_array()) return;
    const int mapSize = outRecipe.geometry.mapSize;
    const std::size_t propLayerCount = outRecipe.propLayers.size();
    outRecipe.props.clear();
    for (const nlohmann::json& groupJson : document["props"]) {
        if (!groupJson.is_object()) continue;
        Params::PropInstanceGroup group;
        ReadPropInstanceGroupJson(groupJson, group, mapSize, propLayerCount, result);
        outRecipe.props.push_back(group);
    }
}

// `PropGroups` — a plain array walk, same shape as `ReadArmyColorJson`'s `{r,g,b,a}` read
// (STEP2_ArmiesAreas_IO), reused verbatim for `Color` (finding 5).
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
            ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
        }
        outRecipe.propLayers.push_back(layer);
    }
}

} // namespace Io
} // namespace SanmapGen
