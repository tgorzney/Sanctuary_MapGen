// MapExporter_IO.h — writes a map to disk: the `.sanmap` JSON plus the `Textures/` payload.
// Layer: IO / BRIDGE. It SAVES only; it never simulates and never runs a stage (ARCH §3.1/§3.2),
// so every pixel it writes is SAMPLED from the baked `Data::MapFields` the pipeline already
// produced — the slope image reads `fields.slope`, it does not re-derive one (ARCH §3.2, the
// shadow-sim rule).
//
// SANMAP_FORMAT_SPEC: "a map on disk is a folder" — `<folder>/<mapName>.sanmap` beside a
// `Textures/` subfolder. "Terrain data is NOT in the JSON": the heightmap and the stratum masks
// are files in that folder. The generator's own settings ride in the `mapGeneratorData` block —
// the .sanmap is the single source of truth, and there is deliberately NO `.json` generator file
// (TAB_REBUILD_PLAN "Files / Save · Removed").
//
// SCOPE NOTES (ARCH §8.4 — reported, not invented):
//  1. RESOLVED ENTITIES (`Data::PlacementResults` markers/props/units) are not written: inputs are
//     the recipe and the fields. `areas`/`armies`/`markers`/`chains`/`props`/`decals` all round-trip
//     real `recipe.*` content (STEP2/STEP3/STEP4; blueprintPath validation added by STEP5). An
//     unresolved `props`/`decals` blueprintPath is never dropped from the written recipe content —
//     this builder itself stays disk-free and never refuses. Whether the WRITE that follows is
//     allowed to proceed on an unresolved path is `MapExporter::ExportSanmapOnly`/`ExportAll`'s own
//     call (`bBlueprintValidationAcknowledged`, STEP39_BlueprintValidationGate_IO), enforced above
//     this builder, not inside it.
//  2. RETIRED: ATMOSPHERE now has a `_PARAMS` home (`Params::Atmosphere`, ATMOSPHERE_PARAMS_SPEC)
//     and round-trips real `recipe.atmosphere` content via `BuildAtmosphereJson`
//     (MapExporter_Atmosphere_IO.cpp) — no longer written from the format's own defaults.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Data { class MapFields; }
namespace Params { struct MapRecipe; }
namespace Io {

class SanpackReader;
struct UnknownImportBag;

// Every output file name is a setting, never a literal at a write site (Constitution §8).
struct MapExportFileNames {
    std::string texturesFolderName = "Textures";
    std::string heightmapRawName   = "heightmap.raw";
    std::string slopeImageName     = "slope.png";
    std::string flowImageName      = "flow.png";
    std::string stratumMaskLowName  = "stratums_1_4.tga";   // surface weights 0..3
    std::string stratumMaskHighName = "stratums_5_8.tga";   // surface weights 4..7
};

struct MapExportOptions {
    // `mapName`/`mapCredits` MOVED to `Params::MapRecipe` (STEP25_MapNameCredits_IO) — they are
    // real, importable document content, not export-run-only options; see MapExporter_Recipe_IO.cpp
    // and MapImporter_IO.cpp's ParseSanmapJsonText.
    MapExportFileNames fileNames;

    int  jsonIndentSpaceCount = 4;
    bool bWriteHeightmapRaw   = true;
    bool bWriteStratumMasks   = true;
    bool bWriteSlopeImage     = true;
    bool bWriteFlowImage      = true;

