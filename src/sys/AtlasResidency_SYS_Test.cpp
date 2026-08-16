// AtlasResidency_SYS_Test.cpp — M5-4 acceptance, the residency half: the CPU-side atlas pages
// the IO layer produced become GPU textures the UI can sample, and they do so ONLY through
// GpuResource_SYS (ARCH §3.2). Needs a live GL context, so it reuses the M5-0b hidden-window
// harness and is its own binary — the ingestion checks stay headless.
#include "AtlasResidency_SYS.h"
#include "GpuResource_TestSupport_SYS.h"
#include <vector>

using namespace SanmapGen::Sys;
using namespace GpuResourceTest;

namespace {

// Stands in for an Io::AtlasImage page; SYS never sees the IO type (ARCH §3.1).
std::vector<unsigned char> MakePageSurface(int width, int height, unsigned char red) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
    for (std::size_t texel = 0; texel < pixels.size(); texel += 4) {
        pixels[texel] = red;
        pixels[texel + 1] = static_cast<unsigned char>(texel / 4);
        pixels[texel + 2] = 0;
        pixels[texel + 3] = 255;
    }
    return pixels;
}

} // namespace

int main(int argc, char** argv) {
    const std::string shaderDirectory = (argc > 1) ? argv[1] : ".";
    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("SKIP: no GL context available in this environment\n");
        return 2;
    }
    GpuResourceManager manager(shaderDirectory);
    Check(manager.Initialize(), "manager initializes against the GL loader");

    const int pageWidth = 8;
    const int pageHeight = 8;
    const std::vector<unsigned char> pageSurface = MakePageSurface(pageWidth, pageHeight, 200);
    AtlasResidency residency("assetAtlasTest");

    Check(residency.PageCount() == 0, "a fresh residency owns no pages");
    Check(!residency.PageTexture(0).IsValid(), "an unuploaded page has no texture");
    Check(residency.UploadPage(manager, 0, pageWidth, pageHeight, pageSurface.data(), pageSurface.size()),
          "page 0 uploads through GpuResource_SYS");
    Check(residency.PageCount() == 1, "the page is now resident");
    Check(residency.PageTexture(0).IsValid(), "page 0 has a valid opaque texture handle");
    Check(residency.BindPage(manager, 0, 0), "the UI can bind the resident page to a sampler unit");
    Check(!residency.BindPage(manager, 1, 0), "binding a page that is not resident fails cleanly");

    std::vector<unsigned char> readback(pageSurface.size(), 0);
    manager.ReadbackTexture(residency.PageTexture(0), readback.data(), readback.size());
    Check(readback == pageSurface, "the atlas page arrived on the GPU byte-for-byte");

    // Re-uploading the same shape must reuse the texture rather than allocate another.
    const int reallocationsAfterFirstUpload = manager.TextureReallocationCount();
    Check(residency.UploadPage(manager, 0, pageWidth, pageHeight, pageSurface.data(), pageSurface.size()),
          "re-uploading page 0 succeeds");
    Check(manager.TextureReallocationCount() == reallocationsAfterFirstUpload,
          "a same-shape re-upload reallocated nothing");

    // Validation: a short buffer or a bad page index is refused, never uploaded (Constitution §6).
    Check(!residency.UploadPage(manager, 1, pageWidth, pageHeight, pageSurface.data(), 16),
          "a short surface is refused");
    Check(!residency.UploadPage(manager, -1, pageWidth, pageHeight, pageSurface.data(), pageSurface.size()),
          "a negative page index is refused");
    Check(!residency.UploadPage(manager, 1, 0, 0, pageSurface.data(), pageSurface.size()),
          "a zero-sized page is refused");
    Check(residency.PageCount() == 1, "no rejected upload left a page behind");

    residency.Clear();
    Check(residency.PageCount() == 0, "Clear drops the handle list");

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", FailureCount());
    return 1;
}
