// MapImporter_ScatterTransform_IO.h — `ReadScatterTransformJson`, plus the small generic machinery
// every rule-Stack importer (Markers/Props/Decals/Units) composes from.
// Layer: IO. Extracted out of the deleted MapImporter_Rules_IO.cpp: all three are shared plumbing
// across 4 sibling `MapImporter_*Stack_IO.cpp` files now, not local to one
// (SANMAP_FORMAT_SPEC Correction 7). Header-only for the two templates (`ReadSharedRuleGates`/
// `ReadRuleArray`) — they are instantiated once per rule type, so they cannot live in a `.cpp`.
#pragma once
#include "JsonPrimitives_IO.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct ScatterTransform; }
namespace Io {

void ReadScatterTransformJson(const nlohmann::json& parent, Params::ScatterTransform& transform);

// The gates every rule family shares (Enabled/slope/height/edge padding/mask gate/transform), so
// the four per-domain readers stay inside the ARCH §1.5 ceiling. `RuleType` must have `bEnabled`,
// `minSlope`, `maxSlope`, `minHeight`, `maxHeight`, `mapEdgePadding`, `maskStratumIndex`,
// `maskWeightMinimum` and `transform` — true of `MarkerRule`/`PropRule`/`DecalRule`/`UnitRule` alike.
template <typename RuleType>
void ReadSharedRuleGates(const nlohmann::json& json, RuleType& rule) {
    ReadJsonBoolean(json, "Enabled", rule.bEnabled);
    ReadJsonFloat(json, "MinSlope", rule.minSlope);
    ReadJsonFloat(json, "MaxSlope", rule.maxSlope);
    ReadJsonFloat(json, "MinHeight", rule.minHeight);
    ReadJsonFloat(json, "MaxHeight", rule.maxHeight);
    ReadJsonInteger(json, "MapEdgePadding", rule.mapEdgePadding);
    ReadJsonInteger(json, "MaskStratumIndex", rule.maskStratumIndex);
    ReadJsonFloat(json, "MaskWeightMinimum", rule.maskWeightMinimum);
    ReadScatterTransformJson(json, rule.transform);
}

// One rule array -> one recipe vector. `ReadOneRule` fills a default-constructed rule, so a
// non-object entry simply yields the default instead of aborting the whole import (Constitution §6
// — reused verbatim by every `*Stack` reader, never reinvented per domain).
template <typename RuleType, typename ReadOneRuleFunction>
void ReadRuleArray(const nlohmann::json& parent, const char* key, std::vector<RuleType>& outRules,
                   ReadOneRuleFunction ReadOneRule) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outRules.clear();
    for (const nlohmann::json& ruleJson : parent[key]) {
        RuleType rule;
        if (ruleJson.is_object()) ReadOneRule(ruleJson, rule);
        outRules.push_back(rule);
    }
}

} // namespace Io
} // namespace SanmapGen
