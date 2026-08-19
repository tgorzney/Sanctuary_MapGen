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
//     unresolved `props`/`decals` blueprintPath is reported, never dropped, never used to block the
//     export (`ValidatePropAndDecalBlueprintPaths` below) — this builder stays disk-free.
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
    std::string mapName    = "mapdef";
    std::string mapCredits = "Sanctuary Map Generator";
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

    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
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

// 0..1 -> the format's 16-bit heightmap sample. Out-of-range input is clamped, never wrapped.
inline unsigned short QuantizeNormalizedHeightSample(float normalizedHeight) {
    if (!(normalizedHeight > 0.0f)) return 0u;                       // also catches NaN
    if (normalizedHeight >= 1.0f) return 65535u;
    return static_cast<unsigned short>(normalizedHeight * 65535.0f + 0.5f);
}

// 0..1 -> one 8-bit mask/visualization channel. Same clamping contract.
inline unsigned char QuantizeNormalizedWeightSample(float normalizedWeight) {
    if (!(normalizedWeight > 0.0f)) return 0u;
    if (normalizedWeight >= 1.0f) return 255u;
    return static_cast<unsigned char>(normalizedWeight * 255.0f + 0.5f);
}

// One `props`/`decals` blueprintPath validation pass (MapExporter_BlueprintValidation_IO.cpp).
// Warn-not-block: AllResolved() gates nothing here — the caller decides what to do with a finding.
struct BlueprintValidationReport {
    std::vector<std::string> unresolvedBlueprintPaths;   // literal strings, props then decals
    bool AllResolved() const { return unresolvedBlueprintPaths.empty(); }
    std::string SummaryText() const;   // ONE wording — shared by the UI dialog body and debugLog
};

// `assetPack` MUST already be `Open()`+`ReadCentralDirectoryOnce()`'d by the caller. Pure/read-only,
// touches no disk, never called from inside BuildSanmapJsonText (same tier as `recipe.IsValid()`).
BlueprintValidationReport ValidatePropAndDecalBlueprintPaths(const Params::MapRecipe& recipe,
                                                              const SanpackReader& assetPack);

// Joins one path segment onto a folder with a single forward slash, whatever the folder ended in.
std::string JoinExportPath(const std::string& folderPath, const std::string& segmentName);

// The result-agnostic core: creates `folderPath` (and its parents) if it is not already there.
// False — with the reason in `outErrorMessage` — for an empty path or a folder the platform
// refused to make. `EnsureExportFolderExists` below is the `MapExportResult`-shaped wrapper every
// Write* action here uses; a caller with no `MapExportResult` of its own (AppSettings_IO's
// clean-shutdown save, STEP19_AppSettings_IO) uses this one directly instead of inventing a
// duplicate folder-creation path (Constitution §6).
bool EnsureFolderExists(const std::string& folderPath, std::string& outErrorMessage);

// Creates a destination folder (and its parents) if it is not already there. False — with the
// reason logged — for an empty path or a folder the platform refused to make. This is the ONE
// door a caller of the individual Write* actions below uses to prepare their destination, so no
// layer above IO ever touches the filesystem itself (ARCH §3.3 — IO owns the format seam).
bool EnsureExportFolderExists(const std::string& folderPath, MapExportResult& result);

// Writes a blob to disk in one call. False on any stream failure — never a partial success.
bool WriteBinaryFileBytes(const std::string& filePath, const void* bytes, std::size_t byteCount);

class MapExporter {
public:
    // `<folder>/<mapName>.sanmap`, no textures. `assetPack` non-null LOGS a blueprintPath finding
    // (never gates bSucceeded — warn-not-block).
    static MapExportResult ExportSanmapOnly(const std::string& folderPath,
                                            const Params::MapRecipe& recipe,
                                            const MapExportOptions& options = MapExportOptions(),
                                            const SanpackReader* assetPack = nullptr);

    // The recipe plus every enabled texture. `assetPack` — see ExportSanmapOnly above.
    static MapExportResult ExportAll(const std::string& folderPath, const Params::MapRecipe& recipe,
                                     const Data::MapFields& fields,
                                     const MapExportOptions& options = MapExportOptions(),
                                     const SanpackReader* assetPack = nullptr);

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
    // round-trip acceptance test drives against MapImporter.
    static std::string BuildSanmapJsonText(const Params::MapRecipe& recipe,
                                           const MapExportOptions& options = MapExportOptions());
};

} // namespace Io
} // namespace SanmapGen
