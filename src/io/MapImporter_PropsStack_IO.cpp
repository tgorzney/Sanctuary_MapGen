// MapImporter_PropsStack_IO.cpp — the top-level `PropsStack` array -> `recipe.propRules`.
// Layer: IO. The exact inverse of MapExporter_PropsStack_IO.cpp. `ReadPropRuleJson`'s body is
// relocated verbatim from the deleted MapImporter_Rules_IO.cpp; only its container changed. Same
// tier/calling contract as `areas`/`armies`/`PropGroups`/etc.: takes the top-level `document`
// directly and is called unconditionally, BEFORE the `mapGeneratorData` presence gate.
#include "FootprintBakeFingerprint_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void ReadPropRuleJson(const nlohmann::json& json, Params::PropRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonFloat(json, "Density", rule.density);
    ReadJsonBoolean(json, "AvoidWater", rule.bAvoidWater);
    ReadJsonBoolean(json, "NearCliffs", rule.bNearCliffs);
    ReadJsonBoolean(json, "Reclaimable", rule.bReclaimable);
    ReadJsonFloat(json, "SpacingMinimum", rule.spacingMinimum);
    ReadJsonFloat(json, "ObstacleDistanceMinimum", rule.obstacleDistanceMinimum);
    ReadJsonFloat(json, "NearCliffDistanceMaximum", rule.nearCliffDistanceMaximum);
    ReadJsonFloat(json, "BaseFootprintWidth", rule.baseFootprintWidth);
    ReadJsonFloat(json, "BaseFootprintDepth", rule.baseFootprintDepth);
    ReadFootprintBakeFingerprintJson(json, "FootprintBakeFingerprint", rule.footprintBakeFingerprint);
    ReadJsonBoolean(json, "SymmetryUseGlobal", rule.symmetry.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", rule.symmetry.symmetryMask);
    ReadJsonIntegerClamped(json, "RadialSymmetryRepeatCount", Params::radialSymmetryRepeatCountMinimum,
                          Params::radialSymmetryRepeatCountMaximum, rule.symmetry.radialSymmetryRepeatCount);
}

// The inverse of `BuildGlobalPropSettingsJson`'s `{r,g,b,a}` shape, mirroring
// `MapImporter_MarkersStack_IO.cpp`'s own `ReadJsonColorRgba` (each domain keeps its own copy).
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

void ReadPropsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    ReadRuleArray(document, "PropsStack", outRecipe.propRules, ReadPropRuleJson);
}

// `GlobalPropSettings` — its own top-level key, a sibling of `PropsStack` (ARCH §20).
void ReadGlobalPropSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("GlobalPropSettings") || !document["GlobalPropSettings"].is_object())
        return;
    const nlohmann::json& json = document["GlobalPropSettings"];
    Params::GlobalPropSettings& settings = outRecipe.globalPropSettings;
    ReadJsonColorRgba(json, "ColorProp", settings.colorProp);
    ReadJsonColorRgba(json, "ColorReclaim", settings.colorReclaim);
}

} // namespace Io
} // namespace SanmapGen
