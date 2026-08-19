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
#pragma once
#include <cstdint>
#include <string>

namespace SanmapGen {
namespace Data { class MapFields; }
namespace Params { struct MapRecipe; }
namespace Io {

struct UnknownImportBag;

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
    static MapImportResult LoadSanmap(const std::string& pathOrFolder, Params::MapRecipe& outRecipe,
                                      Data::MapFields* outFields,
                                      const MapImportOptions& options = MapImportOptions(),
                                      UnknownImportBag* outUnknownData = nullptr);

    // Document text -> recipe, with no disk access at all. This is the half the round-trip
    // acceptance test drives against MapExporter::BuildSanmapJsonText. `outUnknownData` — see
    // `LoadSanmap` above.
    static bool ParseSanmapJsonText(const std::string& documentText, Params::MapRecipe& outRecipe,
                                    const MapImportOptions& options, MapImportResult& result,
                                    UnknownImportBag* outUnknownData = nullptr);

    // `<folder>/Textures/*` -> the caller's fields. The fields are RESIZED to the recipe's
    // geometry first, so a payload that disagrees with the document is clipped, never trusted.
    static bool LoadBakedFields(const std::string& folderPath, const Params::MapRecipe& recipe,
                                Data::MapFields& outFields, const MapImportOptions& options,
                                MapImportResult& result);
};

} // namespace Io
} // namespace SanmapGen
