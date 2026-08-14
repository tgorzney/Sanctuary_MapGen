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

class GpuResourceManager {
public:
    explicit GpuResourceManager(std::string shaderDirectory);
    ~GpuResourceManager();
    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;

    // Loads GL entry points; a current GL context must already exist. Idempotent.
    bool Initialize();
    bool IsInitialized() const { return bInitialized; }

    // Compiles once and caches, keyed by (fileName + defines); repeat calls reuse the
    // cached program and do NOT recompile. Paths resolve under the configured shader
    // directory — never a hardcoded absolute path. Invalid handle on failure (logged).
    GpuProgramHandle GetOrCompileProgram(const std::string& shaderFileName,
                                         const std::string& shaderDefinitions = std::string());
    int CompileCount() const { return compileCount; }

    // Persistent buffer keyed by name: allocated on first use, reallocated ONLY when the
    // byte size changes. Returns true when a (re)allocation actually happened.
    bool EnsureBuffer(const std::string& bufferName, size_t byteSize);
    int ReallocationCount() const { return reallocationCount; }

    void UploadBuffer(const std::string& bufferName, const void* data, size_t byteSize);
    void ReadbackBuffer(const std::string& bufferName, void* destination, size_t byteSize);
    void BindBuffer(const std::string& bufferName, unsigned bindingIndex);

    void SetUniformInt(GpuProgramHandle program, const char* uniformName, int value);
    void Dispatch(GpuProgramHandle program, unsigned groupsX, unsigned groupsY, unsigned groupsZ);

    // Async completion: fence after a dispatch, poll without blocking, delete when done.
    GpuFenceHandle InsertFence();
    bool IsFenceSignaled(GpuFenceHandle fence);   // non-blocking (zero-timeout poll)
    void DeleteFence(GpuFenceHandle fence);

private:
    struct CompiledProgram { std::string key; unsigned program; };
    struct PersistentBuffer { std::string name; unsigned buffer; size_t byteSize; };

    unsigned CompileProgramFromSource(const std::string& source, const std::string& label);
    PersistentBuffer* FindBuffer(const std::string& name);

    std::string shaderDirectory;
    std::vector<CompiledProgram> programs;
    std::vector<PersistentBuffer> buffers;
    int compileCount = 0;
    int reallocationCount = 0;
    bool bInitialized = false;
};

} // namespace Sys
} // namespace SanmapGen
