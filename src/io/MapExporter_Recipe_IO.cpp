// MapExporter_Recipe_IO.cpp — the `.sanmap` document itself: the format's own top-level fields
// plus the `mapGeneratorData` generator-state block. Layer: IO.
// Keys are the format's verbatim strings (ARCH §1.1 naming exception — "identifiers the file
// format dictates ... verbatim so import/export round-trips").
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildStratumLayersJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json stratumLayers = nlohmann::ordered_json::array();
    for (int stratumIndex = 0; stratumIndex < sanmapStratumCount; ++stratumIndex) {
        const bool bHasSettings = stratumIndex < static_cast<int>(recipe.strata.size());
        const Params::Stratum stratum = bHasSettings ? recipe.strata[stratumIndex] : Params::Stratum();
        const Params::StratumAppearance& appearance = stratum.appearance;
        nlohmann::ordered_json layer;
        layer["name"]        = "Stratum " + std::to_string(stratumIndex);
        // `name` still writes the generated placeholder, not `appearance.name` — real gap, flagged
        // for a future pass, not this correction (SANMAP_FORMAT_SPEC Correction 13).
        layer["albedo"]      = { { "path", appearance.albedoTexturePath } };
        layer["normal"]      = { { "path", appearance.normalTexturePath } };
        layer["mask"]        = { { "path", appearance.compositeTexturePath } };
        layer["tileSize"]    = { { "x", stratum.tileCount }, { "y", stratum.tileCount } };
        layer["tileSizeFar"] = { { "x", appearance.farTileCount }, { "y", appearance.farTileCount } };
        layer["tileSizeTriplanar"]    = appearance.triplanarTileCount;
        layer["tileSizeFarTriplanar"] = appearance.farTriplanarTileCount;
        layer["normalScale"]          = appearance.normalScale;
        layer["normalScaleFar"]       = appearance.farNormalScale;
        layer["normalFarNearBlend"]   = appearance.normalFarNearBlend;
        layer["heightFarNearBlend"]   = appearance.heightFarNearBlend;
        // `diffuseRemap` is written FROM Stratum::tint* — not `appearance.diffuseRemapColor`, which
        // was deleted (dead, round-tripped nothing; see StratumAppearance_PARAMS.h).
        layer["diffuseRemap"] = { { "r", stratum.tintRed }, { "g", stratum.tintGreen },
                                  { "b", stratum.tintBlue }, { "a", 1.0f } };
        layer["farColorRemap"] = { { "r", appearance.farColorRemapColor[0] },
                                   { "g", appearance.farColorRemapColor[1] },
                                   { "b", appearance.farColorRemapColor[2] },
                                   { "a", appearance.farColorRemapColor[3] } };
        // Real Vector4, matching the C# ground truth `SanMap.Types.cs` (ARCH §7.2 item 10) — not
        // the bare scalar this line used to write.
        layer["maskRemapMin"] = { {"x", stratum.maskRemapMinimum[0]}, {"y", stratum.maskRemapMinimum[1]},
                                  {"z", stratum.maskRemapMinimum[2]}, {"w", stratum.maskRemapMinimum[3]} };
        layer["maskRemapMax"] = { {"x", stratum.maskRemapMaximum[0]}, {"y", stratum.maskRemapMaximum[1]},
                                  {"z", stratum.maskRemapMaximum[2]}, {"w", stratum.maskRemapMaximum[3]} };
        stratumLayers.push_back(layer);
    }
    return stratumLayers;
}

nlohmann::ordered_json BuildMapGeneratorDataJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json generatorData;
    // Seed/ScaleFeaturesToMapSize/TerrainMinHeight/WorldUnitsPerCell RELOCATED to the top-level
    // `GeneralMapSettings` object (SANMAP_FORMAT_SPEC Correction 2) — no longer written here.
    generatorData["MapSize"]                = recipe.geometry.mapSize;
    generatorData["TerrainMaxHeight"]       = recipe.geometry.terrainMaxHeight;
    // GlobalSymmetryMask RELOCATED to the top-level `Symmetry` object (SANMAP_FORMAT_SPEC
    // Correction 4, STEP16, BuildSymmetryJson below) — no longer written here.
    // SimulationGrouping/GeoLayers RELOCATED to the top-level `HeightmapStack` object
    // (SANMAP_FORMAT_SPEC Correction 3, BuildHeightmapStackJson below) — no longer written here.
    generatorData["Stratums"]               = BuildStrataSettingsJson(recipe);
    nlohmann::ordered_json water;
    water["Enabled"]           = recipe.water.bEnabled;
    water["WaterLevelMax"]     = recipe.water.waterLevelMaximum;
    water["DeepWaterDepthMin"] = recipe.water.deepWaterDepthMinimum;
    water["DeepWaterDepthMax"] = recipe.water.deepWaterDepthMaximum;
    generatorData["Water"] = water;
    // The four placement-rule vectors RELOCATED to the top-level `MarkersStack`/`PropsStack`/
    // `DecalsStack`/`UnitsStack` keys (SANMAP_FORMAT_SPEC Correction 7, ruling #3) — no longer
    // written here.
    return generatorData;
}

