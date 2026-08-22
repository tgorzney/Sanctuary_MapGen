// MapImporter_Recipe_IO.h — MODULE-INTERNAL JSON readers behind MapImporter::ParseSanmapJsonText.
// Layer: IO. Split out under the ARCH §1.5 ceilings; declares no new public type (ARCH §8.4).
//
// EVERY reader here is total: it takes the parent object and a key, and it writes the destination
// ONLY when the key exists with the right JSON type. That is Constitution §6 in one pattern — a
// corrupt or partial document leaves the recipe on its own defaults instead of on garbage.
//
// The generic typed accessors (`ReadJsonFloat`/`ReadJsonInteger`/`ReadJsonBoolean`/
// `ReadJsonEnumeration`/`ReadJsonText`) live in `JsonPrimitives_IO.h` now (IO_MIGRATION_SPEC.md §5
// — they were mis-homed here, cross-included by domains that have nothing to do with Recipe).
// `ReadJsonFloatVector4` stays: it is Stratum/Recipe-domain-specific shape, not a generic primitive.
#pragma once
#include "JsonPrimitives_IO.h"
#include <nlohmann/json.hpp>
#include <string>

namespace SanmapGen {
namespace Params { struct MapRecipe; struct LayerStack; struct Geometry; }
namespace Io {

struct MapImportOptions;
struct MapImportResult;

// A 4-component field stored as `{"x":.., "y":.., "z":.., "w":..}` (ARCH §7.2 item 10's Vector4
// shape). Each component is read independently, same as the scalar readers above, so a partial
// object still updates the components it has instead of discarding the whole field.
inline bool ReadJsonFloatVector4(const nlohmann::json& parent, const char* key,
                                 float destination[4]) {
    if (!parent.contains(key) || !parent[key].is_object()) return false;
    const nlohmann::json& vector = parent[key];
    bool bAnyComponentRead = false;
    bAnyComponentRead |= ReadJsonFloat(vector, "x", destination[0]);
    bAnyComponentRead |= ReadJsonFloat(vector, "y", destination[1]);
    bAnyComponentRead |= ReadJsonFloat(vector, "z", destination[2]);
    bAnyComponentRead |= ReadJsonFloat(vector, "w", destination[3]);
    return bAnyComponentRead;
}

// --- The block readers (MapImporter_Recipe_IO.cpp). -----------------------------------------
void ReadGeometryJson(const nlohmann::json& generatorData, const MapImportOptions& options,
                      Params::MapRecipe& outRecipe, MapImportResult& result);
void ReadWaterJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);
void ReadStrataSettingsJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);

// The band invariant Geometry::IsValid() depends on (STEP41_PostMigrationImportGaps_IO), extracted
// out of `ReadGeometryJson` so it also runs unconditionally (MapImporter_IO.h SCOPE NOTE 3). Called
// from `ParseSimulationDomainsJson` right after `ReadGeneralMapSettingsJson` — see that reader's
// own header comment for why the ordering is load-bearing.
void ClampGeometryBand(Params::Geometry& geometry, MapImportResult& result);

// MapImporter_Areas_IO.cpp / MapImporter_Armies_IO.cpp — `areas`/`armies` -> `recipe.areas`/
// `recipe.armies` (MapImporter_IO.h SCOPE NOTE 3). Total per Constitution §6: a missing/non-object
// key leaves outRecipe.areas/armies untouched (empty).
void ReadAreasJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadArmiesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_Markers_IO.cpp / MapImporter_Chains_IO.cpp — `markers`/`chains` -> `recipe.markers`/
// `recipe.chains` (STEP3_MarkersChains_IO; MapImporter_IO.h SCOPE NOTE 3).
void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadChainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_Props_IO.cpp / MapImporter_Decals_IO.cpp — `props`/`decals`/`PropGroups`/
// `DecalGroups` -> the matching `recipe.*` fields (STEP4_PropsDecals_IO; MapImporter_IO.h SCOPE
// NOTE 3). `ReadPropGroupsJson`/`ReadDecalGroupsJson` MUST run before `ReadPropsJson`/
// `ReadDecalsJson` — see each .cpp's own header comment for the `layerIndex` clamp reason.
void ReadPropGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadPropsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);
void ReadDecalGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadDecalsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);

// MapImporter_Atmosphere_IO.cpp — ~49 top-level `.sanmap` keys -> `recipe.atmosphere`
// (ATMOSPHERE_PARAMS_SPEC; MapImporter_IO.h SCOPE NOTE 3). `result` logs the one fail-safe
// fallback in this domain: an unrecognized `skyboxIntensityMode` defaults to `Exposure`, never a
// crash.
void ReadAtmosphereJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                       MapImportResult& result);

