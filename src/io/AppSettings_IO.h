// AppSettings_IO.h — the first persistent file this app has outside `.sanmap` and the asset atlas
// cache: durable, global (not per-map) preferences (SANMAP_FORMAT_SPEC Correction 9's positive
// half). Layer: IO. A plain bag of scalars, NOT a `Params::` type — PARAMS_PIPELINE_SPEC already
// rules this content "app-local, not a recipe field" — and NOT a `Sys::DispatchPolicy` holder
// either, since IO may not depend on SYS. `Ui::Application` is the one unit that legally touches
// both IO and SYS, so it is the caller that translates a loaded `AppSettings` into policy edits.
//
// Format: `nlohmann::json`, flat top-level object, direct 1:1 with the member names below — no
// PascalCase-section convention (that is an ARCH §1.6 `.sanmap` rule) and no `SanGenVersion`; this
// document sits entirely outside the `.sanmap`/migration system (IO_MIGRATION_SPEC.md §1's own
// definition of `Domain` reserves the `MapExporter_<Domain>_IO` pairing for `.sanmap` sections).
//
// Total, never-throwing (Constitution §6): a missing directory/file or corrupt JSON degrades to a
// default-constructed `AppSettings`, logged, never a hard failure — this is convenience state, not
// map data. `directory` is caller-supplied (mirrors `AssetAtlasCache::LoadFromDisk`/`SaveToDisk`'s
// own shape); the struct itself does not hardcode the bootstrap path — that seam is
// AppSettingsLocation_IO.h.
#pragma once
#include <string>

namespace SanmapGen {
namespace Io {

struct AppSettings {
    std::string sanpackPath;
    std::string assetCacheDirectory;
    std::string environmentPackPath;
    std::string gameInstallRoot;               // root of the game install; GameInstallLocation_IO's
                                               // ValidateGameInstallRoot checks <root>/engine/LJ/lua
                                               // and <root>/engine/Sanctuary_Data/Maps
    std::string scenarioRuntimeOverridePath;   // empty => bundled SanGenScenarioRuntime.lua default
                                               // (DESIGN_MapScenarioIO_R1.md §3)
    std::string lastTemplateIngestTimestamp;   // ISO-8601 UTC; empty = never ingested on this app
                                               // installation (Constitution §6: not an error state, an
                                               // ordinary "hasn't happened yet")
    int  lastTemplateIngestEntryCount = 0;     // ingestedFootprintRecordCount from the last completed
                                               // ingest (Io::TemplateIngestReport, STEP89)
    bool bTemplateIngestEnabled = true;        // durable opt-out: false hides the ticket 91 ingestion
                                               // controls entirely, never deletes an existing disk cache
    bool bUseGpuTerrain = true;    // ApplicationExecutionSettings' real default, Application_Execution_UI.h
    bool bUseGpuFlow    = true;    // ditto
    bool bWysiwygBaking = false;   // ditto
    bool bUseGpuMarkers = false;   // placementStage's own DispatchPolicy; no UI toggle exists yet
};

// The file name inside `directory` (Constitution §8 — no literal at a use site).
inline constexpr const char* kAppSettingsFileName = "AppSettings.json";

// Reads `directory/AppSettings.json`. A missing directory/file, an unreadable stream, or malformed
// JSON all degrade to a default-constructed AppSettings, LOGGED, never a hard failure.
AppSettings LoadAppSettings(const std::string& directory);

// Writes `directory/AppSettings.json`, creating `directory` first if needed (EnsureFolderExists,
// MapExporter_IO.h — reused rather than duplicated). False — logged — on an empty directory, a
// folder the platform refused to make, or a stream failure; never a partial file
// (WriteBinaryFileBytes's own contract).
bool SaveAppSettings(const std::string& directory, const AppSettings& settings);

} // namespace Io
} // namespace SanmapGen
