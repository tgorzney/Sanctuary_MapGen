// MapImporter_Recipe_IO.cpp — `mapGeneratorData` -> the recipe's geometry, water and strata.
// Layer: IO. Mirrors MapExporter_Recipe_IO.cpp / MapExporter_Layers_IO.cpp key for key; the
// round-trip acceptance test is what keeps the two honest.
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadGeometryJson(const nlohmann::json& generatorData, const MapImportOptions& options,
                      Params::MapRecipe& outRecipe, MapImportResult& result) {
    Params::Geometry& geometry = outRecipe.geometry;
    int mapSize = geometry.mapSize;
    if (ReadJsonInteger(generatorData, "MapSize", mapSize)) {
        if (mapSize >= options.safetyLimits.minimumMapSize && mapSize <= options.safetyLimits.maximumMapSize)
            geometry.mapSize = mapSize;
        else
            result.Warn("MapSize " + std::to_string(mapSize) + " is outside the safety limits; kept "
                        + std::to_string(geometry.mapSize) + ".");
    }
    // Seed/ScaleFeaturesToMapSize/TerrainMinHeight/WorldUnitsPerCell RELOCATED to the top-level
    // `GeneralMapSettings` object, read by `ReadGeneralMapSettingsJson` UNCONDITIONALLY and BEFORE
    // this function (SANMAP_FORMAT_SPEC Correction 2; MapImporter_IO.cpp wiring order) — no longer
    // read here.
    ReadJsonFloat(generatorData, "TerrainMaxHeight", geometry.terrainMaxHeight);
    // GlobalSymmetryMask RELOCATED to the top-level `Symmetry` object, read by `ReadSymmetryJson`
    // UNCONDITIONALLY and BEFORE this function (SANMAP_FORMAT_SPEC Correction 4, STEP16;
    // MapImporter_IO.cpp wiring order) — no longer read here.

    // The band invariant Geometry::IsValid() depends on is now ALSO enforced unconditionally, from
    // `ParseSimulationDomainsJson` right after `GeneralMapSettings` is read — that's the fix for
    // current-format files, which since `STEP36` never reach this gated block at all
    // (STEP41_PostMigrationImportGaps_IO). It stays called here too, one more time, because THIS
    // block's own `TerrainMaxHeight` re-read above can still introduce a fresh out-of-band value
    // from a genuinely old file's legacy blob, independent of whatever `GeneralMapSettings` already
    // held and already got clamped once — dropping this second call would silently un-clamp a
    // legacy blob's own hostile `TerrainMaxHeight`/`TerrainMinHeight`/`WorldUnitsPerCell`.
    ClampGeometryBand(geometry, result);
}

void ClampGeometryBand(Params::Geometry& geometry, MapImportResult& result) {
    if (geometry.terrainMaxHeight < 1.0f) {
        result.Warn("TerrainMaxHeight was not positive; raised to 1.");
        geometry.terrainMaxHeight = 1.0f;
    }
    if (geometry.terrainMinHeight > geometry.terrainMaxHeight - 1.0f) {
        result.Warn("TerrainMinHeight sat above the ceiling; held one unit below it.");
        geometry.terrainMinHeight = geometry.terrainMaxHeight - 1.0f;
    }
    if (!(geometry.worldUnitsPerCell > 0.0f)) {
        result.Warn("WorldUnitsPerCell was not positive; restored to 1.");
        geometry.worldUnitsPerCell = 1.0f;
    }
}

void ReadWaterJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe) {
    if (!generatorData.contains("Water") || !generatorData["Water"].is_object()) return;
    const nlohmann::json& water = generatorData["Water"];
    ReadJsonBoolean(water, "Enabled", outRecipe.water.bEnabled);
    ReadJsonFloat(water, "WaterLevelMax", outRecipe.water.waterLevelMaximum);
    ReadJsonFloat(water, "DeepWaterDepthMin", outRecipe.water.deepWaterDepthMinimum);
    ReadJsonFloat(water, "DeepWaterDepthMax", outRecipe.water.deepWaterDepthMaximum);
}

// MERGES onto whatever `ReadStratumLayersJson` already populated, index for index, instead of
// replacing the array outright: `stratumLayers` (read earlier, unconditionally — see
// MapImporter_IO.cpp) is the ONLY `.sanmap` source for `Stratum::appearance`, and the legacy
// `Stratums` blob this function reads has no appearance keys at all, so a clear-then-rebuild here
// would silently discard the appearance a designer just imported. Growing only (never shrinking)
// keeps a document with no `stratumLayers` section — hand-authored, or from before this ticket —
// reading exactly as it did before.
void ReadStrataSettingsJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe) {
    if (!generatorData.contains("Stratums") || !generatorData["Stratums"].is_array()) return;
    const nlohmann::json& strataJson = generatorData["Stratums"];
    if (outRecipe.strata.size() < strataJson.size())
        outRecipe.strata.resize(strataJson.size());
    for (std::size_t index = 0; index < strataJson.size(); ++index) {
        const nlohmann::json& stratumJson = strataJson[index];
        if (!stratumJson.is_object()) continue;
        Params::Stratum& stratum = outRecipe.strata[index];
        // The 8 slope-gate reads RELOCATED to `ReadStratumGenerationSettingsJson`
        // (SANMAP_FORMAT_SPEC Correction 12, MapImporter_StratumGeneration_IO.cpp) — no longer read
        // from this legacy blob.
        int maskMode = static_cast<int>(stratum.importedMaskMode);
        if (ReadJsonEnumeration(stratumJson, "ImportedMaskMode", 3, maskMode))
            stratum.importedMaskMode = static_cast<Params::ImportedMaskMode>(maskMode);
        ReadJsonFloatVector4(stratumJson, "MaskRemapMinimum", stratum.maskRemapMinimum);
        ReadJsonFloatVector4(stratumJson, "MaskRemapMaximum", stratum.maskRemapMaximum);
        ReadJsonBoolean(stratumJson, "Enabled", stratum.bEnabled);
        ReadJsonFloat(stratumJson, "TintRed", stratum.tintRed);
        ReadJsonFloat(stratumJson, "TintGreen", stratum.tintGreen);
        ReadJsonFloat(stratumJson, "TintBlue", stratum.tintBlue);
        ReadJsonFloat(stratumJson, "TileCount", stratum.tileCount);
    }
}

} // namespace Io
} // namespace SanmapGen
