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

} // namespace Io
} // namespace SanmapGen
