// GpuResource_Program_SYS.cpp — program lifecycle, dispatch, and async fences for
// GpuResourceManager (SYS). Compile-once caching lives here; the buffer side is in
// GpuResource_Buffer_SYS.cpp behind the same header.
#include "GpuResource_SYS.h"
#include "GpuGlFunctions_SYS.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <utility>

namespace SanmapGen {
namespace Sys {

GpuResourceManager::GpuResourceManager(std::string shaderDirectoryPath)
    : shaderDirectory(std::move(shaderDirectoryPath)) {}

GpuResourceManager::~GpuResourceManager() {
    if (!bInitialized) return;
    for (const CompiledProgram& entry : programs)
        if (entry.program) glDeleteProgramPointer(entry.program);
    for (const PersistentBuffer& entry : buffers)
        if (entry.buffer) glDeleteBuffersPointer(1, &entry.buffer);
}

bool GpuResourceManager::Initialize() {
    if (bInitialized) return true;
    bInitialized = LoadGpuGlFunctions();
    if (!bInitialized) std::cerr << "GpuResourceManager: failed to load GL compute functions.\n";
    return bInitialized;
}

unsigned GpuResourceManager::CompileProgramFromSource(const std::string& source, const std::string& label) {
    GLuint shader = glCreateShaderPointer(kGlComputeShader);
    const GpuGlChar* sourcePointer = source.c_str();
    glShaderSourcePointer(shader, 1, &sourcePointer, nullptr);
    glCompileShaderPointer(shader);
    GLint status = 0;
    glGetShaderivPointer(shader, kGlCompileStatus, &status);
    if (!status) {
        GpuGlChar infoLog[1024];
        glGetShaderInfoLogPointer(shader, 1024, nullptr, infoLog);
        std::cerr << "GpuResourceManager: compile failed (" << label << "):\n" << infoLog << "\n";
        glDeleteShaderPointer(shader);
        return 0;
    }
    GLuint program = glCreateProgramPointer();
    glAttachShaderPointer(program, shader);
    glLinkProgramPointer(program);
    glDeleteShaderPointer(shader);
    glGetProgramivPointer(program, kGlLinkStatus, &status);
    if (!status) {
        GpuGlChar infoLog[1024];
        glGetProgramInfoLogPointer(program, 1024, nullptr, infoLog);
        std::cerr << "GpuResourceManager: link failed (" << label << "):\n" << infoLog << "\n";
        glDeleteProgramPointer(program);
        return 0;
    }
    return program;
}

GpuProgramHandle GpuResourceManager::GetOrCompileProgram(const std::string& shaderFileName,
                                                         const std::string& shaderDefinitions) {
    std::string key = shaderFileName + "\x1f" + shaderDefinitions;
    for (size_t i = 0; i < programs.size(); ++i)
        if (programs[i].key == key) return GpuProgramHandle{ static_cast<int>(i) };

    std::ifstream file(shaderDirectory + "/" + shaderFileName);
    if (!file.is_open()) {
        std::cerr << "GpuResourceManager: cannot open shader '" << shaderFileName
                  << "' under '" << shaderDirectory << "'.\n";
        return GpuProgramHandle{};
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    if (!shaderDefinitions.empty()) {
        size_t afterVersion = source.find('\n');
        if (afterVersion == std::string::npos) afterVersion = source.size() - 1;
        source.insert(afterVersion + 1, shaderDefinitions + "\n");
    }

    GLuint program = CompileProgramFromSource(source, shaderFileName);
    ++compileCount;
    if (program == 0) return GpuProgramHandle{};
    programs.push_back(CompiledProgram{ key, program });
    return GpuProgramHandle{ static_cast<int>(programs.size() - 1) };
}

void GpuResourceManager::SetUniformInt(GpuProgramHandle program, const char* uniformName, int value) {
    if (!program.IsValid()) return;
    GLuint handle = programs[program.programIndex].program;
    glUseProgramPointer(handle);
    glUniform1iPointer(glGetUniformLocationPointer(handle, uniformName), value);
}

void GpuResourceManager::Dispatch(GpuProgramHandle program, unsigned groupsX, unsigned groupsY, unsigned groupsZ) {
    if (!program.IsValid()) return;
    glUseProgramPointer(programs[program.programIndex].program);
    glDispatchComputePointer(groupsX, groupsY, groupsZ);
    glMemoryBarrierPointer(kGlShaderStorageBarrierBit);
}

GpuFenceHandle GpuResourceManager::InsertFence() {
    return GpuFenceHandle{ glFenceSyncPointer(kGlSyncGpuCommandsComplete, 0) };
}

bool GpuResourceManager::IsFenceSignaled(GpuFenceHandle fence) {
    if (!fence.IsValid()) return true;
    GLenum result = glClientWaitSyncPointer(static_cast<GpuGlSync>(fence.syncObject), kGlSyncFlushCommandsBit, 0);
    return result == kGlAlreadySignaled || result == kGlConditionSatisfied;
}

void GpuResourceManager::DeleteFence(GpuFenceHandle fence) {
    if (fence.IsValid()) glDeleteSyncPointer(static_cast<GpuGlSync>(fence.syncObject));
}

} // namespace Sys
} // namespace SanmapGen