// MapImporter_SlopeDefaults_IO.cpp — the top-level `SlopeDefaults` object -> `recipe.slopeDefaults`
// (STEP10_SlopeDefaults_Mechanism; MapImporter_IO.h SCOPE NOTE 3).
void ReadSlopeDefaultsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_GeneralMapSettings_IO.cpp — the top-level `GeneralMapSettings` object ->
// `recipe.geometry`/`recipe.generalMapSettings` (SANMAP_FORMAT_SPEC Correction 2; MapImporter_IO.h
// SCOPE NOTE 3). MUST also run before `ClampGeometryBand` above — see the .cpp's own header
// comment for why that ordering is load-bearing.
void ReadGeneralMapSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_StratumLayers_IO.cpp — the top-level `stratumLayers[9]` array -> `Params::Stratum::
// appearance` (SANMAP_FORMAT_SPEC Correction 13; MapImporter_IO.h SCOPE NOTE 3) — NOT alongside the
// gated `ReadStrataSettingsJson` above. A wrong array length is a loud, logged warning via `result`,
// never a silent truncation or a hard refusal (Constitution §6).
void ReadStratumLayersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                           MapImportResult& result);

// MapImporter_StratumGeneration_IO.cpp — the top-level `StratumGenerationSettings[9]` array ->
// `Params::Stratum::soilPhysics` + the 9 slope-gate fields (SANMAP_FORMAT_SPEC Correction 12;
// MapImporter_IO.h SCOPE NOTE 3). Grow-only, merge-in-place like `ReadStratumLayersJson` above; a
// length mismatch is likewise a logged warning via `result`, never a refusal.
void ReadStratumGenerationSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                                      MapImportResult& result);

// MapImporter_MarkersStack_IO.cpp / MapImporter_PropsStack_IO.cpp / MapImporter_DecalsStack_IO.cpp /
// MapImporter_UnitsStack_IO.cpp — `MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack` ->
// `recipe.markerRules`/`propRules`/`decalRules`/`unitRules`, REPLACING the deleted
// `ReadPlacementRulesJson` (SANMAP_FORMAT_SPEC Correction 7; MapImporter_IO.h SCOPE NOTE 3).
// `ReadGlobalMarkerSettingsJson` — the sibling `GlobalMarkerSettings` object -> `recipe.
// globalMarkerSettings` (ARCH §11), not nested inside `MarkersStack`.
void ReadMarkersStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadGlobalMarkerSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadPropsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadDecalsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadUnitsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_Symmetry_IO.cpp — the top-level `Symmetry` object -> `recipe.globalSymmetryMask`/
// `radialSymmetryRepeatCount`/`symmetryDetection`/`symmetryBlend` (SANMAP_FORMAT_SPEC Correction 4,
// STEP16; MapImporter_IO.h SCOPE NOTE 3), REPLACING the legacy `mapGeneratorData.GlobalSymmetryMask`
// read removed from `ReadGeometryJson` (relocated, not dual-read).
void ReadSymmetryJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_FlowAccumulation_IO.cpp — the top-level `Flow`/`Accumulation` objects ->
// `recipe.flow`/`recipe.accumulation` (SANMAP_FORMAT_SPEC Correction 6, STEP17; MapImporter_IO.h
// SCOPE NOTE 3). `ReadAccumulationJson` reads no fields (none exist yet) but tolerates any content.
void ReadFlowJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadAccumulationJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_DetailNormal_IO.cpp — the top-level `DetailNormal` object -> `recipe.detailNormal`
// (SANMAP_FORMAT_SPEC Correction 8, STEP18; MapImporter_IO.h SCOPE NOTE 3).
void ReadDetailNormalJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_HeightmapStack_IO.cpp — the top-level `HeightmapStack` object -> `recipe.layerStack`
// (SANMAP_FORMAT_SPEC Correction 3; MapImporter_IO.h SCOPE NOTE 3), REPLACING the legacy
// `ReadLayerStackJson`/`mapGeneratorData.SimulationGrouping`/`GeoLayers` (relocated, not duplicated).
void ReadHeightmapStackJson(const nlohmann::json& document, Params::LayerStack& outLayerStack);

// MapImporter_Scenarios_IO.cpp — the top-level `Scenarios` object -> `recipe.scenarios`
// (STEP69_ParamsScenariosRoundTrip_IO.md §1/§3/§5/§6/§7 — this ticket's own inline tables are the
// binding source of truth). Needs `MapImportResult&` — unlike `ReadAreasJson`/`ReadArmiesJson` —
// because the malformed-array case requires `result.Warn(...)`, matching `ReadPropsJson`/
// `ReadStratumLayersJson`'s existing signature shape. Absent/non-object `Scenarios` leaves
// `outRecipe.scenarios` at its default-constructed value (Constitution §6).
void ReadScenariosJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                       MapImportResult& result);

} // namespace Io
} // namespace SanmapGen
