// MapImporter_IO.h — reads a `.sanmap` back into a `Params::MapRecipe`, and (optionally) its
// `Textures/` payload back into `Data::MapFields`. Layer: IO / BRIDGE.
// This is the missing "Load Sanmap" (PARITY_BACKLOG PB-1) and the read half of the
// `mapGeneratorData` round-trip (SANMAP_FORMAT_SPEC).
//
// It LOADS only — it never simulates and never runs a stage (ARCH §3.1). Constitution §6 is the
// governing rule here: every field is validated, a missing or wrong-typed key falls back to the
// recipe's own default and is LOGGED, and a file past the safety caps is rejected outright. No
// input path can crash the importer or put an unverified buffer into RAM.
//
// SCOPE NOTES (ARCH §8.4 — reported, not invented):
//  1. BAKED FIELDS are written into a CALLER-OWNED `Data::MapFields`, never into the pipeline's
//     own fields: every DATA field has exactly one writing stage (ARCH §3.4), and IO is not a
//     stage. Binding an imported heightfield into generation (as v1's "imported RAW layer" did)
//     needs its own work-order.
//  2. ENTITIES: `areas`/`armies` (STEP2_ArmiesAreas_IO), `markers`/`chains`
//     (STEP3_MarkersChains_IO) and `props`/`decals`/`PropGroups`/`DecalGroups`
//     (STEP4_PropsDecals_IO, live-wired by STEP5_PropsDecalsValidation_UI) all round-trip into
//     their `recipe.*` PARAMS homes by `ParseSanmapJsonText` below. `props`/`decals`
//     `blueprintPath` values are NOT resolved against any sanpack here — that is
//     `Io::ValidatePropAndDecalBlueprintPaths` (MapExporter_BlueprintValidation_IO.h), an
//     export-side, warn-not-block check with no read-side counterpart.
//  3. ORDERING LAW — TOP-LEVEL DOMAINS (stated once here; STEP42_ImporterRecipeHeaderTrim_IO):
//     every top-level `.sanmap` domain key (`areas`/`armies`, `markers`/`chains`, `props`/`decals`/
//     `PropGroups`/`DecalGroups`, `atmosphere`, `SlopeDefaults`, `GeneralMapSettings`,
//     `stratumLayers`, `StratumGenerationSettings`, `MarkersStack`/`GlobalMarkerSettings`/
//     `PropsStack`/`DecalsStack`/`UnitsStack`, `Symmetry`, `Flow`/`Accumulation`, `DetailNormal`) is
//     a SIBLING of the legacy `mapGeneratorData` blob, not nested inside it. Every reader for one
//     takes the top-level `document` directly and is called unconditionally, BEFORE the
//     `mapGeneratorData` presence gate at the tail of `ParseSanmapJsonText`
//     (MapImporter_ParseDocument_IO.cpp) — so hand-authored or current-format content survives even
//     with no generator block at all. `MapImporter_Recipe_IO.h`'s per-declaration comments point
//     back here instead of restating it; check each reader's own `.cpp` header comment for any
//     additional, domain-specific ordering constraint beyond this shared one.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Data { class MapFields; class FloatField; struct BakedLayerImage; struct StratumArt; }
namespace Params { struct MapRecipe; }
namespace Io {

struct UnknownImportBag;
class TemplateIngestReport;   // TemplateIngest_IO.h -- only a nullable const-ref parameter is
                              // needed here (STEP96_FootprintBakeAndStalenessCheck_IO.md §3.1).

// Validation caps (Constitution §6) — settings with sane defaults, never literals at a use site.
struct MapImportSafetyLimits {
    std::uint64_t maximumDocumentByteSize = 64ull * 1024 * 1024;
    std::uint64_t maximumTextureByteSize  = 512ull * 1024 * 1024;
    int           minimumMapSize          = 1;
    int           maximumMapSize          = 8192;
};

struct MapImportOptions {
    MapImportSafetyLimits safetyLimits;
    std::string texturesFolderName = "Textures";
    std::string heightmapRawName   = "heightmap.raw";
    std::string stratumMaskLowName  = "stratums_1_4.tga";
    std::string stratumMaskHighName = "stratums_5_8.tga";
    bool bLoadBakedFields = true;    // ignored when the caller passes no field destination
};

// What one import did. `debugLog` is exactly what the Files tab's import-log panel shows.
struct MapImportResult {
    bool        bSucceeded        = false;
    bool        bRecipeLoaded     = false;
    bool        bBakedFieldsLoaded = false;
    // STEP26A: set by Sanmap_MigrationRunner_IO's no-marker branch (IO_MIGRATION_SPEC.md §6) — never
    // resolved to a starting version or walked a migration, since neither SanGenVersion nor its
    // legacy predecessor field was present. This is what STEP26B's "Check for Migrations…" button
    // gates on: only THEN may a human ask for the separate, UI-layer-only preview/selective-apply
    // pass (`Sanmap_MigrationPreview_IO.h`).
    bool        bNoVersionMarkerFound = false;
    int         warningCount      = 0;
    std::string resolvedDocumentPath;
    std::string resolvedFolderPath;
    std::string debugLog;

    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
    void Warn(const std::string& line) { ++warningCount; Log("WARNING: " + line); }
};

// Resolves what the user picked into (document, folder). A `.sanmap` FILE is used directly with
// its parent as the folder; a FOLDER is searched for `mapdef.sanmap` and then for any `.sanmap`
// inside it. False when nothing usable was found.
bool ResolveSanmapDocumentPath(const std::string& pathOrFolder, std::string& outDocumentPath,
                               std::string& outFolderPath);

class MapImporter {
public:
    // The whole action: resolve, read, parse, and (when asked and given a destination) load the
    // baked fields. `outFields` is nullable — see SCOPE NOTE 1. `outUnknownData` is nullable too
    // (STEP24_ImportNeverRefuses_IO ruling 4, `UnknownImportBag_IO.h`) — when given, every
    // genuinely-unrecognized top-level `.sanmap` key lands there instead of being silently dropped.
    // `currentTemplateIngestReport` is nullable and OPTIONAL (STEP96 §3.1 call site 1): when given a
    // resident (already-populated, this-session) ingestion report, a post-load, non-blocking
    // Io::CheckFootprintBakeStaleness pass runs and, if not AllFresh(), surfaces exactly one
    // aggregate `result.Warn(...)`. With nullptr (the default -- no install configured, or nothing
    // ingested this session) the check is skipped entirely, never forcing an ingest.
    // `outBakedLayerImages` is nullable, same caller-owned posture as `outFields` (STEP101): when
    // given, it receives LoadBakedFields's own decomposition output (the pixel behind the single
    // baked layer DecomposeBakedHeightmapIntoLayers injects into `outRecipe.layerStack`, STEP105) —
    // normally `Pipeline::GenerationAssembler::BakedLayerImages()`, so NoiseBlendStage (STEP100) can
    // read it straight back. With nullptr the recipe/fields still load; the decomposed pixels are
    // simply discarded once this call returns.
    // `outStratumArt` is nullable too, the SAME caller-owned posture (STEP105): when given, it
    // receives each stratum's imported mask art at the TGA's own native resolution (never
    // vertexSize-clipped — `Data::StratumArt::importedMask`'s own "any resolution, resampled
    // bilinearly by the Mask stage" contract), normally `Pipeline::GenerationAssembler::StratumArt()`.
    // With nullptr the recipe/fields still load; the loaded mask art is simply discarded once this
    // call returns (the `materialProportions` write into `outFields` is unaffected either way).
    static MapImportResult LoadSanmap(const std::string& pathOrFolder, Params::MapRecipe& outRecipe,
                                      Data::MapFields* outFields,
                                      const MapImportOptions& options = MapImportOptions(),
                                      UnknownImportBag* outUnknownData = nullptr,
                                      const TemplateIngestReport* currentTemplateIngestReport = nullptr,
                                      std::vector<Data::BakedLayerImage>* outBakedLayerImages = nullptr,
                                      std::vector<Data::StratumArt>* outStratumArt = nullptr);

