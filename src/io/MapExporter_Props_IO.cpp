// MapExporter_Props_IO.cpp — `recipe.props`/`recipe.propLayers` -> the top-level `.sanmap` `props`/
// `PropGroups` JSON. Layer: IO. Own file (not shared with Decals): `props`/`decals` are independent
// top-level format keys with no shared JSON parent, same split ruling STEP2/STEP3 already applied.
//
// Called from `BuildSanmapJsonText` (MapExporter_Recipe_IO.cpp), live-wired by
// STEP5_PropsDecalsValidation_UI. `blueprintPath` resolution against a loaded sanpack is a
// separate, sibling pre-flight step (`Io::ValidatePropAndDecalBlueprintPaths`), not this builder.
//
// `props` is a plain ARRAY, not a dictionary (ENTITY_AUTHORING_PARAMS_SPEC finding 1, confirmed
// `SanMap.cs:153` `PropType[] props`) — `PropInstanceGroup`/`PropTransform` have no folded-in name.
// `PropTransform` composes `InstancedTransform` plus `layerIndex`; on the wire, `position`/
// `rotation`/`scale`/`layerIndex` are flattened as siblings in one JSON object, the same
// flattening-on-the-wire pattern `MarkerTransform` already established.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildPropTransformJson(const Params::PropTransform& propTransform, int mapSize) {
    const Params::InstancedTransform& transform = propTransform.transform;
    nlohmann::ordered_json json;
    // The coordinate flip applies to PropTransform.transform.positionZ (finding 3, evidenced via
    // the identical-but-disabled MapUtils.cs prop construction path): `world.z = mapSize -
    // positionZ - 1`. positionX/positionY and rotation/scale are untouched by the flip.
    json["position"] = { { "x", transform.positionX }, { "y", transform.positionY },
                         { "z", static_cast<float>(mapSize) - transform.positionZ - 1.0f } };
    // WATCH-ROTATION-FLIP: rotation round-trips verbatim, no coordinate transform (finding 4, same
    // ruling as Steps 2/3) — UNCONFIRMED whether this is correct. If in-game testing shows a
    // placed prop facing the wrong direction, this is where the fix goes. See STEP4_PropsDecals_IO.md.
    json["rotation"] = { { "x", transform.rotationX }, { "y", transform.rotationY },
                         { "z", transform.rotationZ }, { "w", transform.rotationW } };
    json["scale"]    = { { "x", transform.scaleX }, { "y", transform.scaleY }, { "z", transform.scaleZ } };
    json["layerIndex"] = propTransform.layerIndex;
    return json;
}

} // namespace

nlohmann::ordered_json BuildPropsJson(const Params::MapRecipe& recipe) {
    const int mapSize = recipe.geometry.mapSize;
    nlohmann::ordered_json props = nlohmann::ordered_json::array();
    for (const Params::PropInstanceGroup& group : recipe.props) {
        nlohmann::ordered_json transforms = nlohmann::ordered_json::array();
        for (const Params::PropTransform& propTransform : group.transforms)
            transforms.push_back(BuildPropTransformJson(propTransform, mapSize));

        nlohmann::ordered_json groupJson;
        groupJson["blueprintPath"] = group.blueprintPath;
        groupJson["Reclaimable"]   = group.bReclaimable;
        groupJson["transforms"]    = transforms;
        props.push_back(groupJson);
    }
    return props;
}

// `PropGroups` — SanGen-owned manual-layer metadata, top-level PascalCase array, SANMAP_FORMAT_SPEC
// Correction 14. `Color` reuses the `{r,g,b,a}` shape already shipped for `armyColor` (Step 2).
nlohmann::ordered_json BuildPropGroupsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json propGroups = nlohmann::ordered_json::array();
    for (const Params::PropInstanceLayer& layer : recipe.propLayers) {
        nlohmann::ordered_json layerJson;
        layerJson["Name"]  = layer.name;
        layerJson["Color"] = { { "r", layer.color[0] }, { "g", layer.color[1] },
                               { "b", layer.color[2] }, { "a", layer.color[3] } };
        layerJson["IconScale"] = layer.iconScale;
        propGroups.push_back(layerJson);
    }
    return propGroups;
}

} // namespace Io
} // namespace SanmapGen
