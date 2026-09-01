// MapExporter_MarkersStack_IO.cpp — `recipe.markerRuleLayers` -> the top-level `MarkersStack` array
// (one element per `MarkerRuleLayer`, each carrying a nested `Rules` array), plus
// `recipe.globalMarkerSettings` -> the top-level `GlobalMarkerSettings` object.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 15 (two-level shape, `Rules` key) + ARCH_16_01_
// NewParamsShapes.md §16.1 (the symmetry triplet promoted onto the layer) + ARCH §11 (ruling #2:
// `GlobalMarkerSettings` is its own top-level key, a SIBLING of `MarkersStack`, not nested inside
// it — a JSON array cannot structurally host a nested key). `BuildMarkerRuleJson`'s body is
// relocated verbatim from the deleted MapExporter_Rules_IO.cpp; only its container changed.
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
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

// The `MarkerRuleLayer` wrapper (Correction 15): `Name`/`Enabled`/`Hidden` plus the symmetry
// triplet flattened as sibling keys, then `Rules` = each contained `MarkerRule` via
// `BuildMarkerRuleJson` above (which no longer writes the 3 symmetry keys itself).
nlohmann::ordered_json BuildMarkerRuleLayerJson(const Params::MarkerRuleLayer& layer) {
    nlohmann::ordered_json json;
    json["Name"] = layer.name;
    json["Enabled"] = layer.bEnabled;
    json["Hidden"] = layer.bHidden;
    json["SymmetryUseGlobal"] = layer.symmetry.bSymmetryUseGlobal;
    json["SymmetryMask"] = layer.symmetry.symmetryMask;
    json["RadialSymmetryRepeatCount"] = layer.symmetry.radialSymmetryRepeatCount;
    json["ParentBundleIdentifier"] = layer.parentBundleIdentifier;
    json["MarkerTypeName"] = layer.markerTypeName;
    nlohmann::ordered_json rules = nlohmann::ordered_json::array();
    for (const Params::MarkerRule& rule : layer.rules)
        rules.push_back(BuildMarkerRuleJson(rule));
    json["Rules"] = std::move(rules);
    return json;
}

} // namespace

nlohmann::ordered_json BuildMarkersStackJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markersStack = nlohmann::ordered_json::array();
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers)
        markersStack.push_back(BuildMarkerRuleLayerJson(layer));
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
    // ARCH §19.32 — per-Type-section "selected" icon-size pair, alongside the base triplet above.
    json["MarkerScaleSelectedAlloy"]  = settings.scaleSelectedAlloy;
    json["MarkerScaleSelectedPlasma"] = settings.scaleSelectedPlasma;
    json["MarkerScaleSelectedSpawn"]  = settings.scaleSelectedSpawn;
    json["MarkerSelectColorAlloy"]   = { { "r", settings.selectColorAlloy[0] },
        { "g", settings.selectColorAlloy[1] }, { "b", settings.selectColorAlloy[2] },
        { "a", settings.selectColorAlloy[3] } };
    json["MarkerSelectColorPlasma"]  = { { "r", settings.selectColorPlasma[0] },
        { "g", settings.selectColorPlasma[1] }, { "b", settings.selectColorPlasma[2] },
        { "a", settings.selectColorPlasma[3] } };
    json["MarkerSelectColorSpawn"]   = { { "r", settings.selectColorSpawn[0] },
        { "g", settings.selectColorSpawn[1] }, { "b", settings.selectColorSpawn[2] },
        { "a", settings.selectColorSpawn[3] } };
    json["MarkerSelectColorDefault"] = { { "r", settings.selectColorDefault[0] },
        { "g", settings.selectColorDefault[1] }, { "b", settings.selectColorDefault[2] },
        { "a", settings.selectColorDefault[3] } };
    return json;
}

} // namespace Io
} // namespace SanmapGen
