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

// MapExporter_MarkersStack_IO.cpp / MapExporter_PropsStack_IO.cpp / MapExporter_DecalsStack_IO.cpp /
// MapExporter_UnitsStack_IO.cpp — `recipe.markerRules`/`propRules`/`decalRules`/`unitRules` -> the
// top-level `MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack` bare JSON ARRAYS
// (SANMAP_FORMAT_SPEC Correction 7, ruling #1), siblings of `mapGeneratorData`, REPLACING the old
// nested `mapGeneratorData.PlacementRules` object (relocated, not duplicated — ruling #3).
// `BuildGlobalMarkerSettingsJson` — `recipe.globalMarkerSettings` -> the top-level
// `GlobalMarkerSettings` object (ARCH §11), a SIBLING of `MarkersStack`, not nested inside it
// (ruling #2).
nlohmann::ordered_json BuildMarkersStackJson(const Params::MapRecipe& recipe);
nlohmann::ordered_json BuildGlobalMarkerSettingsJson(const Params::MapRecipe& recipe);
nlohmann::ordered_json BuildPropsStackJson(const Params::MapRecipe& recipe);
nlohmann::ordered_json BuildDecalsStackJson(const Params::MapRecipe& recipe);
nlohmann::ordered_json BuildUnitsStackJson(const Params::MapRecipe& recipe);

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

// MapExporter_Atmosphere_IO.cpp — `recipe.atmosphere` -> ~49 FLAT top-level `.sanmap` document
// keys (ATMOSPHERE_PARAMS_SPEC). SHAPE DIFFERENCE from every `Build*Json` above: this one takes
// `document` BY REFERENCE and writes directly into it instead of returning one self-contained
// object — there is no single `atmosphere` sub-object on the wire to return (see the .cpp's own
// header comment for the full field-name-mismatch table).
void BuildAtmosphereJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document);

// MapExporter_SlopeDefaults_IO.cpp — `recipe.slopeDefaults` -> the top-level `SlopeDefaults`
// object (STEP10_SlopeDefaults_Mechanism, MASKING_SPEC.md §1.7's shared-default layer). One flat
// object, sibling of `armies`/`atmosphere`/etc., NOT nested in `mapGeneratorData`.
nlohmann::ordered_json BuildSlopeDefaultsJson(const Params::MapRecipe& recipe);

// MapExporter_StratumGeneration_IO.cpp — `recipe.strata` -> the top-level `StratumGenerationSettings`
// array (SANMAP_FORMAT_SPEC Correction 12): per-stratum soil physics (6 fields, new writes) plus the
// 9 slope-gate fields (`SlopeUseGlobal` + the 8 relocated from the legacy `mapGeneratorData.Stratums`
// blob). Sibling of `stratumLayers`, index-aligned with it, same fixed-`sanmapStratumCount`
// cardinality pattern as `BuildStratumLayersJson`.
nlohmann::ordered_json BuildStratumGenerationSettingsJson(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
