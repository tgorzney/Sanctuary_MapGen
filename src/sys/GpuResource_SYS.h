// GpuResource_SYS.h — the single OpenGL resource owner (SYS).
// Compiles each compute program exactly once (keyed by file + defines), owns persistent
// SSBOs reallocated only on resize, and dispatches asynchronously via GL fences instead
// of a blocking map on the hot path. Realizes DISPATCH_INTERFACE_SPEC §3 and the GPU side
// of the ARCH §4 dispatch contract. GL handles never leak past this seam (Constitution
// §1, ARCH §3.2, §5): the public surface exposes only opaque handles and plain integers.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Sys {

// Named workgroup sizes shared between the GLSL `local_size` (injected as #defines at
// compile) and the C++ dispatch math — retires the duplicated literals (erosion 256,
// avalanche/terrain 16x16, markers 8x8).
namespace WorkgroupSize {
    constexpr int kDropletLinear      = 256; // erosion droplet pass
    constexpr int kFieldTileWidth     = 16;  // avalanche / terrain field passes
    constexpr int kFieldTileHeight    = 16;
    constexpr int kScatterTileWidth   = 8;   // marker / prop scatter passes
    constexpr int kScatterTileHeight  = 8;
}

// Opaque handles — no GL type escapes the seam.
struct GpuProgramHandle {
    int programIndex = -1;
    bool IsValid() const { return programIndex >= 0; }
};
struct GpuFenceHandle {
    void* syncObject = nullptr;
    bool IsValid() const { return syncObject != nullptr; }
};
struct GpuTextureHandle {
    int textureIndex = -1;
    bool IsValid() const { return textureIndex >= 0; }
};

// Texel layouts the managed textures support. RGBA8 is what the preview composite and the
// map canvas need; a further format is added when a caller actually needs one (ARCH §8.4).
enum class GpuTextureFormat { Rgba8 };
// How a compute kernel touches an image-unit binding.
enum class GpuImageAccess { ReadOnly, WriteOnly, ReadWrite };

class GpuResourceManager {
public:
    // The loader resolves a kernel by scanning an ORDERED list of directories, first match
    // wins (an include search path), so every .glsl stays beside its _PROC/_UI twin
    // (ARCH §1.4). The single-directory form is the one-entry path.
    explicit GpuResourceManager(std::string shaderDirectory);
    explicit GpuResourceManager(std::vector<std::string> shaderSearchDirectories);
    ~GpuResourceManager();
    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;

    // Loads GL entry points; a current GL context must already exist. Idempotent.
    bool Initialize();
    bool IsInitialized() const { return bInitialized; }

    // Compiles once and caches, keyed by (fileName + defines); repeat calls reuse the
    // cached program and do NOT recompile. Paths resolve against the configured shader
    // search path — never a hardcoded absolute path. Invalid handle on failure (logged).
    GpuProgramHandle GetOrCompileProgram(const std::string& shaderFileName,
                                         const std::string& shaderDefinitions = std::string());
    // Same compile-once cache for a kernel whose source spans SEVERAL GLSL files (a kernel
    // too large to keep one file inside the ARCH §1.5 ceiling). Each file is one GLSL
    // compilation unit — exactly one declares main(), the others provide functions declared
    // by prototype — and they are linked into a single compute program. Never #include:
    // the files stay independently readable and are resolved on the shader search path.
    GpuProgramHandle GetOrCompileProgramFromParts(const std::vector<std::string>& shaderFileNames,
                                                  const std::string& shaderDefinitions = std::string());
    int CompileCount() const { return compileCount; }

    // Persistent buffer keyed by name: allocated on first use, reallocated ONLY when the
    // byte size changes. Returns true when a (re)allocation actually happened.
    bool EnsureBuffer(const std::string& bufferName, size_t byteSize);
    int ReallocationCount() const { return reallocationCount; }

    void UploadBuffer(const std::string& bufferName, const void* data, size_t byteSize);
    void ReadbackBuffer(const std::string& bufferName, void* destination, size_t byteSize);
    void BindBuffer(const std::string& bufferName, unsigned bindingIndex);

    // Persistent texture keyed by name, same lifecycle as the buffers above: created on
    // first use, storage reallocated ONLY when the dimensions or the format change. The
    // same name always resolves to the same texture; invalid handle on bad dimensions.
    GpuTextureHandle EnsureTexture(const std::string& textureName, int width, int height,
                                   GpuTextureFormat format = GpuTextureFormat::Rgba8);
    int TextureReallocationCount() const { return textureReallocationCount; }

    // Full-surface upload / readback of tightly packed texels; a byteSize smaller than the
    // texture's own footprint is rejected and logged rather than read out of bounds.
    void UploadTexture(GpuTextureHandle texture, const void* data, size_t byteSize);
    void ReadbackTexture(GpuTextureHandle texture, void* destination, size_t byteSize);
    void BindTextureImage(GpuTextureHandle texture, unsigned imageUnit, GpuImageAccess access);
    void BindTextureSampler(GpuTextureHandle texture, unsigned textureUnit);

    void SetUniformInt(GpuProgramHandle program, const char* uniformName, int value);
    void Dispatch(GpuProgramHandle program, unsigned groupsX, unsigned groupsY, unsigned groupsZ);

    // Async completion: fence after a dispatch, poll without blocking, delete when done.
    GpuFenceHandle InsertFence();
    bool IsFenceSignaled(GpuFenceHandle fence);   // non-blocking (zero-timeout poll)
    void DeleteFence(GpuFenceHandle fence);

private:
    struct CompiledProgram { std::string key; unsigned program; };
    struct PersistentBuffer { std::string name; unsigned buffer; size_t byteSize; };
    struct ManagedTexture {
        std::string name; unsigned texture; int width; int height; GpuTextureFormat format;
    };

    unsigned CompileProgramFromSource(const std::string& source, const std::string& label);
    unsigned CompileShaderUnit(const std::string& source, const std::string& label);
    std::string ResolveShaderPath(const std::string& shaderFileName) const;
    std::string LoadShaderSource(const std::string& shaderFileName, const std::string& shaderDefinitions,
                                 bool& bLoaded);
    PersistentBuffer* FindBuffer(const std::string& name);
    int FindTextureIndex(const std::string& name) const;
    ManagedTexture* ResolveTexture(GpuTextureHandle texture);

    std::vector<std::string> shaderSearchDirectories;
    std::vector<CompiledProgram> programs;
    std::vector<PersistentBuffer> buffers;
    std::vector<ManagedTexture> textures;
    int compileCount = 0;
    int reallocationCount = 0;
    int textureReallocationCount = 0;
    bool bInitialized = false;
};

} // namespace Sys
} // namespace SanmapGen
