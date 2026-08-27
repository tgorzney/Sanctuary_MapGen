// MapExporter_Markers_IO.cpp — `recipe.markers` -> the top-level `.sanmap` `markers` dictionary.
// Layer: IO. Own file (not shared with Chains): `markers`/`chains` are independent top-level format
// keys with no shared JSON parent (IO Architecture Expert ruling, applied directly from
// STEP2_ArmiesAreas_IO's `armies`/`areas` precedent — STEP3_MarkersChains_IO).
// Two-level name-keyed-object dictionary (ENTITY_AUTHORING_PARAMS_SPEC finding 2, confirmed
// `SanMap.cs:151`/`SanMap.Types.cs:161-176`): `markers[type] = {resource, transforms[name] = {...}}`.
// `MarkerTransform` COMPOSES `InstancedTransform` (not flattened like `UnitTransform`) — field
// access is `markerTransform.transform.positionX`, but the JSON shape is identical to
// `UnitTransform`'s: `position`/`rotation`/`scale` are top-level siblings of `alias`.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildMarkerTransformJson(const Params::MarkerTransform& markerTransform,
                                                int mapSize) {
    const Params::InstancedTransform& transform = markerTransform.transform;
    nlohmann::ordered_json json;
    // The coordinate flip applies to MarkerTransform.transform.positionZ (finding 4, confirmed
    // directly against MapUtils.cs's ArmySpawnMarker/AlloySpotMarker construction sites — the
    // reference converter actually exercises this path for markers): `world.z = mapSize -
    // positionZ - 1`. positionX/positionY and rotation/scale are untouched by the flip.
    json["position"] = { { "x", transform.positionX }, { "y", transform.positionY },
                         { "z", static_cast<float>(mapSize) - transform.positionZ - 1.0f } };
    // WATCH-ROTATION-FLIP: rotation round-trips verbatim, no coordinate transform (finding 5, same
    // ruling as Step 2) — UNCONFIRMED whether this is correct. If in-game testing shows a placed
    // marker facing the wrong direction, this is where the fix goes. See STEP3_MarkersChains_IO.md.
    json["rotation"] = { { "x", transform.rotationX }, { "y", transform.rotationY },
                         { "z", transform.rotationZ }, { "w", transform.rotationW } };
    json["scale"]    = { { "x", transform.scaleX }, { "y", transform.scaleY }, { "z", transform.scaleZ } };
    json["alias"]    = markerTransform.alias;
    // Correction 16 (STEP68): 0 = ungrouped, sibling of alias, same unconditional-write posture.
    json["symmetryGroupIdentifier"] = markerTransform.symmetryGroupIdentifier;
    // STEP114: empty = use the type default (§4a), same unconditional-write posture as alias.
    json["iconNameOverride"] = markerTransform.iconNameOverride;
    json["InstanceIdentifier"] = markerTransform.instanceIdentifier;
    return json;
}

} // namespace

nlohmann::ordered_json BuildMarkersJson(const Params::MapRecipe& recipe) {
    const int mapSize = recipe.geometry.mapSize;
    nlohmann::ordered_json markers = nlohmann::ordered_json::object();
    for (const Params::MarkerInstanceGroup& group : recipe.markers) {
        nlohmann::ordered_json transforms = nlohmann::ordered_json::object();
        for (const Params::MarkerTransform& markerTransform : group.transforms)
            transforms[markerTransform.name] = BuildMarkerTransformJson(markerTransform, mapSize);

        nlohmann::ordered_json groupJson;
        groupJson["resource"]   = group.bResource;
        groupJson["transforms"] = transforms;
        markers[group.name] = groupJson;
    }
    return markers;
}

// `MarkerGroups` — SanGen-owned manual-layer metadata, top-level PascalCase array (STEP60), a
// fresh sibling of `markers`, mirroring `PropGroups`/`DecalGroups`'s `Name`/`Color`/`IconScale`
// shape plus the `Id` field directly (no separate follow-up ticket, unlike Props/Decals) and
// Correction 16's SymmetrySetting triplet, flattened as sibling keys — the same three spellings
// already live at the per-rule tier (Correction 4) and the MarkersStack tier (Correction 15),
// reused verbatim, not renamed.
nlohmann::ordered_json BuildMarkerGroupsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markerGroups = nlohmann::ordered_json::array();
    for (const Params::MarkerInstanceLayer& layer : recipe.markerLayers) {
        nlohmann::ordered_json layerJson;
        layerJson["Name"]  = layer.name;
        layerJson["Color"] = { { "r", layer.color[0] }, { "g", layer.color[1] },
                               { "b", layer.color[2] }, { "a", layer.color[3] } };
        layerJson["IconScale"] = layer.iconScale;
        layerJson["Id"] = layer.layerId;
        layerJson["SymmetryUseGlobal"] = layer.symmetry.bSymmetryUseGlobal;
        layerJson["SymmetryMask"] = layer.symmetry.symmetryMask;
        layerJson["RadialSymmetryRepeatCount"] = layer.symmetry.radialSymmetryRepeatCount;
        layerJson["Locked"] = layer.bLocked;
        layerJson["Hidden"] = layer.bHidden;   // STEP144
        layerJson["GridSnapEnabled"] = layer.bGridSnapEnabled;
        layerJson["GridSnapSizeWorldUnits"] = layer.gridSnapSizeWorldUnits;
        layerJson["ColorOverrideEnabled"] = layer.bColorOverrideEnabled;
        layerJson["SymmetryEnabled"] = layer.bSymmetryEnabled;
        layerJson["ParentBundleIdentifier"] = layer.parentBundleIdentifier;
        layerJson["MarkerTypeName"] = layer.markerTypeName;
        markerGroups.push_back(layerJson);
    }
    return markerGroups;
}

// `MarkerLayerBundles` — SanGen-owned Group-above-Layer container, top-level PascalCase array
// (ARCH §19, Correction 19), a fresh sibling of `MarkerGroups`/`markers`. Array order is NOT this
// array's identity (unlike MarkerGroups/PropGroups/DecalGroups) — membership/nesting resolve by
// `Identifier`, since a Bundle forest can be reordered/reparented independently of array position.
nlohmann::ordered_json BuildMarkerLayerBundlesJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markerLayerBundles = nlohmann::ordered_json::array();
    for (const Params::MarkerLayerBundle& bundle : recipe.markerLayerBundles) {
        nlohmann::ordered_json bundleJson;
        bundleJson["Identifier"] = bundle.identifier;
        bundleJson["Name"] = bundle.name;
        bundleJson["ParentBundleIdentifier"] = bundle.parentBundleIdentifier;
        bundleJson["MarkerTypeName"] = bundle.markerTypeName;
        bundleJson["AssemblyIdentifier"] = bundle.assemblyIdentifier;
        markerLayerBundles.push_back(bundleJson);
    }
    return markerLayerBundles;
}

} // namespace Io
} // namespace SanmapGen
