// GpuResource_Texture_SYS_Test.cpp — acceptance checks for the managed GL texture (M5-0b):
// create, resize-only-on-change, CPU upload -> GPU readback round-trip byte-for-byte, and a
// compute kernel writing the texture through its image unit. Runs against the shared hidden
// GL context created by GpuResource_SYS_Test.cpp.
#include "GpuResource_SYS.h"
#include "GpuResource_TestSupport_SYS.h"
#include <string>
#include <vector>

using namespace SanmapGen::Sys;

namespace GpuResourceTest {

namespace {

// Writes each texel's own coordinates into R/G so a mis-ordered readback is visible.
const char* const kImageWriteKernel =
    "#version 430 core\n"
    "layout(local_size_x = 8, local_size_y = 8) in;\n"
    "layout(rgba8, binding = 0) uniform writeonly image2D destinationImage;\n"
    "uniform int surfaceWidth;\n"
    "uniform int surfaceHeight;\n"
    "void main() {\n"
    "    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);\n"
    "    if (texel.x >= surfaceWidth || texel.y >= surfaceHeight) return;\n"
    "    imageStore(destinationImage, texel, vec4(float(texel.x) / 255.0, float(texel.y) / 255.0, 0.0, 1.0));\n"
    "}\n";

std::vector<unsigned char> MakeRgba8Pattern(int width, int height) {
    std::vector<unsigned char> pattern(static_cast<size_t>(width) * height * 4);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            unsigned char* texel = &pattern[(static_cast<size_t>(y) * width + x) * 4];
            texel[0] = static_cast<unsigned char>(x * 7 + 1);
            texel[1] = static_cast<unsigned char>(y * 11 + 2);
            texel[2] = static_cast<unsigned char>((x * y) & 0xFF);
            texel[3] = 255;
        }
    return pattern;
}

} // namespace

void RunTextureChecks(const std::string& shaderDirectory) {
    const std::string shaderName = "ImageWrite_Test.glsl";
    WriteTestFile(shaderDirectory + "/" + shaderName, kImageWriteKernel);

    GpuResourceManager manager(shaderDirectory);
    Check(manager.Initialize(), "texture manager initializes against the GL loader");

    // 1. Lifecycle: allocate once, reuse at the same size, reallocate on a size change.
    const int width = 8, height = 6;
    GpuTextureHandle texture = manager.EnsureTexture("preview", width, height, GpuTextureFormat::Rgba8);
    Check(texture.IsValid(), "texture is created on first use");
    Check(manager.TextureReallocationCount() == 1, "first EnsureTexture allocates");
    GpuTextureHandle sameTexture = manager.EnsureTexture("preview", width, height);
    Check(sameTexture.textureIndex == texture.textureIndex, "same name resolves to the same texture");
    Check(manager.TextureReallocationCount() == 1, "same-size EnsureTexture reuses (no realloc)");
    manager.EnsureTexture("preview", width * 2, height);
    Check(manager.TextureReallocationCount() == 2, "a dimension change reallocates exactly once");
    texture = manager.EnsureTexture("preview", width, height);
    Check(manager.TextureReallocationCount() == 3, "shrinking back reallocates");
    Check(!manager.EnsureTexture("bad", 0, 4).IsValid(), "zero-sized texture is rejected, not created");

    // 2. Round-trip: CPU pattern -> texture -> CPU, byte-for-byte.
    const std::vector<unsigned char> uploaded = MakeRgba8Pattern(width, height);
    manager.UploadTexture(texture, uploaded.data(), uploaded.size());
    std::vector<unsigned char> readBack(uploaded.size(), 0);
    manager.ReadbackTexture(texture, readBack.data(), readBack.size());
    Check(readBack == uploaded, "RGBA8 upload/readback round-trips byte-for-byte");

    // 3. A compute kernel writes the texture through its image unit.
    GpuProgramHandle program = manager.GetOrCompileProgram(shaderName);
    Check(program.IsValid(), "image-write kernel compiles");
    manager.BindTextureImage(texture, 0, GpuImageAccess::WriteOnly);
    manager.SetUniformInt(program, "surfaceWidth", width);
    manager.SetUniformInt(program, "surfaceHeight", height);
    manager.Dispatch(program, (width + 7) / 8, (height + 7) / 8, 1);
    std::vector<unsigned char> kernelOutput(uploaded.size(), 0);
    manager.ReadbackTexture(texture, kernelOutput.data(), kernelOutput.size());
    bool bKernelWroteEveryTexel = true;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const unsigned char* texel = &kernelOutput[(static_cast<size_t>(y) * width + x) * 4];
            if (texel[0] != x || texel[1] != y || texel[2] != 0 || texel[3] != 255)
                bKernelWroteEveryTexel = false;
        }
    Check(bKernelWroteEveryTexel, "compute kernel writes the bound image, readback matches");
}

} // namespace GpuResourceTest
