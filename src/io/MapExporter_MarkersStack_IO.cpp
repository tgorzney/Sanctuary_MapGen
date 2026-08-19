// MapExporter_MarkersStack_IO.cpp — `recipe.markerRules` -> the top-level `MarkersStack` array,
// plus `recipe.globalMarkerSettings` -> the top-level `GlobalMarkerSettings` object.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 7 (ruling #1: a bare top-level array, same shape as
// `PropGroups`/`DecalGroups`/`StratumGenerationSettings`) + ARCH §11 (ruling #2: `GlobalMarkerSettings`
// is its own top-level key, a SIBLING of `MarkersStack`, not nested inside it — a JSON array cannot
// structurally host a nested key). `BuildMarkerRuleJson`'s body is relocated verbatim from the
// deleted MapExporter_Rules_IO.cpp; only its container changed.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildMarkerRuleJson(const Params::MarkerRule& rule) {
    nlohmann::ordered_json json;
    json["Enabled"] = rule.bEnabled;   json["Hidden"] = rule.bHidden;
    json["Category"] = static_cast<int>(rule.category);
    json["MinSlope"] = rule.minSlope;  json["MaxSlope"] = rule.maxSlope;
    json["MinHeight"] = rule.minHeight; json["MaxHeight"] = rule.maxHeight;
    json["MaskStratumIndex"] = rule.maskStratumIndex;
    json["MaskWeightMinimum"] = rule.maskWeightMinimum;
    json["ObstacleDistanceMinimum"] = rule.obstacleDistanceMinimum;
    json["ClearanceSpacing"] = rule.clearanceSpacing;
    json["MapEdgePadding"] = rule.mapEdgePadding;
    json["AreaRadiusMinimum"] = rule.areaRadiusMinimum;
    json["AreaRadiusMaximum"] = rule.areaRadiusMaximum;
    json["CheckMaximumRadius"] = rule.bCheckMaximumRadius;
    json["AreaHeightRange"] = rule.areaHeightRange;
    json["UseDensity"] = rule.bUseDensity;  json["Density"] = rule.density;
    json["Count"] = rule.count;
    json["UseAllPositions"] = rule.bUseAllPositions;
    json["RandomSelection"] = rule.bRandomSelection;
    json["Priority"] = static_cast<int>(rule.priority);
    json["FocusGradient"] = static_cast<int>(rule.focusGradient);
    json["FocusGradientRadius"] = rule.focusGradientRadius;
    json["FocusGradientStrength"] = rule.focusGradientStrength;
    json["FocusGradientContrast"] = rule.focusGradientContrast;
    // Correction 7's confirmed cardinality change: v1 global scalars, now per-layer fields
    // (MarkerRule_PARAMS.h) — a genuine addition, not a relocation.
    json["HydroMultiplier"] = rule.hydroMultiplier;
    json["ReclaimDensity"]  = rule.reclaimDensity;
    json["MexDensity"]      = rule.mexDensity;
    json["SpawnPointCount"] = rule.spawnPointCount;
    json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal;
    json["SymmetryMask"] = rule.symmetryMask;
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

} // namespace

nlohmann::ordered_json BuildMarkersStackJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markersStack = nlohmann::ordered_json::array();
    for (const Params::MarkerRule& rule : recipe.markerRules)
        markersStack.push_back(BuildMarkerRuleJson(rule));
    return markersStack;
}

// ARCH §11: wire keys keep the `Global*`/`Marker*`-prefixed spelling SANMAP_FORMAT_SPEC Correction 7
// ratified (`GlobalIconAlloy`, `MarkerColorAlloy`, `MarkerScaleAlloy`, ...) even though the C++ field
// names drop those prefixes — wire spelling and C++ spelling diverge here by design. Colors are
// written as `{r,g,b,a}` objects, the same shape `BuildStratumLayersJson`'s `farColorRemap` already
// uses for a `float[4]` color (MapExporter_Recipe_IO.cpp) — this format's established color shape.
nlohmann::ordered_json BuildGlobalMarkerSettingsJson(const Params::MapRecipe& recipe) {
    const Params::GlobalMarkerSettings& settings = recipe.globalMarkerSettings;
    nlohmann::ordered_json json;
    json["GlobalIconAlloy"]  = settings.iconNameAlloy;
    json["GlobalIconPlasma"] = settings.iconNamePlasma;
    json["GlobalIconSpawn"]  = settings.iconNameSpawn;
    json["MarkerColorAlloy"]  = { { "r", settings.colorAlloy[0] }, { "g", settings.colorAlloy[1] },
                                  { "b", settings.colorAlloy[2] }, { "a", settings.colorAlloy[3] } };
    json["MarkerColorPlasma"] = { { "r", settings.colorPlasma[0] }, { "g", settings.colorPlasma[1] },
                                  { "b", settings.colorPlasma[2] }, { "a", settings.colorPlasma[3] } };
    json["MarkerColorSpawn"]  = { { "r", settings.colorSpawn[0] }, { "g", settings.colorSpawn[1] },
                                  { "b", settings.colorSpawn[2] }, { "a", settings.colorSpawn[3] } };
    json["MarkerScaleAlloy"]  = settings.scaleAlloy;
    json["MarkerScalePlasma"] = settings.scalePlasma;
    json["MarkerScaleSpawn"]  = settings.scaleSpawn;
    return json;
}

} // namespace Io
} // namespace SanmapGen
