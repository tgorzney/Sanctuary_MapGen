// ApplicationMain_UI.cpp — the process entry point, and nothing else. Layer: UI.
// It resolves the ONE thing the shell cannot know about itself — where its GPU kernels live — and
// hands control to Ui::Application. There is no application logic here: the v1 `main.cpp` that
// owned the regeneration loop, the icon cache and the tab switch is retired (ARCH §5.5).
//
// The shader search path is resolved, never hardcoded (ARCH §4 / GpuResource_SYS.h): an explicit
// argv[1] wins, otherwise the `sangen_shaders` directory the build stages beside the executable.
// So a shipped binary finds its kernels next to itself and a developer can point it elsewhere.
#include "Application_UI.h"
#include <string>
#include <vector>

namespace {

const char* const stagedShaderDirectoryName = "sangen_shaders";
const char* const stagedLuaResourceDirectoryName = "sangen_lua_resources";

// The directory part of argv[0], with its trailing separator; empty when it carries none.
std::string ExecutableDirectory(const char* executablePath) {
    if (executablePath == nullptr) return std::string();
    const std::string path(executablePath);
    const std::size_t lastSeparator = path.find_last_of("/\\");
    return lastSeparator == std::string::npos ? std::string() : path.substr(0, lastSeparator + 1);
}

std::vector<std::string> ResolveShaderSearchDirectories(int argumentCount, char** arguments) {
    std::vector<std::string> searchDirectories;
    if (argumentCount > 1 && arguments[1] != nullptr && arguments[1][0] != '\0')
        searchDirectories.push_back(arguments[1]);
    searchDirectories.push_back(ExecutableDirectory(argumentCount > 0 ? arguments[0] : nullptr) +
                                stagedShaderDirectoryName);
    searchDirectories.push_back(stagedShaderDirectoryName);
    return searchDirectories;
}

// STEP77 — same shape as ResolveShaderSearchDirectories above but SINGLE-directory:
// Io::LoadScenarioRuntimeText takes one directory, not a search list.
std::string ResolveScenarioRuntimeResourceDirectory(int argumentCount, char** arguments) {
    return ExecutableDirectory(argumentCount > 0 ? arguments[0] : nullptr) +
          stagedLuaResourceDirectoryName;
}

} // namespace

int main(int argumentCount, char** arguments) {
    SanmapGen::Ui::ApplicationSettings settings;
    settings.shaderSearchDirectories = ResolveShaderSearchDirectories(argumentCount, arguments);
    settings.scenarioRuntimeResourceDirectory =
        ResolveScenarioRuntimeResourceDirectory(argumentCount, arguments);
    // The atlas ingests the sprite families ASSET_LOADING_SPEC names; the caps travel in settings.
    settings.assetEntryFilter.extensions.push_back(".dds");
    SanmapGen::Ui::Application application(std::move(settings));
    return application.Run();
}
