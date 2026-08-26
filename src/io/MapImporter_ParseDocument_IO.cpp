// MapImporter_ParseDocument_IO.cpp — MapImporter::ParseSanmapJsonText, relocated out of
// MapImporter_IO.cpp (STEP35_ImporterParseDocumentSplit_IO), plus the 4 file-private
// orchestration-tier helpers it now sequences, mirroring MapExporter_DocumentAssembly_IO.cpp's
// shipped shape (STEP31) 1:1 on the import side. None of these 4 helpers own a single top-level
// `.sanmap` section — they only sequence calls into the real per-domain readers that already live
// elsewhere (MapImporter_Recipe_IO.h and its siblings), so they are orchestration helpers, not
// subject to "one domain per file."
//
// REORDER NOTE (⚠️ STEP35 ruling): grouping into these 4 helpers moves `stratumLayers`/
// `StratumGenerationSettings` up next to the other entity domains, and moves `GeneralMapSettings`/
// `HeightmapStack`/`Symmetry` to run before `Flow`/`Accumulation`/`DetailNormal` — both relative to
// the pre-STEP35 physical order. Safe: every reader here is independently gated on its own
// top-level key (Constitution §6) and reads no sibling domain's key; every pre-STEP35 ordering
// comment said only "before the mapGeneratorData gate," never "before/after sibling domain X" —
// confirmed by re-reading every reader's own file, and by the full round-trip suite staying green.
#include "MapImporter_ArmyIdentityNormalize_IO.h"
#include "MapImporter_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "Sanmap_MigrationRunner_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// `name`/`credits`, `hasWater`/`waterLevel`/`waterDepth`, `deepWaterDepthMin`, `height`, `width` —
// the document's own envelope, not any format-native sub-object. The gated legacy-blob readers in
// ParseSanmapJsonText's own tail still run AFTER this and win on overlap (STEP27/STEP30 precedent).
void ParseDocumentEnvelopeJson(const nlohmann::json& document, const MapImportOptions& options,
                               Params::MapRecipe& outRecipe) {
    ReadJsonText(document, "name", outRecipe.mapName);
    // A missing/empty name falls back to "mapdef" (mirrors the Files tab's own TextInputRules
    // invariant, STEP25_MapNameCredits_IO); `credits` gets no such fallback — an empty credits
    // string is legitimate real content (every official demo map's credits field is empty).
    if (outRecipe.mapName.empty()) outRecipe.mapName = "mapdef";
    ReadJsonText(document, "credits", outRecipe.mapCredits);

    // Top-level mirrors of the legacy mapGeneratorData.Water block (STEP27/STEP30) — read here so
    // an official/hand-authored map with no mapGeneratorData block still keeps its water settings.
    ReadJsonBoolean(document, "hasWater", outRecipe.water.bEnabled);
    ReadJsonFloat(document, "waterLevel", outRecipe.water.waterLevelMaximum);
    ReadJsonFloat(document, "waterDepth", outRecipe.water.deepWaterDepthMaximum);
    ReadJsonFloat(document, "deepWaterDepthMin", outRecipe.water.deepWaterDepthMinimum);

    // `height` is the terrain's own vertical extent; mapGeneratorData overrides it when present.
    ReadJsonFloat(document, "height", outRecipe.geometry.terrainMaxHeight);
    int mapWidth = outRecipe.geometry.mapSize;
    if (ReadJsonInteger(document, "width", mapWidth)
        && mapWidth >= options.safetyLimits.minimumMapSize
        && mapWidth <= options.safetyLimits.maximumMapSize)
        outRecipe.geometry.mapSize = mapWidth;
}

// `areas`/`armies`/`markers`/`MarkerGroups`/`chains`/`PropGroups`/`props`/`DecalGroups`/`decals`/
// `stratumLayers`/`StratumGenerationSettings`/`Scenarios` — top-level `.sanmap` keys, siblings of
// `mapGeneratorData`, read unconditionally so hand-authored content survives even with no
// generator block (STEP2/STEP3/STEP5). *Groups readers run before the instance readers (the
// layerIndex clamp, ARCH §12, validates against the layer arrays *Groups populates) — new for
// markers this ticket (STEP60_MarkerInstanceLayer_PARAMS), same ordering Props/Decals already
// use; `StratumGenerationSettings` runs after `stratumLayers` so its cardinality check sees that
// array's already-grown length. NOT alongside `ReadStrataSettingsJson` (ParseSanmapJsonText's
// gated tail): that one merges the separate legacy `mapGeneratorData.Stratums` blob onto whatever
// this function already wrote.
//
// ⚠️ STEP76_ArmyIdentityNaming_IO §4b: `NormalizeArmyIdentities` runs LAST, at the END of this
// function, not beside `ReadArmiesJson`. It rewrites BOTH `outRecipe.armies` AND
// `outRecipe.markers` (the `"Spawn"` group's key references), and `ReadArmiesJson` runs before
// `ReadMarkersJson` below — calling the normalizer any earlier would rewrite armies while
// `outRecipe.markers` was still empty, silently orphaning every spawn marker.
void ParseEntityDomainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                            MapImportResult& result) {
    ReadAreasJson(document, outRecipe);
    ReadArmiesJson(document, outRecipe);
    ReadMarkerGroupsJson(document, outRecipe);
    ReadMarkerLayerBundlesJson(document, outRecipe, result);
    ReadMarkersJson(document, outRecipe, result);
    ReconcileMarkerLayers(outRecipe, result);              // STEP115
    ReadChainsJson(document, outRecipe);
    ReadPropGroupsJson(document, outRecipe);
    ReadPropsJson(document, outRecipe, result);
    ReconcilePropLayers(outRecipe, result);                // STEP115
    ReadDecalGroupsJson(document, outRecipe);
    ReadDecalsJson(document, outRecipe, result);
    ReconcileDecalLayers(outRecipe, result);                // STEP115
    ReadStratumLayersJson(document, outRecipe, result);
    ReadStratumGenerationSettingsJson(document, outRecipe, result);
    // STEP69_ParamsScenariosRoundTrip_IO: lobby-resolved spawn/alloy scenario data, same tier,
    // sibling of the above (NOT nested in `mapGeneratorData`).
    ReadScenariosJson(document, outRecipe, result);
    // STEP76_ArmyIdentityNaming_IO §4b — must stay last, see the warning above.
    NormalizeArmyIdentities(outRecipe, result);
}

