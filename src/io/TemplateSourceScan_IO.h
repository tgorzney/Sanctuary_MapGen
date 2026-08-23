// TemplateSourceScan_IO.h — resolves the .santp/.sanprop template source roots off a game install
// and yields raw file text, doing NO interpretation of that text (dialect/JSON-sniffing is
// TemplateDialect_IO's job, ticket 87 — Constitution §1 layering: this file only LOADS). Layer: IO.
// First recursive directory walk in src/ (the sole existing std::filesystem::directory_iterator,
// MapImporter_IO.cpp:56, is non-recursive and unrelated).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// Lower rank == higher priority when the SAME templateIdentifier is later found in more than one
// source (TemplateDialect_IO's first-write-wins collision policy, ticket 87). Loose files (either
// LJ/lua's own tree or the Gamedata/Props+Pandemonium loose trees) rank above an extracted/unzipped
// pack tree, which ranks above reading the same content out of a compressed .sanpack —
// DESIGN_SantpFootprintIngestion_R1.md §3.3's own "prefer unzipped over sanpack, cheaper, no
// inflate" ordering, generalized across ALL sources by the same "prefer the more direct source" logic.
enum class TemplateSourceRank { LooseFile = 0, UnzippedPackTree = 1, CompressedSanpack = 2 };

struct TemplateSourceFile {
    // Stable identity for caching (ticket 88) and diagnostics — the real filesystem path for a
    // loose/unzipped source, or "<sanpackPath>!<entryName>" for a zip entry (mirrors
    // Io::SourceFingerprint's own sourcePath usage, AssetAtlasCache_IO.h:38-48).
    std::string        logicalPath;
    std::string        sourceText;
    TemplateSourceRank sourceRank = TemplateSourceRank::LooseFile;
    std::uint64_t       byteSize = 0;
    std::uint64_t       modifiedTime = 0;   // 0 for a sanpack-extracted entry (no independent mtime)
    std::uint64_t       contentHash = 0;    // FNV-1a over sourceText's bytes -- computed here because
                                             // the bytes are already resident (§4.2's "content hashing
                                             // is cheap at this corpus size" argument); ticket 88's
                                             // cache validity check and ticket 89's per-record
                                             // Io::SourceFingerprint both consume this field.
};

// Instrumentation + degrade-gracefully signal (Constitution §6 — a missing subtree is a partial
// ingest, never a hard failure; DESIGN_SantpFootprintIngestion_R1.md §4.4).
struct TemplateSourceScanReport {
    std::vector<TemplateSourceFile> files;
    bool bGamedataRootPresent            = false;   // <root>/engine/Sanctuary_Data/Gamedata
    bool bUnzippedEnvironmentTreePresent = false;   // Environment.sanpack.unzipped/Environment
    bool bEnvironmentSanpackPresent      = false;   // Environment.sanpack (fallback source)
    int  skippedOversizeFileCount        = 0;       // file_size() over the safety cap, never opened
    int  skippedUnreadableFileCount      = 0;       // ReadTextFileBytes/SanpackReader failure
};

// gameInstallRoot is validated by GameInstallLocation_IO (STEP64) BEFORE this call — this function
// does NOT re-validate it, only treats an empty root as "scan nothing" (Constitution §6, total
// behaviour). Walks, IN THIS PRIORITY ORDER: the four LJ/lua loose subtrees (units/props/markers/
// projectiles), the Gamedata/Props + Gamedata/Pandemonium loose trees, then the Environment source
// (the unzipped tree if present, else Environment.sanpack via Io::SanpackReader — NEVER both,
// §3.3's own "double-ingesting is a guaranteed tpId collision storm" warning).
TemplateSourceScanReport ScanTemplateSources(const std::string& gameInstallRoot);

} // namespace Io
} // namespace SanmapGen
