// GpuResource_Texture_SYS.cpp — managed GL textures for GpuResourceManager (SYS). Same
// lifecycle as the persistent SSBOs in GpuResource_Buffer_SYS.cpp: created once on first
// use keyed by name, storage reallocated ONLY when the dimensions or the format change,
// never per frame. Callers hold an opaque GpuTextureHandle and never see a GL name
// (Constitution §1, ARCH §3.2/§5), so the composite can write a real image the canvas
// samples instead of faking one as a packed-uint buffer.
#include "GpuResource_SYS.h"
#include "GpuGlFunctions_SYS.h"
#include <iostream>

namespace SanmapGen {
namespace Sys {

namespace {

struct TextureLayout { GLint internalFormat; GLenum texelFormat; GLenum texelType; int bytesPerTexel; };

TextureLayout LayoutOfFormat(GpuTextureFormat format) {
    switch (format) {
        case GpuTextureFormat::Rgba8:
        default: return TextureLayout{ static_cast<GLint>(kGlRgba8), GL_RGBA, GL_UNSIGNED_BYTE, 4 };
    }
}

GLenum GlAccessOfImageAccess(GpuImageAccess access) {
    switch (access) {
        case GpuImageAccess::ReadOnly:  return kGlReadOnly;
        case GpuImageAccess::WriteOnly: return kGlWriteOnly;
        default:                        return kGlReadWrite;
    }
}

} // namespace

int GpuResourceManager::FindTextureIndex(const std::string& name) const {
    for (size_t index = 0; index < textures.size(); ++index)
        if (textures[index].name == name) return static_cast<int>(index);
    return -1;
}

GpuResourceManager::ManagedTexture* GpuResourceManager::ResolveTexture(GpuTextureHandle texture) {
    if (!texture.IsValid() || static_cast<size_t>(texture.textureIndex) >= textures.size()) return nullptr;
    return &textures[texture.textureIndex];
}

GpuTextureHandle GpuResourceManager::EnsureTexture(const std::string& textureName, int width, int height,
                                                   GpuTextureFormat format) {
    if (width <= 0 || height <= 0) {
        std::cerr << "GpuResourceManager: texture '" << textureName << "' needs positive dimensions.\n";
        return GpuTextureHandle{};
    }
    int index = FindTextureIndex(textureName);
    if (index < 0) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   // one level, no mips
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kGlClampToEdge);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, kGlClampToEdge);
        textures.push_back(ManagedTexture{ textureName, texture, 0, 0, format });
        index = static_cast<int>(textures.size()) - 1;
    }
    ManagedTexture& entry = textures[index];
    if (entry.width == width && entry.height == height && entry.format == format)
        return GpuTextureHandle{ index };                                    // reuse as-is

    const TextureLayout layout = LayoutOfFormat(format);
    glBindTexture(GL_TEXTURE_2D, entry.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, layout.internalFormat, width, height, 0,
                 layout.texelFormat, layout.texelType, nullptr);
    entry.width = width;
    entry.height = height;
    entry.format = format;
    ++textureReallocationCount;
    return GpuTextureHandle{ index };
}

void GpuResourceManager::UploadTexture(GpuTextureHandle texture, const void* data, size_t byteSize) {
    ManagedTexture* target = ResolveTexture(texture);
    if (target == nullptr || data == nullptr) return;
    const TextureLayout layout = LayoutOfFormat(target->format);
    const size_t surfaceBytes = static_cast<size_t>(target->width) * target->height * layout.bytesPerTexel;
    if (byteSize < surfaceBytes) {
        std::cerr << "GpuResourceManager: upload of '" << target->name << "' is short of the surface.\n";
        return;
    }
    glBindTexture(GL_TEXTURE_2D, target->texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, target->width, target->height,
                    layout.texelFormat, layout.texelType, data);
}

void GpuResourceManager::ReadbackTexture(GpuTextureHandle texture, void* destination, size_t byteSize) {
    ManagedTexture* target = ResolveTexture(texture);
    if (target == nullptr || destination == nullptr) return;
    const TextureLayout layout = LayoutOfFormat(target->format);
    const size_t surfaceBytes = static_cast<size_t>(target->width) * target->height * layout.bytesPerTexel;
    if (byteSize < surfaceBytes) {
        std::cerr << "GpuResourceManager: readback of '" << target->name << "' is short of the surface.\n";
        return;
    }
    // Kernel image writes and client uploads must land before the pixels are pulled back.
    glMemoryBarrierPointer(kGlShaderImageAccessBarrierBit | kGlTextureUpdateBarrierBit);
    glBindTexture(GL_TEXTURE_2D, target->texture);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, layout.texelFormat, layout.texelType, destination);
}

void GpuResourceManager::BindTextureImage(GpuTextureHandle texture, unsigned imageUnit, GpuImageAccess access) {
    ManagedTexture* target = ResolveTexture(texture);
    if (target == nullptr) return;
    glBindImageTexturePointer(imageUnit, target->texture, 0, GL_FALSE, 0, GlAccessOfImageAccess(access),
                              static_cast<GLenum>(LayoutOfFormat(target->format).internalFormat));
}

// The presentation identifier a UI toolkit draws with. It is handed out as a plain value so no
// GL type crosses the seam, and the manager keeps owning the texture it names.
unsigned long long GpuResourceManager::TexturePresentationIdentifier(GpuTextureHandle texture) {
    ManagedTexture* target = ResolveTexture(texture);
    return target == nullptr ? 0ull : static_cast<unsigned long long>(target->texture);
}

void GpuResourceManager::BindTextureSampler(GpuTextureHandle texture, unsigned textureUnit) {
    ManagedTexture* target = ResolveTexture(texture);
    if (target == nullptr) return;
    glActiveTexturePointer(static_cast<GLenum>(kGlTextureUnitZero + textureUnit));
    glBindTexture(GL_TEXTURE_2D, target->texture);
}

} // namespace Sys
} // namespace SanmapGen
