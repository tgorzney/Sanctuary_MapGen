// GpuResource_Buffer_SYS.cpp — persistent SSBO allocation, upload, bind, and readback for
// GpuResourceManager (SYS). Follows PreviewRenderer's persistent-buffer pattern: buffers
// are allocated once and reallocated ONLY on size change, never torn down per dispatch.
#include "GpuResource_SYS.h"
#include "GpuGlFunctions_SYS.h"

namespace SanmapGen {
namespace Sys {

GpuResourceManager::PersistentBuffer* GpuResourceManager::FindBuffer(const std::string& name) {
    for (PersistentBuffer& entry : buffers)
        if (entry.name == name) return &entry;
    return nullptr;
}

bool GpuResourceManager::EnsureBuffer(const std::string& bufferName, size_t byteSize) {
    PersistentBuffer* existing = FindBuffer(bufferName);
    if (existing != nullptr && existing->byteSize == byteSize) return false;  // reuse as-is

    if (existing == nullptr) {
        GLuint buffer = 0;
        glGenBuffersPointer(1, &buffer);
        buffers.push_back(PersistentBuffer{ bufferName, buffer, 0 });
        existing = &buffers.back();
    }
    glBindBufferPointer(kGlShaderStorageBuffer, existing->buffer);
    glBufferDataPointer(kGlShaderStorageBuffer, static_cast<GpuGlSizeiPointer>(byteSize), nullptr, kGlDynamicCopy);
    existing->byteSize = byteSize;
    ++reallocationCount;
    return true;
}

void GpuResourceManager::UploadBuffer(const std::string& bufferName, const void* data, size_t byteSize) {
    PersistentBuffer* target = FindBuffer(bufferName);
    if (target == nullptr) return;
    glBindBufferPointer(kGlShaderStorageBuffer, target->buffer);
    glBufferSubDataPointer(kGlShaderStorageBuffer, 0, static_cast<GpuGlSizeiPointer>(byteSize), data);
}

void GpuResourceManager::ReadbackBuffer(const std::string& bufferName, void* destination, size_t byteSize) {
    PersistentBuffer* target = FindBuffer(bufferName);
    if (target == nullptr) return;
    glBindBufferPointer(kGlShaderStorageBuffer, target->buffer);
    glGetBufferSubDataPointer(kGlShaderStorageBuffer, 0, static_cast<GpuGlSizeiPointer>(byteSize), destination);
}

void GpuResourceManager::BindBuffer(const std::string& bufferName, unsigned bindingIndex) {
    PersistentBuffer* target = FindBuffer(bufferName);
    if (target == nullptr) return;
    glBindBufferBasePointer(kGlShaderStorageBuffer, bindingIndex, target->buffer);
}

} // namespace Sys
} // namespace SanmapGen
