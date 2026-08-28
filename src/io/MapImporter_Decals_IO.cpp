// MapImporter_Decals_IO.cpp — the top-level `.sanmap` `decals`/`DecalGroups` JSON ->
// `recipe.decals`/`recipe.decalLayers`. Layer: IO. The exact inverse of MapExporter_Decals_IO.cpp.
//
// Called from `ParseSanmapJsonText` — see MapImporter_Props_IO.cpp's header note; the same posture
// applies here. `ReadDecalGroupsJson` (MapImporter_DecalGroups_IO.cpp, ARCH §20 split) MUST run
// before `ReadDecalsJson` when both are used, for the same `layerIndex` range-clamp reason.
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../sys/PathStem_SYS.h"

namespace SanmapGen {
namespace Io {
namespace {

// `position`/`rotation`/`scale` are nested {x,y,z}/{x,y,z,w} objects, `layerIndex` a flattened
// sibling — written into the COMPOSED `decalTransform.transform` member. `positionZ` inverts the
// export-side flip (finding 3): `positionZ = mapSize - jsonZ - 1`.
void ReadDecalTransformJson(const nlohmann::json& json, Params::DecalTransform& decalTransform, int mapSize,
                            int& inOutNextInstanceIdentifier) {
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
    // Correction 16-equivalent (ARCH §21.4): no range to validate — 0 is always legal.
    ReadJsonInteger(json, "SymmetryGroupIdentifier", decalTransform.symmetryGroupIdentifier);
    // ARCH §21.4 — legacy-backfill mirrors ReadMarkerTransformJson's own precedent exactly.
    decalTransform.instanceIdentifier = inOutNextInstanceIdentifier++;
    ReadJsonInteger(json, "InstanceIdentifier", decalTransform.instanceIdentifier);
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
                                int mapSize, std::size_t decalLayerCount, MapImportResult& result,
                                int& inOutNextInstanceIdentifier) {
    ReadJsonText(json, "blueprintPath", group.blueprintPath);
    if (!json.contains("transforms") || !json["transforms"].is_array()) return;
    for (const nlohmann::json& transformJson : json["transforms"]) {
        if (!transformJson.is_object()) continue;
        Params::DecalTransform decalTransform;
        ReadDecalTransformJson(transformJson, decalTransform, mapSize, inOutNextInstanceIdentifier);
        ClampDecalLayerIndex(decalTransform, decalLayerCount, result);
        group.transforms.push_back(decalTransform);
    }
}

} // namespace

void ReadDecalsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!document.contains("decals") || !document["decals"].is_array()) return;
    const int mapSize = outRecipe.geometry.mapSize;
    const std::size_t decalLayerCount = outRecipe.decalLayers.size();
    // ARCH §21.4 — threaded across the ENTIRE array walk below, never reset per group.
    int nextInstanceIdentifier = 0;
    outRecipe.decals.clear();
    for (const nlohmann::json& groupJson : document["decals"]) {
        if (!groupJson.is_object()) continue;
        Params::DecalInstanceGroup group;
        ReadDecalInstanceGroupJson(groupJson, group, mapSize, decalLayerCount, result, nextInstanceIdentifier);
        outRecipe.decals.push_back(group);
    }
}

// STEP115: mirrors ReconcilePropLayers (MapImporter_Props_IO.cpp)/ReconcileMarkerLayers
// (MapImporter_MarkerLayerReconcile_IO.cpp) — same problem (`DecalGroups` is SanGen-invented, never
// present on a real file). One DecalInstanceLayer synthesized per `outRecipe.decals` GROUP entry —
// `decals` is a plain ORDERED ARRAY, so two entries sharing the same blueprintPath legitimately get
// two separate layers, never collapsed by name.
void ReconcileDecalLayers(Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!outRecipe.decalLayers.empty() || outRecipe.decals.empty()) return;
    for (Params::DecalInstanceGroup& group : outRecipe.decals) {
        Params::DecalInstanceLayer layer;
        layer.name = Sys::FileStemFromPath(group.blueprintPath);
        const int newLayerIndex = static_cast<int>(outRecipe.decalLayers.size());
        layer.layerId = newLayerIndex;
        outRecipe.decalLayers.push_back(layer);
        for (Params::DecalTransform& transform : group.transforms)
            transform.layerIndex = newLayerIndex;
    }
    result.Warn("No DecalGroups section present; synthesized "
               + std::to_string(outRecipe.decalLayers.size())
               + " decal layer(s) from the existing decal blueprint group(s) so the Manual Decal"
                 " Layers tooling has something to show.");
}

} // namespace Io
} // namespace SanmapGen
