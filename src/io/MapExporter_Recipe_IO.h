// MapExporter_Recipe_IO.h — MODULE-INTERNAL JSON builders behind MapExporter::BuildSanmapJsonText.
// Layer: IO. Split out under the ARCH §1.5 ceilings; declares no new public type (ARCH §8.4) —
// only the aspect functions the exporter's own translation units share.
//
// The JSON library is the project's existing one (nlohmann, already fetched by CMake and used by
// the legacy exporter) — no second JSON dependency is introduced (work-order M1-6).
// `ordered_json` keeps the written key order stable so two exports of the same recipe diff clean.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Params { struct MapRecipe; struct LayerStack; }
namespace Io {

struct MapExportOptions;

// The generator-state block: the on-disk form of `Params::MapRecipe` (SANMAP_FORMAT_SPEC
// "mapGeneratorData — SanGen's generator-state round-trip").
nlohmann::ordered_json BuildMapGeneratorDataJson(const Params::MapRecipe& recipe);

// The format's fixed 9 texture layers, filled from the recipe's `strata` (shorter is legal —
// strata past the end are written on their defaults).
nlohmann::ordered_json BuildStratumLayersJson(const Params::MapRecipe& recipe);

// MapExporter_Layers_IO.cpp
nlohmann::ordered_json BuildLayerStackJson(const Params::LayerStack& layerStack);
nlohmann::ordered_json BuildStrataSettingsJson(const Params::MapRecipe& recipe);

// MapExporter_Rules_IO.cpp — the four placement rule vectors, as one object.
nlohmann::ordered_json BuildPlacementRulesJson(const Params::MapRecipe& recipe);

// MapExporter_Areas_IO.cpp — `recipe.areas` -> the top-level `areas` dictionary (JSON object keyed
// by MapArea::name). No coordinate flip (ENTITY_AUTHORING_PARAMS_SPEC finding 3).
nlohmann::ordered_json BuildAreasJson(const Params::MapRecipe& recipe);

// MapExporter_Armies_IO.cpp — `recipe.armies` -> the top-level `armies` dictionary (JSON object
// keyed by Army::name, recursing through UnitGroup.groups/units).
nlohmann::ordered_json BuildArmiesJson(const Params::MapRecipe& recipe);

// MapExporter_Markers_IO.cpp — `recipe.markers` -> the top-level `markers` dictionary (two-level
// JSON object keyed by MarkerInstanceGroup::name / MarkerTransform::name). Applies the coordinate
// flip to MarkerTransform.transform.positionZ (STEP3_MarkersChains_IO).
nlohmann::ordered_json BuildMarkersJson(const Params::MapRecipe& recipe);

// MapExporter_Chains_IO.cpp — `recipe.chains` -> the top-level `chains` dictionary (JSON object
// keyed by MarkerChain::name, each value a bare array of {type,name} objects, not a wrapping
// object — STEP3_MarkersChains_IO finding 3). No coordinate flip.
nlohmann::ordered_json BuildChainsJson(const Params::MapRecipe& recipe);

// MapExporter_Props_IO.cpp / MapExporter_Decals_IO.cpp — `recipe.props`/`recipe.decals` -> the
// top-level `props`/`decals` plain JSON ARRAYS (not dictionaries — STEP4_PropsDecals_IO finding 1),
// and `recipe.propLayers`/`recipe.decalLayers` -> the top-level `PropGroups`/`DecalGroups` PascalCase
// arrays (SANMAP_FORMAT_SPEC Correction 14). Applies the coordinate flip to
// PropTransform/DecalTransform.transform.positionZ. Called from BuildSanmapJsonText
// (STEP5_PropsDecalsValidation_UI live-wired these); `blueprintPath` resolution against a sanpack
// is a separate, sibling pre-flight step (`Io::ValidatePropAndDecalBlueprintPaths`), not this pass.
nlohmann::ordered_json BuildPropsJson(const Params::MapRecipe& recipe);
nlohmann::ordered_json BuildPropGroupsJson(const Params::MapRecipe& recipe);
nlohmann::ordered_json BuildDecalsJson(const Params::MapRecipe& recipe);
nlohmann::ordered_json BuildDecalGroupsJson(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
