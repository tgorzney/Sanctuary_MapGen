// GameInstallLocation_IO.cpp — the two-subpath filesystem check (GameInstallLocation_IO.h). Reuses
// FilesystemPrimitives_IO.h's JoinExportPath rather than hand-rolling path joins (IO Architecture
// conventions). Total, never-throwing (Constitution §6): an empty candidate root is refused before
// any filesystem call is attempted, never a call on an empty path.
#include "GameInstallLocation_IO.h"
#include "FilesystemPrimitives_IO.h"
#include <filesystem>

namespace SanmapGen {
namespace Io {

GameInstallRootValidation ValidateGameInstallRoot(const std::string& candidateRoot) {
    GameInstallRootValidation validation;
    if (candidateRoot.empty()) {
        validation.reason = "no game install root was given.";
        return validation;
    }

    const std::string engineRoot     = JoinExportPath(candidateRoot, "engine");
    const std::string luaScriptPath  = JoinExportPath(engineRoot, "LJ/lua");
    const std::string mapAssetPath   = JoinExportPath(engineRoot, "Sanctuary_Data/Maps");

    std::error_code luaCheckError;
    std::error_code mapCheckError;
    const bool bHasLuaScriptPath = std::filesystem::is_directory(luaScriptPath, luaCheckError);
    const bool bHasMapAssetPath  = std::filesystem::is_directory(mapAssetPath, mapCheckError);

    if (bHasLuaScriptPath && bHasMapAssetPath) {
        validation.bValid = true;
        return validation;
    }

    if (!bHasLuaScriptPath && !bHasMapAssetPath) {
        validation.reason = "neither '" + luaScriptPath + "' nor '" + mapAssetPath + "' exists.";
    } else if (!bHasLuaScriptPath) {
        validation.reason = "'" + luaScriptPath + "' does not exist.";
    } else {
        validation.reason = "'" + mapAssetPath + "' does not exist.";
    }
    return validation;
}

} // namespace Io
} // namespace SanmapGen
