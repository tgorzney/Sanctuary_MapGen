// ScenarioScript_AreaImport_IO.h -- the ONE disk-touching, human-triggered entry point for
// ARCH_15_11's foreign-scenario-.lua area-rectangle carve-out
// (`ARCH_15_11_ForeignScenarioAreaImport.md` §15.11). Layer: IO. Reads a FOREIGN (non-SanGen
// -authored) scenario .lua file, enforces the refusal guard (item 1) and the byte-size cap (item 10)
// BEFORE calling the pure extractor (ScenarioScript_AreaRectangleExtract_IO.h), then reconciles the
// returned rectangles into recipe.areas additively (item 9). Human-triggered, one-shot, no live
// binding, no provenance field, no re-sync action (item 8) -- never called from map open, export,
// generate, or any dirty-hash recompute.
//
// This is NOT "the reader half" of ScenarioScript_DataLua_IO (item 11) -- it reads a filename set
// DISJOINT from what that file (and ScenarioScript_Export_IO) ever write, and that disjointness is
// mechanically checked below, not merely conventional.
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "ScenarioScript_AreaRectangleExtract_IO.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// Constitution §6 byte cap (item 10) -- checked via a filesystem stat BEFORE the file is ever read
// into memory, so an oversized file is never loaded to find out it's oversized. Chosen generously
// above every real scenario script observed (map_scripts_backup's officialbak files are all well
// under 32 KB) while still bounding pathological input.
inline constexpr std::size_t kMaxScenarioAreaImportSourceBytes = 4u * 1024u * 1024u;   // 4 MiB

// Mirrors ScenarioExportResult's own shape (bool flags + string lists + Log()) -- a DISTINCT type,
// never merged with it: this is an import, not an export.
struct ScenarioAreaImportResult {
    bool bRefusedGeneratedFile      = false;   // filename or banner-line match -- item 1
    bool bRefusedUnreadableFile     = false;   // file missing, unreadable, or a stat failure
    bool bRefusedOversizedFile      = false;   // byte cap exceeded -- item 10; never scanned
    bool bRectangleCountCapExceeded = false;   // carried through from the pure extractor -- item 10

    std::vector<std::string>                    writtenNames;           // newly added to recipe.areas
    std::vector<std::string>                    skippedCollisionNames;  // name already existed --
                                                                          // skipped and reported, item 9
    std::vector<std::string>                    collisionIdentifiers;   // in-FILE reassignments,
                                                                          // carried through from the
                                                                          // extractor -- item 7
    std::vector<ScenarioAreaExtractionNearMiss>  nearMisses;             // carried through verbatim,
                                                                          // item 6

    std::string debugLog;
    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
};

// sourceFilePath: any file the human picked (e.g. via FileDialog::OpenFilePath filtered to "*.lua"
// -- UI wiring is explicit follow-up, not built by this ticket). recipe.areas is mutated additively,
// in place, on success -- left completely untouched on every refusal path. This function itself
// never opens a platform dialog and never has a default/implicit invocation path (item 8).
ScenarioAreaImportResult ImportAreaRectanglesFromScenarioScriptFile(const std::string& sourceFilePath,
                                                                    Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
