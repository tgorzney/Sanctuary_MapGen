// GpuResource_ShaderPath_SYS.cpp — shader search-path resolution and source loading for
// GpuResourceManager (SYS). The manager is configured with an ORDERED list of directories
// and resolves a kernel by scanning it, first match wins, exactly like an include search
// path. That keeps each .glsl beside its _PROC/_UI twin (ARCH §1.4) instead of forcing the
// build to stage every kernel into one directory. Paths are always relative to a configured
// directory — never hardcoded and never absolute inside the code.
#include "GpuResource_SYS.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <utility>

namespace SanmapGen {
namespace Sys {

namespace {

std::string DescribeSearchPath(const std::vector<std::string>& searchDirectories) {
    std::string description;
    for (const std::string& directory : searchDirectories) {
        if (!description.empty()) description += "; ";
        description += directory;
    }
    return description.empty() ? std::string("<empty search path>") : description;
}

} // namespace

GpuResourceManager::GpuResourceManager(std::string shaderDirectory)
    : shaderSearchDirectories(1, std::move(shaderDirectory)) {}

GpuResourceManager::GpuResourceManager(std::vector<std::string> searchDirectories)
    : shaderSearchDirectories(std::move(searchDirectories)) {}

std::string GpuResourceManager::ResolveShaderPath(const std::string& shaderFileName) const {
    for (const std::string& directory : shaderSearchDirectories) {
        std::string candidatePath = directory.empty() ? shaderFileName : directory + "/" + shaderFileName;
        std::ifstream probe(candidatePath);
        if (probe.is_open()) return candidatePath;   // first match wins
    }
    return std::string();
}

std::string GpuResourceManager::LoadShaderSource(const std::string& shaderFileName,
                                                 const std::string& shaderDefinitions, bool& bLoaded) {
    const std::string resolvedPath = ResolveShaderPath(shaderFileName);
    std::ifstream file(resolvedPath);
    if (resolvedPath.empty() || !file.is_open()) {
        std::cerr << "GpuResourceManager: cannot open shader '" << shaderFileName
                  << "' on the search path (" << DescribeSearchPath(shaderSearchDirectories) << ").\n";
        bLoaded = false;
        return std::string();
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    if (!shaderDefinitions.empty()) {
        size_t afterVersion = source.find('\n');            // definitions go below #version
        if (afterVersion == std::string::npos) afterVersion = source.size() - 1;
        source.insert(afterVersion + 1, shaderDefinitions + "\n");
    }
    bLoaded = true;
    return source;
}

} // namespace Sys
} // namespace SanmapGen