std::string MapExporter::BuildSanmapJsonText(const Params::MapRecipe& recipe,
                                             const MapExportOptions& options) {
    const Params::Geometry& geometry = recipe.geometry;
    nlohmann::ordered_json document;
    document["fileVersion"]   = sanmapFileVersion;
    document["mapVersion"]    = sanmapMapVersion;
    // Top-level, sibling of fileVersion/mapVersion/name — REPLACES the old nested
    // mapGeneratorData.MapGeneratorDataVersion write (SANMAP_FORMAT_SPEC Correction 1). Read back by
    // Sanmap_MigrationRunner_IO on import, from the same kCurrentSanGenVersion constant.
    document["SanGenVersion"] = kCurrentSanGenVersion;
    document["name"]        = options.mapName;
    document["credits"]     = options.mapCredits;
    document["width"]       = geometry.mapSize;
    document["length"]      = geometry.mapSize;
    // The format types `height` as a C# int (SanMap.cs:24). Round, don't truncate — a
    // designer-set 127.6 must land on 128, not silently drop to 127 in Newtonsoft's coercion.
    document["height"]      = static_cast<int>(std::lround(geometry.terrainMaxHeight));
    document["heightmapResolution"] = geometry.VertexSize();

    document["hasWater"]   = recipe.water.bEnabled;
    document["waterLevel"] = recipe.water.waterLevelMaximum;
    document["waterDepth"] = recipe.water.deepWaterDepthMaximum;

    document["shader"]            = "RTS/TerrainLit";
    document["heightTransition"]  = 0.5f;
    document["fadeDistance"]      = 128.0f;
    document["fadeStartDistance"] = 1.0f;
    document["stratumLayers"]     = BuildStratumLayersJson(recipe);
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

    // SANMAP_FORMAT_SPEC Correction 7: the four placement-rule vectors as bare top-level arrays,
    // siblings of `mapGeneratorData` (ruling #1), REPLACING the legacy nested
    // `mapGeneratorData.PlacementRules` object written above by `BuildMapGeneratorDataJson`.
    // `GlobalMarkerSettings` is its own top-level key, a sibling of `MarkersStack` (ARCH §11,
    // ruling #2), not nested inside it.
    document["MarkersStack"] = BuildMarkersStackJson(recipe);
    document["PropsStack"]   = BuildPropsStackJson(recipe);
    document["DecalsStack"]  = BuildDecalsStackJson(recipe);
    document["UnitsStack"]   = BuildUnitsStackJson(recipe);
    document["GlobalMarkerSettings"] = BuildGlobalMarkerSettingsJson(recipe);

    // ATMOSPHERE_PARAMS_SPEC: ~49 flat top-level keys (`sunRA`, `skylightIntensity`, `windSpeed`,
    // ...), same tier as the entity domains above — writes directly into `document`, see
    // MapExporter_Atmosphere_IO.cpp's own header comment for the shape difference.
    BuildAtmosphereJson(recipe, document);
    // STEP10_SlopeDefaults_Mechanism: one flat top-level object, same tier as the entity domains
    // above — sibling of `armies`/`atmosphere`, NOT nested in `mapGeneratorData`.
    document["SlopeDefaults"] = BuildSlopeDefaultsJson(recipe);
    // SANMAP_FORMAT_SPEC Correction 2: one flat top-level object, same tier as `SlopeDefaults`
    // above — sibling of `armies`/`atmosphere`/`SlopeDefaults`, NOT nested in `mapGeneratorData`.
    // REPLACES the legacy `mapGeneratorData.Seed`/`ScaleFeaturesToMapSize`/`TerrainMinHeight`/
    // `WorldUnitsPerCell` writes removed from BuildMapGeneratorDataJson above (relocated, not
    // duplicated), plus the one genuinely new field, `GlobalGravity`.
    document["GeneralMapSettings"] = BuildGeneralMapSettingsJson(recipe);
    // SANMAP_FORMAT_SPEC Correction 3: one flat top-level object, same tier as `SlopeDefaults`/
    // `GeneralMapSettings` above — sibling of `mapGeneratorData`, NOT nested in it. REPLACES the
    // legacy `mapGeneratorData.SimulationGrouping`/`GeoLayers` writes removed from
    // BuildMapGeneratorDataJson above (relocated, not duplicated).
    document["HeightmapStack"] = BuildHeightmapStackJson(recipe.layerStack);
    // SANMAP_FORMAT_SPEC Correction 4 (STEP16): one flat top-level object, same tier as
    // `SlopeDefaults`/`GeneralMapSettings`/`HeightmapStack` above — sibling of `mapGeneratorData`,
    // NOT nested in it. REPLACES the legacy `mapGeneratorData.GlobalSymmetryMask` write removed
    // from BuildMapGeneratorDataJson above (relocated, not duplicated).
    document["Symmetry"] = BuildSymmetryJson(recipe);
    // SANMAP_FORMAT_SPEC Correction 6 (STEP17): two flat top-level objects, same tier as
    // `SlopeDefaults`/`GeneralMapSettings`/`HeightmapStack`/`Symmetry` above — siblings of
    // `mapGeneratorData`, NOT nested in it. Pure reservation: no PROC consumer for either.
    document["Flow"] = BuildFlowJson(recipe);
    document["Accumulation"] = BuildAccumulationJson(recipe);
    // SANMAP_FORMAT_SPEC Correction 8 (STEP18): one flat top-level object, same tier as `Flow`/
    // `Accumulation` above — sibling of `mapGeneratorData`, NOT nested in it. Pure reservation:
    // no PROC consumer for the layered-heightmap-delta system this settles a home for.
    document["DetailNormal"] = BuildDetailNormalJson(recipe);

    document["mapGeneratorData"] = BuildMapGeneratorDataJson(recipe);
    const int indent = options.jsonIndentSpaceCount > 0 ? options.jsonIndentSpaceCount : -1;
    return document.dump(indent);
}

} // namespace Io
} // namespace SanmapGen
