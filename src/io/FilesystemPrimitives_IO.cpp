// FilesystemPrimitives_IO.cpp — implementations for the 3 generic filesystem primitives (STEP32).
// Consolidates what used to be split inconsistently across MapExporter_IO.cpp
// (`EnsureFolderExists`) and MapExporter_Textures_IO.cpp (`JoinExportPath`/`WriteBinaryFileBytes`)
// into their one real, domain-free home.
#include "FilesystemPrimitives_IO.h"
#include <filesystem>
#include <fstream>

namespace SanmapGen {
namespace Io {

std::string JoinExportPath(const std::string& folderPath, const std::string& segmentName) {
    if (folderPath.empty()) return segmentName;
    const char lastCharacter = folderPath[folderPath.size() - 1];
    if (lastCharacter == '/' || lastCharacter == '\\') return folderPath + segmentName;
    return folderPath + "/" + segmentName;
}

bool EnsureFolderExists(const std::string& folderPath, std::string& outErrorMessage) {
    if (folderPath.empty()) { outErrorMessage = "no destination folder was given."; return false; }
    std::error_code folderError;
    const std::filesystem::path folder(folderPath);
    std::filesystem::create_directories(folder, folderError);
    if (folderError && !std::filesystem::exists(folder)) {
        outErrorMessage = "could not create " + folderPath;
        return false;
    }
    return true;
}

bool WriteBinaryFileBytes(const std::string& filePath, const void* bytes, std::size_t byteCount) {
    if (bytes == nullptr && byteCount > 0) return false;
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    if (!outputStream) return false;
    if (byteCount > 0)
        outputStream.write(static_cast<const char*>(bytes), static_cast<std::streamsize>(byteCount));
    outputStream.flush();
    return static_cast<bool>(outputStream);
}

bool ReadTextFileBytes(const std::string& filePath, std::string& outText) {
    std::ifstream inputStream(filePath, std::ios::binary);
    if (!inputStream) return false;
    outText.assign(std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());
    return true;
}

} // namespace Io
} // namespace SanmapGen
