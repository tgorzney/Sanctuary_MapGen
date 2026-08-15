// GpuResource_ProgramParts_SYS.cpp — shader-source loading and multi-unit program linking
// for GpuResourceManager (SYS). A compute kernel too large for one file under the ARCH §1.5
// ceiling ships as several GLSL compilation units (one declares main(), the rest provide
// prototyped functions); this file compiles each and links them into one program, keeping
// the same compile-once cache. Source loading and search-path resolution ("resolved path,
// never hardcoded") live beside it in GpuResource_ShaderPath_SYS.cpp.
#include "GpuResource_SYS.h"
#include "GpuGlFunctions_SYS.h"
#include <iostream>

namespace SanmapGen {
namespace Sys {

unsigned GpuResourceManager::CompileShaderUnit(const std::string& source, const std::string& label) {
    GLuint shader = glCreateShaderPointer(kGlComputeShader);
    const GpuGlChar* sourcePointer = source.c_str();
    glShaderSourcePointer(shader, 1, &sourcePointer, nullptr);
    glCompileShaderPointer(shader);
    GLint status = 0;
    glGetShaderivPointer(shader, kGlCompileStatus, &status);
    if (status) return shader;
    GpuGlChar infoLog[1024];
    glGetShaderInfoLogPointer(shader, 1024, nullptr, infoLog);
    std::cerr << "GpuResourceManager: compile failed (" << label << "):\n" << infoLog << "\n";
    glDeleteShaderPointer(shader);
    return 0;
}

GpuProgramHandle GpuResourceManager::GetOrCompileProgramFromParts(
        const std::vector<std::string>& shaderFileNames, const std::string& shaderDefinitions) {
    if (shaderFileNames.empty()) return GpuProgramHandle{};
    std::string key;
    for (const std::string& fileName : shaderFileNames) key += fileName + "\x1e";
    key += "\x1f" + shaderDefinitions;
    for (size_t i = 0; i < programs.size(); ++i)
        if (programs[i].key == key) return GpuProgramHandle{ static_cast<int>(i) };

    std::vector<GLuint> compiledUnits;
    compiledUnits.reserve(shaderFileNames.size());
    bool bAllCompiled = true;
    for (const std::string& fileName : shaderFileNames) {
        bool bLoaded = false;
        const std::string source = LoadShaderSource(fileName, shaderDefinitions, bLoaded);
        GLuint unit = bLoaded ? CompileShaderUnit(source, fileName) : 0;
        if (unit == 0) { bAllCompiled = false; break; }
        compiledUnits.push_back(unit);
    }
    ++compileCount;

    GLuint program = 0;
    if (bAllCompiled) {
        program = glCreateProgramPointer();
        for (GLuint unit : compiledUnits) glAttachShaderPointer(program, unit);
        glLinkProgramPointer(program);
        GLint status = 0;
        glGetProgramivPointer(program, kGlLinkStatus, &status);
        if (!status) {
            GpuGlChar infoLog[1024];
            glGetProgramInfoLogPointer(program, 1024, nullptr, infoLog);
            std::cerr << "GpuResourceManager: link failed (" << shaderFileNames[0] << "):\n" << infoLog << "\n";
            glDeleteProgramPointer(program);
            program = 0;
        }
    }
    for (GLuint unit : compiledUnits) glDeleteShaderPointer(unit);
    if (program == 0) return GpuProgramHandle{};
    programs.push_back(CompiledProgram{ key, program });
    return GpuProgramHandle{ static_cast<int>(programs.size() - 1) };
}

} // namespace Sys
} // namespace SanmapGen