    // Document text -> recipe, with no disk access at all. This is the half the round-trip
    // acceptance test drives against MapExporter::BuildSanmapJsonText. `outUnknownData` — see
    // `LoadSanmap` above.
    static bool ParseSanmapJsonText(const std::string& documentText, Params::MapRecipe& outRecipe,
                                    const MapImportOptions& options, MapImportResult& result,
                                    UnknownImportBag* outUnknownData = nullptr);

    // `<folder>/Textures/*` -> the caller's fields. The fields are RESIZED to the recipe's
    // geometry first, so a payload that disagrees with the document is clipped, never trusted.
    // `documentPath` (STEP109) is the resolved `.sanmap` document path (e.g.
    // `result.resolvedDocumentPath`) — its filename STEM (underscores -> spaces) becomes a
    // fresh-synthesis GeoLayer's own name; see MapImporter_HeightmapDecomposition_IO.h.
    // `recipe` is non-const: once the heightmap itself loads, this injects/re-derives the single
    // baked layer (MapImporter_HeightmapDecomposition_IO.h, STEP105) straight into
    // `recipe.layerStack`, with its pixel landing in `outBakedLayerImages`. `outStratumArt` (STEP105)
    // receives each stratum's imported mask art at native resolution; this call also grows
    // `recipe.strata` and defaults `importedMaskMode` to `StaticOverride` for any stratum that just
    // gained a non-empty imported mask and reads as "unconfigured" -- no slot yet, OR a slot still
    // sitting at its own class default (Disabled). (NOT a size-only test: `Io::MapExporter` always
    // pads `stratumLayers[9]` from `Params::Stratum()` defaults, so `recipe.strata` is already grown
    // to 9 by the time this runs for ANY exported `.sanmap` — see MapImporter_Fields_IO.cpp's own
    // comment for the verified trace.) An already-non-Disabled value (a real SanGen-authored
    // round-tripped map, or this same call's own prior default) is never overwritten.
    static bool LoadBakedFields(const std::string& folderPath, const std::string& documentPath,
                                Params::MapRecipe& recipe,
                                Data::MapFields& outFields, const MapImportOptions& options,
                                MapImportResult& result,
                                std::vector<Data::BakedLayerImage>& outBakedLayerImages,
                                std::vector<Data::StratumArt>& outStratumArt);

    // A validated 16-bit little-endian RAW heightmap into a CALLER-supplied field (STEP102) —
    // `outField` is resized to `vertexSize` x `vertexSize` first, so a payload that disagrees
    // with the caller's request is clipped/flat-padded, never trusted (Constitution §6). This is
    // the same validated body `LoadBakedFields` has always used for `heightmap.raw` (that call
    // site is unchanged — it still targets `outFields.heightfield`); exposed as a public primitive
    // so the Layer Editor's "Import RAW" action (LayerEditor_BakedImage_UI.cpp) can target any
    // `Data::FloatField`, not only the baked-fields destination.
    static bool LoadRawHeightmapIntoField(const std::string& filePath, int vertexSize,
                                          Data::FloatField& outField, const MapImportOptions& options,
                                          MapImportResult& result);
};

} // namespace Io
} // namespace SanmapGen