    // Display mappings for the two visualization images (they are pictures, not data):
    // slope is shown across 0..slopeDisplayMaximumDegrees, flow is multiplied for visibility.
    float slopeDisplayMaximumDegrees = 90.0f;
    float flowDisplayMultiplier      = 100.0f;
};

// What one export did. `debugLog` is what the Files tab's log panel shows.
struct MapExportResult {
    bool                     bSucceeded = false;
    std::vector<std::string> writtenFilePaths;
    std::string              debugLog;
    int  warningCount = 0;

    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
    void Warn(const std::string& line) { ++warningCount; Log("WARNING: " + line); }
    void RecordWrittenFile(const std::string& filePath) { writtenFilePaths.push_back(filePath); }
    int WrittenFileCount() const { return static_cast<int>(writtenFilePaths.size()); }
};

// Fixed by the format (SANMAP_FORMAT_SPEC "Container").
inline constexpr int sanmapFileVersion   = 3;
inline constexpr int sanmapMapVersion    = 1;
inline constexpr int sanmapStratumCount  = 9;
// `SanGenVersion` (SANMAP_FORMAT_SPEC Correction 1) REPLACES the old write-only
// `mapGeneratorDataVersion` literal — it is written top-level from `Io::kCurrentSanGenVersion`
// (Sanmap_MigrationManifest_IO.h), the single source of truth `Sanmap_MigrationRunner_IO` reads
// back on import (IO_MIGRATION_SPEC.md §3/§4).

// STEP32 split, under this header's ARCH §1.5 ceiling: the quantizers moved to
// MapExporter_SampleQuantize_IO.h, `BlueprintValidationReport`/`ValidatePropAndDecalBlueprintPaths`
// moved to MapExporter_BlueprintValidation_IO.h, and the generic (never exporter-specific)
// `JoinExportPath`/`EnsureFolderExists`/`WriteBinaryFileBytes` moved to FilesystemPrimitives_IO.h.
// `EnsureExportFolderExists` below, the `MapExportResult`-shaped wrapper, stays here.

// Creates a destination folder (and its parents) if it is not already there. False — with the
// reason logged — for an empty path or a folder the platform refused to make. This is the ONE
// door a caller of the individual Write* actions below uses to prepare their destination, so no
// layer above IO ever touches the filesystem itself (ARCH §3.3 — IO owns the format seam).
bool EnsureExportFolderExists(const std::string& folderPath, MapExportResult& result);

class MapExporter {
public:
    // `<folder>/<mapName>.sanmap`, no textures. `assetPack` non-null runs the blueprintPath safety
    // net (STEP39_BlueprintValidationGate_IO): if any prop/decal blueprintPath fails to resolve
    // against it, the export REFUSES (bSucceeded stays false, nothing is written) UNLESS the caller
    // already passed `bBlueprintValidationAcknowledged = true` — the same choice the Files tab's own
    // confirm dialog's "Export Anyway" button represents (FilesTab_Draw_UI.cpp). This is a
    // refuse-by-default gate, not a silent skip: a future caller that never thought about
    // blueprintPaths at all gets refused rather than shipping a `.sanmap` the game aborts loading
    // partway through (SANMAP_FORMAT_SPEC). `assetPack == nullptr` skips validation entirely — the
    // pre-STEP39 contract for a caller with no asset pack loaded. `unknownData` is nullable
    // (STEP24_ImportNeverRefuses_IO ruling 6, `UnknownImportBag_IO.h`) — when given (the same bag
    // the load-edit-save session's own `LoadSanmap`/`ParseSanmapJsonText` call populated), its
    // captured keys re-merge as one nested `UnknownImport` object in the re-exported document
    // (STEP28_UnknownImportNesting_IO) — never colliding with a known-domain field, since the bag's
    // content never lands at the document's own top level.
    static MapExportResult ExportSanmapOnly(const std::string& folderPath,
                                            const Params::MapRecipe& recipe,
                                            const MapExportOptions& options = MapExportOptions(),
                                            const SanpackReader* assetPack = nullptr,
                                            const UnknownImportBag* unknownData = nullptr,
                                            bool bBlueprintValidationAcknowledged = false);

    // The recipe plus every enabled texture. `assetPack`/`unknownData`/
    // `bBlueprintValidationAcknowledged` — see ExportSanmapOnly above.
    static MapExportResult ExportAll(const std::string& folderPath, const Params::MapRecipe& recipe,
                                     const Data::MapFields& fields,
                                     const MapExportOptions& options = MapExportOptions(),
                                     const SanpackReader* assetPack = nullptr,
                                     const UnknownImportBag* unknownData = nullptr,
                                     bool bBlueprintValidationAcknowledged = false);

    // The individual export actions the Files tab offers, each usable on its own.
    static bool WriteHeightmapRaw(const std::string& filePath, const Data::MapFields& fields,
                                  MapExportResult& result);
    static bool WriteSlopeImage(const std::string& filePath, const Data::MapFields& fields,
                                const MapExportOptions& options, MapExportResult& result);
    static bool WriteFlowImage(const std::string& filePath, const Data::MapFields& fields,
                               const MapExportOptions& options, MapExportResult& result);
    static bool WriteStratumMaskImages(const std::string& folderPath, const Data::MapFields& fields,
                                       const MapExportOptions& options, MapExportResult& result);

    // recipe -> the whole `.sanmap` document, as text. Pure and disk-free: this is the half the
    // round-trip acceptance test drives against MapImporter. `unknownData` is nullable — see
    // `ExportSanmapOnly` above; the re-merge is the LAST write, immediately before `document.dump()`
    // (ruling 6), writing the whole bag under the single `UnknownImport` key — a no-op when the bag
    // is empty (no key is written at all).
    static std::string BuildSanmapJsonText(const Params::MapRecipe& recipe,
                                           const MapExportOptions& options = MapExportOptions(),
                                           const UnknownImportBag* unknownData = nullptr);
};

} // namespace Io
} // namespace SanmapGen
