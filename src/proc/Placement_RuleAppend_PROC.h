// Placement_RuleAppend_PROC.h — the marker family's rule-flattening entry point, split out of
// Placement_Rules_PROC.cpp so the two-level markerRuleLayers walk fits the §1.5 ceilings
// (ARCH_01_05_FileSizeCeilings.md §1.5). See Placement_MarkerRules_PROC.cpp for the definition.
#pragma once
#include "Placement_PROC.h"

namespace SanmapGen { namespace Proc {

void AppendMarkerRules(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                       std::vector<ScatterRuleConfiguration>& configurations,
                       std::vector<Data::TemplateIdentifier>& identifiers,
                       std::vector<int>& radialSymmetryRepeatCounts);

} }
