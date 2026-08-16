// GpuResource_Program_SYS.cpp — program lifecycle, dispatch, and async fences for
// GpuResourceManager (SYS). Compile-once caching lives here; the buffer side is in
// GpuResource_Buffer_SYS.cpp and GpuResource_Texture_SYS.cpp behind the same header; the
// shader search path lives in GpuResource_ShaderPath_SYS.cpp.
#include "GpuResource_SYS.h"
#include "GpuGlFunctions_SYS.h"
#include <iostream>

namespace SanmapGen {
namespace Sys {

GpuResourceManager::~GpuResourceManager() {
    if (!bInitialized) return;
    for (const CompiledProgram& entry : programs)
        if (entry.program) glDeleteProgramPointer(entry.program);
    for (const PersistentBuffer& entry : buffers)
        if (entry.buffer) glDeleteBuffersPointer(1, &entry.buffer);
    for (const ManagedTexture& entry : textures)
        if (entry.texture) glDeleteTextures(1, &entry.texture);
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

    bool bLoaded = false;
    const std::string source = LoadShaderSource(shaderFileName, shaderDefinitions, bLoaded);
    if (!bLoaded) return GpuProgramHandle{};

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
    // Both write targets a kernel can own: SSBOs and image-unit textures. A multi-pass kernel
    // that read-modify-writes its image between dispatches (the preview composite) is only
    // correct if the image writes are visible to the next dispatch, so the barrier covers both
    // kinds rather than the buffer kind alone.
    glMemoryBarrierPointer(kGlShaderStorageBarrierBit | kGlShaderImageAccessBarrierBit);
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
