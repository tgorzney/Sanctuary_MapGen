// MapImporter_MarkersStack_IO.cpp — the top-level `MarkersStack` array -> `recipe.markerRuleLayers`
// (one `MarkerRuleLayer` per element, each carrying a nested `Rules` array), plus the top-level
// `GlobalMarkerSettings` object -> `recipe.globalMarkerSettings`.
// Layer: IO. The exact inverse of MapExporter_MarkersStack_IO.cpp. `ReadMarkerRuleJson`'s body is
// relocated verbatim from the deleted MapImporter_Rules_IO.cpp; only its container changed. Same
// tier/calling contract as `areas`/`armies`/`PropGroups`/etc.: both readers take the top-level
// `document` directly and are called unconditionally, BEFORE the `mapGeneratorData` presence gate.
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

constexpr int markerCategoryCount = 4;   // Generic, Spawn, Alloys, Expansion
constexpr int markerPriorityCount = 3;   // LargestArea, SmallestArea, LeastVariance
constexpr int focusGradientCount  = 4;   // None, CenterFocus, EdgeFocus, Torus

void ReadMarkerRuleJson(const nlohmann::json& json, Params::MarkerRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonBoolean(json, "Hidden", rule.bHidden);
    int enumerationValue = static_cast<int>(rule.category);
    if (ReadJsonEnumeration(json, "Category", markerCategoryCount, enumerationValue))
        rule.category = static_cast<Params::MarkerCategory>(enumerationValue);
    ReadJsonFloat(json, "ObstacleDistanceMinimum", rule.obstacleDistanceMinimum);
    ReadJsonFloat(json, "ClearanceSpacing", rule.clearanceSpacing);
    ReadJsonFloat(json, "AreaRadiusMinimum", rule.areaRadiusMinimum);
    ReadJsonFloat(json, "AreaRadiusMaximum", rule.areaRadiusMaximum);
    ReadJsonBoolean(json, "CheckMaximumRadius", rule.bCheckMaximumRadius);
    ReadJsonFloat(json, "AreaHeightRange", rule.areaHeightRange);
    ReadJsonBoolean(json, "UseDensity", rule.bUseDensity);
    ReadJsonFloat(json, "Density", rule.density);
    ReadJsonInteger(json, "Count", rule.count);
    ReadJsonBoolean(json, "UseAllPositions", rule.bUseAllPositions);
    ReadJsonBoolean(json, "RandomSelection", rule.bRandomSelection);
    enumerationValue = static_cast<int>(rule.priority);
    if (ReadJsonEnumeration(json, "Priority", markerPriorityCount, enumerationValue))
        rule.priority = static_cast<Params::MarkerPriority>(enumerationValue);
    enumerationValue = static_cast<int>(rule.focusGradient);
    if (ReadJsonEnumeration(json, "FocusGradient", focusGradientCount, enumerationValue))
        rule.focusGradient = static_cast<Params::FocusGradient>(enumerationValue);
    ReadJsonFloat(json, "FocusGradientRadius", rule.focusGradientRadius);
    ReadJsonFloat(json, "FocusGradientStrength", rule.focusGradientStrength);
    ReadJsonFloat(json, "FocusGradientContrast", rule.focusGradientContrast);
    // HydroMultiplier/ReclaimDensity/MexDensity/SpawnPointCount retired (never had a UI or a PROC
    // consumer — struct-default dead weight): a file still carrying those keys just has them
    // ignored now, the same as any other unrecognized key.
}

// The `MarkerRuleLayer` wrapper (Correction 15): `Name`/`Enabled`/`Hidden` plus the symmetry
// triplet read from sibling keys (relocated here from the old per-rule read), then `Rules` walked
// via the existing `ReadRuleArray` helper into `layer.rules`.
void ReadMarkerRuleLayerJson(const nlohmann::json& json, Params::MarkerRuleLayer& layer) {
    ReadJsonText(json, "Name", layer.name);
    ReadJsonBoolean(json, "Enabled", layer.bEnabled);
    ReadJsonBoolean(json, "Hidden", layer.bHidden);
    ReadJsonBoolean(json, "SymmetryUseGlobal", layer.symmetry.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", layer.symmetry.symmetryMask);
    ReadJsonIntegerClamped(json, "RadialSymmetryRepeatCount", Params::radialSymmetryRepeatCountMinimum,
                          Params::radialSymmetryRepeatCountMaximum,
                          layer.symmetry.radialSymmetryRepeatCount);
    ReadJsonInteger(json, "ParentBundleIdentifier", layer.parentBundleIdentifier);
    ReadJsonText(json, "MarkerTypeName", layer.markerTypeName);
    ReadRuleArray(json, "Rules", layer.rules, ReadMarkerRuleJson);
}

// The inverse of `BuildGlobalMarkerSettingsJson`'s `{r,g,b,a}` shape.
bool ReadJsonColorRgba(const nlohmann::json& parent, const char* key, float destination[4]) {
    if (!parent.contains(key) || !parent[key].is_object()) return false;
    const nlohmann::json& color = parent[key];
    bool bAnyComponentRead = false;
    bAnyComponentRead |= ReadJsonFloat(color, "r", destination[0]);
    bAnyComponentRead |= ReadJsonFloat(color, "g", destination[1]);
    bAnyComponentRead |= ReadJsonFloat(color, "b", destination[2]);
    bAnyComponentRead |= ReadJsonFloat(color, "a", destination[3]);
    return bAnyComponentRead;
}

} // namespace

void ReadMarkersStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    ReadRuleArray(document, "MarkersStack", outRecipe.markerRuleLayers, ReadMarkerRuleLayerJson);
}

void ReadGlobalMarkerSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("GlobalMarkerSettings") || !document["GlobalMarkerSettings"].is_object())
        return;
    const nlohmann::json& json = document["GlobalMarkerSettings"];
    Params::GlobalMarkerSettings& settings = outRecipe.globalMarkerSettings;
    ReadJsonText(json, "GlobalIconAlloy", settings.iconNameAlloy);
    ReadJsonText(json, "GlobalIconPlasma", settings.iconNamePlasma);
    ReadJsonText(json, "GlobalIconSpawn", settings.iconNameSpawn);
    ReadJsonColorRgba(json, "MarkerColorAlloy", settings.colorAlloy);
    ReadJsonColorRgba(json, "MarkerColorPlasma", settings.colorPlasma);
    ReadJsonColorRgba(json, "MarkerColorSpawn", settings.colorSpawn);
    ReadJsonFloat(json, "MarkerScaleAlloy", settings.scaleAlloy);
    ReadJsonFloat(json, "MarkerScalePlasma", settings.scalePlasma);
    ReadJsonFloat(json, "MarkerScaleSpawn", settings.scaleSpawn);
    // ARCH §19.32 — per-Type-section "selected" icon-size pair, alongside the base triplet above.
    ReadJsonFloat(json, "MarkerScaleSelectedAlloy", settings.scaleSelectedAlloy);
    ReadJsonFloat(json, "MarkerScaleSelectedPlasma", settings.scaleSelectedPlasma);
    ReadJsonFloat(json, "MarkerScaleSelectedSpawn", settings.scaleSelectedSpawn);
    ReadJsonColorRgba(json, "MarkerSelectColorAlloy", settings.selectColorAlloy);
    ReadJsonColorRgba(json, "MarkerSelectColorPlasma", settings.selectColorPlasma);
    ReadJsonColorRgba(json, "MarkerSelectColorSpawn", settings.selectColorSpawn);
    ReadJsonColorRgba(json, "MarkerSelectColorDefault", settings.selectColorDefault);
}

} // namespace Io
} // namespace SanmapGen
