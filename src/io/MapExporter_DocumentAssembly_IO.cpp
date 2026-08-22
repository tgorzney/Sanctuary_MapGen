// MapExporter_DocumentAssembly_IO.cpp — the 4 orchestration-tier helpers moved out of
// MapExporter_Recipe_IO.cpp verbatim (STEP31_ExporterRecipeOrchestrator_IO), so
// BuildSanmapJsonText's own file holds zero real logic beyond its own calling sequence.
#include "MapExporter_DocumentAssembly_IO.h"
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>

namespace SanmapGen {
namespace Io {

// The ~17 flat root scalars (`fileVersion` through `fadeStartDistance`) — the document's own
// envelope, not any format-native sub-object.
void BuildDocumentEnvelopeJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document) {
    const Params::Geometry& geometry = recipe.geometry;
    document["fileVersion"]   = sanmapFileVersion;
    document["mapVersion"]    = sanmapMapVersion;
    // Top-level, sibling of fileVersion/mapVersion/name — REPLACES the old nested
    // mapGeneratorData.MapGeneratorDataVersion write (SANMAP_FORMAT_SPEC Correction 1). Read back by
    // Sanmap_MigrationRunner_IO on import, from the same kCurrentSanGenVersion constant.
    document["SanGenVersion"] = kCurrentSanGenVersion;
    document["name"]        = recipe.mapName;
    document["credits"]     = recipe.mapCredits;
    document["width"]       = geometry.mapSize;
    document["length"]      = geometry.mapSize;
    // The format types `height` as a C# int (SanMap.cs:24). Round, don't truncate — a
    // designer-set 127.6 must land on 128, not silently drop to 127 in Newtonsoft's coercion.
    document["height"]      = static_cast<int>(std::lround(geometry.terrainMaxHeight));
    document["heightmapResolution"] = geometry.VertexSize();

    document["hasWater"]   = recipe.water.bEnabled;
    // The format types `waterLevel` as a C# int too (STEP84 §6.2, measured: reference shows
    // "waterLevel": 78, never "78.0") — same `.0`-vs-bare-integer distinction as `height` above, same
    // fix (round, don't truncate). `waterDepth` is NOT one of these: the reference shows a genuine
    // fractional double there (`"waterDepth": 30.615638732910156`) — do not "tidy" it.
    document["waterLevel"] = static_cast<int>(std::lround(recipe.water.waterLevelMaximum));
    document["waterDepth"] = recipe.water.deepWaterDepthMaximum;
    // STEP30_LegacyBlobFieldHoming_IO: a 4th sibling of the STEP27 trio above, camelCase to match
    // (the official format's own Water region uses camelCase, unlike SanGen-owned PascalCase
    // sections). The legacy `mapGeneratorData.Water.DeepWaterDepthMin`
    // (BuildMapGeneratorDataJson) still writes too and stays authoritative on import when present.
    document["deepWaterDepthMin"] = recipe.water.deepWaterDepthMinimum;

    document["shader"]            = "RTS/TerrainLit";
    document["heightTransition"]  = 0.5f;
    document["fadeDistance"]      = 128.0f;
    document["fadeStartDistance"] = 1.0f;
}

// `stratumLayers`, `StratumGenerationSettings`, `areas`, `armies`, `markers`, `chains`, `decals`,
// `props`, `PropGroups`, `DecalGroups` — the recipe's own authored entities.
void AppendEntityDomainsJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document) {
    document["stratumLayers"] = BuildStratumLayersJson(recipe);
    // SANMAP_FORMAT_SPEC Correction 12: soil physics + slope-gate overrides, index-aligned with
    // `stratumLayers`, same tier, sibling of it (not nested in `mapGeneratorData`).
    document["StratumGenerationSettings"] = BuildStratumGenerationSettingsJson(recipe);

    // SCOPE NOTE 1 (MapExporter_IO.h): every entity domain now round-trips real content.
    // `PropGroups`/`DecalGroups` are SanGen-owned manual-layer metadata (SANMAP_FORMAT_SPEC
    // Correction 14), siblings of `props`/`decals`, not nested inside them.
    document["areas"]       = BuildAreasJson(recipe);
    document["armies"]      = BuildArmiesJson(recipe);
    document["markers"]     = BuildMarkersJson(recipe);
    document["chains"]      = BuildChainsJson(recipe);
    document["decals"]      = BuildDecalsJson(recipe);
    document["props"]       = BuildPropsJson(recipe);
    document["PropGroups"]  = BuildPropGroupsJson(recipe);
    document["DecalGroups"] = BuildDecalGroupsJson(recipe);
}

// `MarkersStack`, `PropsStack`, `DecalsStack`, `UnitsStack`, `GlobalMarkerSettings` — the
// placement-rule vectors and their shared settings.
void AppendStackDomainsJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document) {
    // SANMAP_FORMAT_SPEC Correction 7: the four placement-rule vectors as bare top-level arrays,
    // siblings of `mapGeneratorData` (ruling #1), REPLACING the legacy nested
    // `mapGeneratorData.PlacementRules` object written by `BuildMapGeneratorDataJson`.
    // `GlobalMarkerSettings` is its own top-level key, a sibling of `MarkersStack` (ARCH §11,
    // ruling #2), not nested inside it.
    document["MarkersStack"] = BuildMarkersStackJson(recipe);
    document["PropsStack"]   = BuildPropsStackJson(recipe);
    document["DecalsStack"]  = BuildDecalsStackJson(recipe);
    document["UnitsStack"]   = BuildUnitsStackJson(recipe);
    document["GlobalMarkerSettings"] = BuildGlobalMarkerSettingsJson(recipe);
}

// `BuildAtmosphereJson`, `SlopeDefaults`, `GeneralMapSettings`, `HeightmapStack`, `Symmetry`,
// `Flow`, `Accumulation`, `DetailNormal` — all flat top-level objects, siblings of
// `mapGeneratorData` (NOT nested in it), each REPLACING a legacy `mapGeneratorData.*` field its
// own builder's header comment documents in full; short Correction-number references kept per
// call below for traceability, without repeating that full explanation eight times.
void AppendSimulationDomainsJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document) {
    BuildAtmosphereJson(recipe, document);                                  // ATMOSPHERE_PARAMS_SPEC
    document["SlopeDefaults"] = BuildSlopeDefaultsJson(recipe);             // STEP10_SlopeDefaults_Mechanism
    document["GeneralMapSettings"] = BuildGeneralMapSettingsJson(recipe);   // Correction 2
    document["HeightmapStack"] = BuildHeightmapStackJson(recipe.layerStack); // Correction 3
    document["Symmetry"] = BuildSymmetryJson(recipe);                       // Correction 4 (STEP16)
    document["Flow"] = BuildFlowJson(recipe);                               // Correction 6 (STEP17)
    document["Accumulation"] = BuildAccumulationJson(recipe);               // Correction 6 (STEP17)
    document["DetailNormal"] = BuildDetailNormalJson(recipe);               // Correction 8 (STEP18)
}

} // namespace Io
} // namespace SanmapGen
