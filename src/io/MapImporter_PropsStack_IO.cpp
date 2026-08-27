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

} // namespace

void ReadPropsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    ReadRuleArray(document, "PropsStack", outRecipe.propRules, ReadPropRuleJson);
}

} // namespace Io
} // namespace SanmapGen
