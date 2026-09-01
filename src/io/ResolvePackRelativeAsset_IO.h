// ResolvePackRelativeAsset_IO.h — resolves a pack-relative asset path (the SAME literal string a
// `Params::PropInstanceGroup::blueprintPath` or a `.santp` `visuals.lods[].model` entry carries,
// e.g. "Environment/01_Highlands/Props/edmm0101/edmm0101.santp") against a validated game install,
// to raw bytes. Layer: IO. Auto-NavMesh mesh-ingestion work (Format Expert ruling): a new,
// generalized primitive — TemplateSourceScan_IO's own unzipped-tree/sanpack-fallback ordering
// (§3.3's "prefer unzipped over sanpack, cheaper, no inflate"), extended past just .santp/.sanprop
// text to ANY Environment-pack-relative asset (here: `.sanmodel` binaries), reusing
// SanpackReader::ExtractFiltered rather than any new zip code.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

struct PackRelativeAssetResult {
    bool                        bSucceeded = false;
    std::string                 errorMessage;   // populated on any failure
    std::vector<unsigned char>  bytes;          // valid only when bSucceeded == true
};

// gameInstallRoot empty, or neither the unzipped tree nor Environment.sanpack resolves
// `packRelativePath`, degrades to a clean failure (Constitution §6) — never throws, never asserts.
// Tries, IN ORDER: `<gameInstallRoot>/engine/Sanctuary_Data/Gamedata/Environment.sanpack.unzipped/
// <packRelativePath>` (a loose file read, no inflate), then the same path looked up as a literal
// zip-entry name inside `.../Gamedata/Environment.sanpack` (SanpackReader) — mirrors
// TemplateSourceScan_IO's own "unzipped tree present -> never also read the sanpack" posture,
// generalized to accept whichever one actually resolves this specific path.
PackRelativeAssetResult ResolvePackRelativeAssetBytes(const std::string& gameInstallRoot,
                                                       const std::string& packRelativePath);

} // namespace Io
} // namespace SanmapGen
