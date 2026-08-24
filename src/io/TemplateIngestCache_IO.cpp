// TemplateIngestCache_IO.cpp — implementation; see TemplateIngestCache_IO.h's own header comment.
// JSON, not AssetAtlasCache's binary ByteCursor blob format (see the ticket's own "Deliberate
// design decision") — every read/write below goes through nlohmann::json, never a raw byte offset.
// Per-record encode/decode lives in TemplateIngestCache_Record_IO.h/.cpp (ARCH §1.5 aspect split,
// keeps this file under the hard 150-line ceiling).
#include "TemplateIngestCache_IO.h"
#include "TemplateIngestCache_Record_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "JsonPrimitives_IO.h"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <utility>

namespace SanmapGen {
namespace Io {
namespace {

std::uint64_t HashText(const std::string& text) {
    std::uint64_t digest = 14695981039346656037ull;
    for (const unsigned char byte : text) {
        digest ^= byte;
        digest *= 1099511628211ull;      // FNV-1a
    }
    return digest;
}

// The SAME <stem>_<16-hex-FNV1a> scheme as AssetAtlasCache_Fingerprint_IO.cpp's own CacheFileStem
// (AssetAtlasCache_Fingerprint_IO.cpp:45-51), duplicated locally rather than exported from its
// anonymous namespace (ticket 86's own "duplicate the small algorithm" discipline) — keyed on the
// whole gameInstallRoot rather than a single source file, since this cache has no one source file.
std::string CacheFileStem(const std::string& gameInstallRoot) {
    const std::filesystem::path root(gameInstallRoot);
    char digestText[17] = {};
    std::snprintf(digestText, sizeof(digestText), "%016llx",
                  static_cast<unsigned long long>(HashText(gameInstallRoot)));
    return root.filename().string() + "_" + digestText;
}

} // namespace

std::string TemplateIngestCachePathFor(const std::string& cacheDirectory, const std::string& gameInstallRoot) {
    return JoinExportPath(cacheDirectory, CacheFileStem(gameInstallRoot) + ".santpingestcache");
}

bool LoadTemplateIngestCache(const std::string& cacheDirectory, const std::string& gameInstallRoot,
                              const TemplateIngestFingerprint& expected, CachedTemplateIngest& outCache) {
    const std::string filePath = TemplateIngestCachePathFor(cacheDirectory, gameInstallRoot);
    std::string documentText;
    if (!ReadTextFileBytes(filePath, documentText)) return false;   // missing/unreadable file

    nlohmann::json document;
    try {
        document = nlohmann::json::parse(documentText);
    } catch (const nlohmann::json::exception&) {
        return false;   // malformed JSON, never thrown past this function
    }
    if (!document.is_object()) return false;

    // Format-version stamp checked BEFORE the fingerprint (Constitution §7): a reader-logic change
    // invalidates every cache unconditionally, without ever comparing source-file state.
    if (!document.contains("formatVersion") || !document["formatVersion"].is_number_unsigned()) return false;
    if (document["formatVersion"].get<std::uint32_t>() != kTemplateIngestCacheFormatVersion) return false;

    if (!document.contains("fingerprint") || !document["fingerprint"].is_object()) return false;
    const nlohmann::json& fingerprintObject = document["fingerprint"];
    TemplateIngestFingerprint stored;
    if (!ReadJsonInteger(fingerprintObject, "sourceFileCount", stored.sourceFileCount)) return false;
    if (!ReadJsonUnsignedInteger64(fingerprintObject, "combinedContentHash", stored.combinedContentHash))
        return false;
    if (!stored.Matches(expected)) return false;

    if (!document.contains("records") || !document["records"].is_array()) return false;
    std::vector<TemplateRecord> records;
    records.reserve(document["records"].size());
    for (const nlohmann::json& recordObject : document["records"]) {
        TemplateRecord record;
        if (!RecordFromJson(recordObject, record)) return false;   // one bad record aborts the WHOLE load
        records.push_back(std::move(record));
    }

    outCache.fingerprint = stored;
    outCache.records = std::move(records);
    return true;
}

bool SaveTemplateIngestCache(const std::string& cacheDirectory, const std::string& gameInstallRoot,
                              const CachedTemplateIngest& cache) {
    std::string errorMessage;
    if (!EnsureFolderExists(cacheDirectory, errorMessage)) {
        std::cerr << "TemplateIngestCache: " << errorMessage << "\n";
        return false;
    }

    nlohmann::json document;
    document["formatVersion"] = kTemplateIngestCacheFormatVersion;
    document["fingerprint"] = {
        { "sourceFileCount",     cache.fingerprint.sourceFileCount },
        { "combinedContentHash", cache.fingerprint.combinedContentHash },
    };
    nlohmann::json recordsArray = nlohmann::json::array();
    for (const TemplateRecord& record : cache.records) recordsArray.push_back(RecordToJson(record));
    document["records"] = std::move(recordsArray);

    const std::string filePath = TemplateIngestCachePathFor(cacheDirectory, gameInstallRoot);
    const std::string documentText = document.dump(4);
    if (!WriteBinaryFileBytes(filePath, documentText.data(), documentText.size())) {
        std::cerr << "TemplateIngestCache: failed to write '" << filePath << "'.\n";
        return false;
    }
    return true;
}

} // namespace Io
} // namespace SanmapGen