// `MarkersStack`/`GlobalMarkerSettings`/`PropsStack`/`DecalsStack`/`UnitsStack` — the placement-rule
// vectors and their shared settings (Correction 7), REPLACING the deleted
// `ReadPlacementRulesJson(generatorData, ...)` gated call (relocated, not dual-written).
void ParseStackDomainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    ReadMarkersStackJson(document, outRecipe);
    ReadGlobalMarkerSettingsJson(document, outRecipe);
    ReadPropsStackJson(document, outRecipe);
    ReadDecalsStackJson(document, outRecipe);
    ReadUnitsStackJson(document, outRecipe);
}

// `Atmosphere`/`SlopeDefaults`/`GeneralMapSettings`/`HeightmapStack`/`Symmetry`/`Flow`/
// `Accumulation`/`DetailNormal` — flat top-level objects, siblings of `mapGeneratorData`, each
// REPLACING a legacy `mapGeneratorData.*` field its own reader's header comment documents in full
// (Correction numbers kept per call for traceability). `GeneralMapSettings` is LOAD-BEARING: it
// must run before `ClampGeometryBand` right below it — that guard's clamp/Warn block depends on
// `geometry.terrainMinHeight`/`worldUnitsPerCell` already being set (see
// MapImporter_GeneralMapSettings_IO.cpp's own header comment). `ClampGeometryBand` runs
// unconditionally HERE, not inside the gated legacy `ReadGeometryJson` tail, so it still catches a
// hand-edited/corrupted value on a current-format document with no `mapGeneratorData` block at all
// (STEP41_PostMigrationImportGaps_IO).
void ParseSimulationDomainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                                MapImportResult& result) {
    ReadAtmosphereJson(document, outRecipe, result);          // ATMOSPHERE_PARAMS_SPEC
    ReadSlopeDefaultsJson(document, outRecipe);                // STEP10_SlopeDefaults_Mechanism
    ReadGeneralMapSettingsJson(document, outRecipe);           // Correction 2
    ClampGeometryBand(outRecipe.geometry, result);              // STEP41_PostMigrationImportGaps_IO
    ReadHeightmapStackJson(document, outRecipe.layerStack);    // Correction 3
    ReadSymmetryJson(document, outRecipe);                     // Correction 4 (STEP16)
    ReadFlowJson(document, outRecipe);                         // Correction 6 (STEP17)
    ReadAccumulationJson(document, outRecipe);                 // Correction 6 (STEP17)
    ReadDetailNormalJson(document, outRecipe);                 // Correction 8 (STEP18)
}

} // namespace

bool MapImporter::ParseSanmapJsonText(const std::string& documentText, Params::MapRecipe& outRecipe,
                                      const MapImportOptions& options, MapImportResult& result,
                                      UnknownImportBag* outUnknownData) {
    nlohmann::json document;
    try {
        document = nlohmann::json::parse(documentText);
    } catch (const std::exception& parseError) {
        result.Log(std::string("JSON parse error: ") + parseError.what());
        return false;
    }
    if (!document.is_object()) { result.Log("The document is not a JSON object."); return false; }

    // The migration runner is the LITERAL FIRST thing that touches the document — before even the
    // envelope reads below. Non-fallible (STEP24_ImportNeverRefuses_IO): version is never grounds
    // to refuse; the runner mutates `document` as far as it safely can and loud-warns instead of
    // gating. Populates `outUnknownData` (ruling 4) with every unrecognized top-level key, when given.
    RunSanmapMigrations(document, result, outUnknownData);

    ParseDocumentEnvelopeJson(document, options, outRecipe);
    ParseEntityDomainsJson(document, outRecipe, result);
    ParseStackDomainsJson(document, outRecipe);
    ParseSimulationDomainsJson(document, outRecipe, result);

    // Absence here is the EXPECTED, NORMAL state for any current-format export (STEP36 stopped the
    // exporter from ever writing this legacy block) — every top-level domain reader above has
    // already recovered everything there is to recover. Informational, not a warning: Log(), not
    // Warn() (STEP38).
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) {
        result.Log("No legacy mapGeneratorData block present: nothing needed it (current-format export).");
        return true;
    }
    const nlohmann::json& generatorData = document["mapGeneratorData"];
    ReadGeometryJson(generatorData, options, outRecipe, result);
    ReadWaterJson(generatorData, outRecipe);
    ReadStrataSettingsJson(generatorData, outRecipe);
    return true;
}

} // namespace Io
} // namespace SanmapGen
