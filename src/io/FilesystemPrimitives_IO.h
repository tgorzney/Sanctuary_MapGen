// FilesystemPrimitives_IO.h — the generic, reusable filesystem toolkit (STEP32). Mirrors the
// JsonPrimitives_IO.h precedent: a domain-free primitive header, not export-specific. Consolidates
// `JoinExportPath`/`EnsureFolderExists`/`WriteBinaryFileBytes`, which used to live split across
// MapExporter_IO.h and inconsistently across two unrelated `.cpp`s despite two confirmed
// cross-domain dependents (`AppSettings_IO.cpp`, `MapImporter_Fields_IO.cpp`) that never touch the
// `.sanmap` export domain at all.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// Joins one path segment onto a folder with a single forward slash, whatever the folder ended in.
std::string JoinExportPath(const std::string& folderPath, const std::string& segmentName);

// Creates `folderPath` (and its parents) if it is not already there. False — with the reason in
// `outErrorMessage` — for an empty path or a folder the platform refused to make. The ONE door any
// caller with no `MapExportResult` of its own uses to prepare a destination folder (Constitution
// §6) — `MapExporter_IO.h`'s `EnsureExportFolderExists` is the `MapExportResult`-shaped wrapper
// built on top of this for the export actions themselves.
bool EnsureFolderExists(const std::string& folderPath, std::string& outErrorMessage);

// Writes a blob to disk in one call. False on any stream failure — never a partial success.
bool WriteBinaryFileBytes(const std::string& filePath, const void* bytes, std::size_t byteCount);

// Reads filePath's entire contents as text. false + outText left EMPTY if the file does not exist
// or cannot be opened for read -- never throws, never partial-fills outText on failure. The read
// counterpart to WriteBinaryFileBytes, first needed by ScenarioScript_RuntimeResource_IO's
// (STEP72) bundled/override resolution and later reused by ScenarioScript_Export_IO's (STEP71)
// overwrite-safety banner check.
bool ReadTextFileBytes(const std::string& filePath, std::string& outText);

// Reads filePath's entire contents as raw bytes. False + outBytes left EMPTY if the file does not
// exist or cannot be opened for read — never throws, never partial-fills outBytes on failure. The
// binary counterpart to ReadTextFileBytes/WriteBinaryFileBytes, first needed by the Auto-NavMesh
// mesh-ingestion work's pack-relative asset resolver (a `.sanmodel` is binary, not text).
bool ReadBinaryFileBytes(const std::string& filePath, std::vector<unsigned char>& outBytes);

} // namespace Io
} // namespace SanmapGen
