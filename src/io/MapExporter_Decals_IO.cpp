// MapExporter_Decals_IO.cpp — `recipe.decals`/`recipe.decalLayers` -> the top-level `.sanmap`
// `decals`/`DecalGroups` JSON. Layer: IO. Own file (not shared with Props): independent top-level
// format key, same split ruling STEP2/STEP3 already applied.
//
// Called from `BuildSanmapJsonText` — see MapExporter_Props_IO.cpp's header note; the same posture
// applies here (STEP5_PropsDecalsValidation_UI live-wired this).
//
// `decals` is the same plain-ARRAY shape as `props` (ENTITY_AUTHORING_PARAMS_SPEC finding 1,
// confirmed `SanMap.cs:157` `DecalType[] decals`). `DecalTransform` composes `InstancedTransform`
// plus `layerIndex`, flattened on the wire exactly like `PropTransform`.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildDecalTransformJson(const Params::DecalTransform& decalTransform, int mapSize) {
    const Params::InstancedTransform& transform = decalTransform.transform;
    nlohmann::ordered_json json;
    // The coordinate flip applies to DecalTransform.transform.positionZ (finding 3, the strongest
    // evidence class — MapUtils.cs:166 actually executes this flip for decals): `world.z = mapSize
    // - positionZ - 1`. positionX/positionY and rotation/scale are untouched by the flip.
    json["position"] = { { "x", transform.positionX }, { "y", transform.positionY },
                         { "z", static_cast<float>(mapSize) - transform.positionZ - 1.0f } };
    // WATCH-ROTATION-FLIP: rotation round-trips verbatim, no coordinate transform (finding 4, same
    // ruling as Steps 2/3) — UNCONFIRMED whether this is correct. If in-game testing shows a
    // placed decal facing the wrong direction, this is where the fix goes. See STEP4_PropsDecals_IO.md.
    json["rotation"] = { { "x", transform.rotationX }, { "y", transform.rotationY },
                         { "z", transform.rotationZ }, { "w", transform.rotationW } };
    json["scale"]    = { { "x", transform.scaleX }, { "y", transform.scaleY }, { "z", transform.scaleZ } };
    json["layerIndex"] = decalTransform.layerIndex;
    return json;
}

} // namespace

nlohmann::ordered_json BuildDecalsJson(const Params::MapRecipe& recipe) {
    const int mapSize = recipe.geometry.mapSize;
    nlohmann::ordered_json decals = nlohmann::ordered_json::array();
    for (const Params::DecalInstanceGroup& group : recipe.decals) {
        nlohmann::ordered_json transforms = nlohmann::ordered_json::array();
        for (const Params::DecalTransform& decalTransform : group.transforms)
            transforms.push_back(BuildDecalTransformJson(decalTransform, mapSize));

        nlohmann::ordered_json groupJson;
        groupJson["blueprintPath"] = group.blueprintPath;
        groupJson["transforms"]    = transforms;
        decals.push_back(groupJson);
    }
    return decals;
}

// `DecalGroups` — SanGen-owned manual-layer metadata, top-level PascalCase array, SANMAP_FORMAT_
// SPEC Correction 14. `Color` reuses the `{r,g,b,a}` shape already shipped for `armyColor` (Step 2).
nlohmann::ordered_json BuildDecalGroupsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json decalGroups = nlohmann::ordered_json::array();
    for (const Params::DecalInstanceLayer& layer : recipe.decalLayers) {
        nlohmann::ordered_json layerJson;
        layerJson["Name"]  = layer.name;
        layerJson["Color"] = { { "r", layer.color[0] }, { "g", layer.color[1] },
                               { "b", layer.color[2] }, { "a", layer.color[3] } };
        layerJson["IconScale"] = layer.iconScale;
        layerJson["Id"] = layer.layerId;
        layerJson["Locked"] = layer.bLocked;
        decalGroups.push_back(layerJson);
    }
    return decalGroups;
}

} // namespace Io
} // namespace SanmapGen
