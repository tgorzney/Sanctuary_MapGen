// MapImporter_DecalsStack_IO.cpp — the top-level `DecalsStack` array -> `recipe.decalRules`.
// Layer: IO. The exact inverse of MapExporter_DecalsStack_IO.cpp. `ReadDecalRuleJson`'s body is
// relocated verbatim from the deleted MapImporter_Rules_IO.cpp; only its container changed. Same
// tier/calling contract as `areas`/`armies`/`PropGroups`/etc.: takes the top-level `document`
// directly and is called unconditionally, BEFORE the `mapGeneratorData` presence gate.
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void ReadDecalRuleJson(const nlohmann::json& json, Params::DecalRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonFloat(json, "Density", rule.density);
    ReadJsonFloat(json, "SpacingMinimum", rule.spacingMinimum);
    ReadJsonBoolean(json, "SymmetryUseGlobal", rule.symmetry.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", rule.symmetry.symmetryMask);
    ReadJsonIntegerClamped(json, "RadialSymmetryRepeatCount", Params::radialSymmetryRepeatCountMinimum,
                          Params::radialSymmetryRepeatCountMaximum, rule.symmetry.radialSymmetryRepeatCount);
}

// The inverse of `BuildGlobalDecalSettingsJson`'s `{r,g,b,a}` shape, mirroring
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

void ReadDecalsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    ReadRuleArray(document, "DecalsStack", outRecipe.decalRules, ReadDecalRuleJson);
}

// `GlobalDecalSettings` — its own top-level key, a sibling of `DecalsStack` (ARCH §20).
void ReadGlobalDecalSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("GlobalDecalSettings") || !document["GlobalDecalSettings"].is_object())
        return;
    const nlohmann::json& json = document["GlobalDecalSettings"];
    Params::GlobalDecalSettings& settings = outRecipe.globalDecalSettings;
    ReadJsonColorRgba(json, "ColorDecal", settings.colorDecal);
}

} // namespace Io
} // namespace SanmapGen
