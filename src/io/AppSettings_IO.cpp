// AppSettings_IO.cpp — the round trip: nlohmann::json, flat, direct 1:1 with the member names
// (AppSettings_IO.h Design item 3). Layer: IO. Folder creation and the raw byte write are BORROWED
// from FilesystemPrimitives_IO.h (EnsureFolderExists / JoinExportPath / WriteBinaryFileBytes) rather
// than re-implemented, the same file-write precedent the `.sanmap` writer already sets: a direct
// `ofstream` trunc-write, not atomic (Constitution §6 — this is convenience state, not map data).
#include "AppSettings_IO.h"
#include "JsonPrimitives_IO.h"
#include "FilesystemPrimitives_IO.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <iterator>

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::json ToJson(const AppSettings& settings) {
    nlohmann::json document;
    document["sanpackPath"]          = settings.sanpackPath;
    document["assetCacheDirectory"]  = settings.assetCacheDirectory;
    document["environmentPackPath"]  = settings.environmentPackPath;
    document["gameInstallRoot"]              = settings.gameInstallRoot;
    document["scenarioRuntimeOverridePath"]  = settings.scenarioRuntimeOverridePath;
    document["lastTemplateIngestTimestamp"]  = settings.lastTemplateIngestTimestamp;
    document["lastTemplateIngestEntryCount"] = settings.lastTemplateIngestEntryCount;
    document["bTemplateIngestEnabled"]       = settings.bTemplateIngestEnabled;
    document["bUseGpuTerrain"]       = settings.bUseGpuTerrain;
    document["bUseGpuFlow"]          = settings.bUseGpuFlow;
    document["bWysiwygBaking"]       = settings.bWysiwygBaking;
    document["bUseGpuMarkers"]       = settings.bUseGpuMarkers;
    return document;
}

// Every field is read independently and a miss never overwrites the caller's default, so a
// document that is missing or mistyping ONE key still yields every other field it did carry
// (Constitution §6 — a partial document degrades field-by-field, not all-or-nothing).
void FromJson(const nlohmann::json& document, AppSettings& outSettings) {
    ReadJsonText(document, "sanpackPath", outSettings.sanpackPath);
    ReadJsonText(document, "assetCacheDirectory", outSettings.assetCacheDirectory);
    ReadJsonText(document, "environmentPackPath", outSettings.environmentPackPath);
    ReadJsonText(document, "gameInstallRoot", outSettings.gameInstallRoot);
    ReadJsonText(document, "scenarioRuntimeOverridePath", outSettings.scenarioRuntimeOverridePath);
    ReadJsonText(document, "lastTemplateIngestTimestamp", outSettings.lastTemplateIngestTimestamp);
    ReadJsonInteger(document, "lastTemplateIngestEntryCount", outSettings.lastTemplateIngestEntryCount);
    ReadJsonBoolean(document, "bTemplateIngestEnabled", outSettings.bTemplateIngestEnabled);
    ReadJsonBoolean(document, "bUseGpuTerrain", outSettings.bUseGpuTerrain);
    ReadJsonBoolean(document, "bUseGpuFlow", outSettings.bUseGpuFlow);
    ReadJsonBoolean(document, "bWysiwygBaking", outSettings.bWysiwygBaking);
    ReadJsonBoolean(document, "bUseGpuMarkers", outSettings.bUseGpuMarkers);
}

} // namespace

AppSettings LoadAppSettings(const std::string& directory) {
    AppSettings settings;
    if (directory.empty()) return settings;
    const std::string filePath = JoinExportPath(directory, kAppSettingsFileName);
    std::ifstream inputStream(filePath, std::ios::binary);
    if (!inputStream) {
        std::cerr << "AppSettings: no settings file at '" << filePath << "'; using defaults.\n";
        return settings;
    }
    std::string documentText;
    documentText.assign(std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());
    nlohmann::json document;
    try {
        document = nlohmann::json::parse(documentText);
    } catch (const std::exception& parseError) {
        std::cerr << "AppSettings: '" << filePath << "' is not valid JSON (" << parseError.what()
                   << "); using defaults.\n";
        return settings;
    }
    if (!document.is_object()) {
        std::cerr << "AppSettings: '" << filePath << "' is not a JSON object; using defaults.\n";
        return settings;
    }
    FromJson(document, settings);
    return settings;
}

bool SaveAppSettings(const std::string& directory, const AppSettings& settings) {
    if (directory.empty()) {
        std::cerr << "AppSettings: no destination directory was given.\n";
        return false;
    }
    std::string errorMessage;
    if (!EnsureFolderExists(directory, errorMessage)) {
        std::cerr << "AppSettings: " << errorMessage << "\n";
        return false;
    }
    const std::string filePath = JoinExportPath(directory, kAppSettingsFileName);
    const std::string documentText = ToJson(settings).dump(4);
    if (!WriteBinaryFileBytes(filePath, documentText.data(), documentText.size())) {
        std::cerr << "AppSettings: failed to write '" << filePath << "'.\n";
        return false;
    }
    return true;
}

} // namespace Io
} // namespace SanmapGen
