#include "ScenarioScript_RuntimeResource_IO.h"
#include "FilesystemPrimitives_IO.h"

namespace SanmapGen {
namespace Io {

namespace {
constexpr const char* kBundledRuntimeFileName = "SanGenScenarioRuntime.lua";
}

ScenarioRuntimeResourceResult LoadScenarioRuntimeText(const std::string& runtimeResourceDirectory,
                                                      const std::string& runtimeOverridePathOrEmpty) {
    ScenarioRuntimeResourceResult result;

    if (!runtimeOverridePathOrEmpty.empty()) {
        std::string overrideText;
        if (ReadTextFileBytes(runtimeOverridePathOrEmpty, overrideText)) {
            result.bSucceeded = true;
            result.runtimeLuaText = std::move(overrideText);
            result.sourceDescription = "override";
            return result;
        }
        result.errorMessage = "scenario runtime override path '" + runtimeOverridePathOrEmpty +
            "' could not be read -- degrading to the bundled default runtime.";
        // fall through -- never a silent swap, never a hard failure over the override alone
    }

    const std::string bundledPath = JoinExportPath(runtimeResourceDirectory, kBundledRuntimeFileName);
    std::string bundledText;
    if (ReadTextFileBytes(bundledPath, bundledText)) {
        result.bSucceeded = true;
        result.runtimeLuaText = std::move(bundledText);
        result.sourceDescription = "bundled";
        return result; // errorMessage, if set above, is PRESERVED -- the loud degrade advisory
    }

    result.bSucceeded = false;
    if (!result.errorMessage.empty()) {
        result.errorMessage += " Bundled default at '" + bundledPath + "' was also unreadable.";
    } else {
        result.errorMessage = "bundled scenario runtime resource at '" + bundledPath +
            "' could not be read, and no override path was given.";
    }
    return result;
}

} // namespace Io
} // namespace SanmapGen
