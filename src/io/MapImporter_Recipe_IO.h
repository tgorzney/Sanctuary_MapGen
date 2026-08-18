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
namespace Params { struct MapRecipe; struct LayerStack; }
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

// --- The block readers (MapImporter_Recipe_IO.cpp / MapImporter_Layers_IO.cpp). -----------------
void ReadGeometryJson(const nlohmann::json& generatorData, const MapImportOptions& options,
                      Params::MapRecipe& outRecipe, MapImportResult& result);
void ReadWaterJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);
void ReadLayerStackJson(const nlohmann::json& generatorData, Params::LayerStack& outLayerStack);
void ReadStrataSettingsJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);
void ReadPlacementRulesJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);

// MapImporter_Areas_IO.cpp / MapImporter_Armies_IO.cpp — `areas`/`armies` are top-level `.sanmap`
// keys, SIBLINGS of `mapGeneratorData`, not nested inside it. Both take the top-level `document`
// (never `generatorData`) and must be called unconditionally, BEFORE the `mapGeneratorData`
// presence gate in MapImporter_IO.cpp — see that file's "Critical wiring correction" note. Total
// per Constitution §6: a missing/non-object key leaves outRecipe.areas/armies untouched (empty).
void ReadAreasJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadArmiesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_Markers_IO.cpp / MapImporter_Chains_IO.cpp — `markers`/`chains` are top-level
// `.sanmap` keys, SIBLINGS of `mapGeneratorData`, same posture and calling contract as
// `areas`/`armies` above (STEP3_MarkersChains_IO, following Step 2's wiring-order correction
// directly).
void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadChainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

// MapImporter_Props_IO.cpp / MapImporter_Decals_IO.cpp — `props`/`decals` and `PropGroups`/
// `DecalGroups` are top-level `.sanmap` keys (STEP4_PropsDecals_IO). `ReadPropGroupsJson`/
// `ReadDecalGroupsJson` MUST be called before `ReadPropsJson`/`ReadDecalsJson`: the `layerIndex`
// range-clamp (ARCH §12) validates against `outRecipe.propLayers.size()`/`outRecipe.decalLayers.
// size()`, which the *Groups readers populate. Called from ParseSanmapJsonText, Groups before
// instances, in that order (STEP5_PropsDecalsValidation_UI live-wired these — see
// MapImporter_IO.h SCOPE NOTE 2).
void ReadPropGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadPropsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);
void ReadDecalGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadDecalsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);

// MapImporter_Atmosphere_IO.cpp — ~49 top-level `.sanmap` keys -> `recipe.atmosphere`
// (ATMOSPHERE_PARAMS_SPEC). Same tier and calling contract as `areas`/`armies` above: takes the
// top-level `document` directly, called unconditionally BEFORE the `mapGeneratorData` presence
// gate. `result` is needed here (unlike Areas/Armies/Markers/Chains) solely to log the one
// fail-safe fallback in this domain: an unrecognized `skyboxIntensityMode` string defaults to
// `Exposure` with a warning, never a crash.
void ReadAtmosphereJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                       MapImportResult& result);

// MapImporter_SlopeDefaults_IO.cpp — the top-level `SlopeDefaults` object -> `recipe.slopeDefaults`
// (STEP10_SlopeDefaults_Mechanism). Same tier and calling contract as `areas`/`armies`/`atmosphere`
// above: takes the top-level `document` directly, called unconditionally BEFORE the
// `mapGeneratorData` presence gate.
void ReadSlopeDefaultsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

} // namespace Io
} // namespace SanmapGen
