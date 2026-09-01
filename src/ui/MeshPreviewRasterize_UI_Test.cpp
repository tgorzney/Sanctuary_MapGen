// MeshPreviewRasterize_UI_Test.cpp — acceptance test for MeshPreviewRasterize_UI. Pure CPU, no GL
// context needed (this rasterizer never touches GL — see the header's "why CPU, not GL" note), so
// this runs as an ordinary console test.
#include "MeshPreviewRasterize_UI.h"

#include <cstddef>
#include <cstdio>
#include <vector>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// A camera-facing quad (two triangles) in the XY plane at z=0 -- normal (0,0,1).
Sys::SanmodelMesh MakeQuadMesh() {
    Sys::SanmodelMesh mesh;
    mesh.positions = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
    };
    mesh.triangleIndices = { 0, 1, 2,  0, 2, 3 };
    return mesh;
}

bool IsBackgroundPixel(const std::vector<unsigned char>& pixels, std::size_t pixelIndex,
                       const unsigned char background[4]) {
    return pixels[pixelIndex * 4 + 0] == background[0] && pixels[pixelIndex * 4 + 1] == background[1]
        && pixels[pixelIndex * 4 + 2] == background[2] && pixels[pixelIndex * 4 + 3] == background[3];
}

} // namespace

int main() {
    // Empty mesh: the whole buffer stays exactly the background color, correctly sized.
    {
        Ui::MeshPreviewRasterizeSettings settings;
        settings.viewportWidth = 64;
        settings.viewportHeight = 48;
        Ui::MeshPreviewCameraState camera;
        std::vector<unsigned char> pixels;
        Ui::RasterizeMeshPreview(Sys::SanmodelMesh(), camera, settings, pixels);
        Check(pixels.size() == static_cast<std::size_t>(64 * 48 * 4), "empty mesh: buffer sized width*height*4");
        bool bAllBackground = true;
        for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(64) * 48; ++pixel)
            bAllBackground = bAllBackground && IsBackgroundPixel(pixels, pixel, settings.backgroundColor);
        Check(bAllBackground, "empty mesh: every pixel is background");
    }

    // A camera-facing quad: the center pixel must differ from background (something was drawn),
    // and a corner far outside the quad's projection must stay background.
    {
        Ui::MeshPreviewRasterizeSettings settings;
        settings.viewportWidth = 128;
        settings.viewportHeight = 128;
        const Sys::SanmodelMesh mesh = MakeQuadMesh();
        Ui::MeshPreviewCameraState camera;
        Ui::FitCameraToBounds(camera, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        std::vector<unsigned char> pixels;
        Ui::RasterizeMeshPreview(mesh, camera, settings, pixels);

        const std::size_t centerPixel =
            static_cast<std::size_t>(settings.viewportHeight / 2) * settings.viewportWidth + settings.viewportWidth / 2;
        Check(!IsBackgroundPixel(pixels, centerPixel, settings.backgroundColor),
              "facing quad: center pixel differs from background");
        const std::size_t cornerPixel = 0;   // top-left corner, well outside the quad's projection
        Check(IsBackgroundPixel(pixels, cornerPixel, settings.backgroundColor),
              "facing quad: far corner stays background");
    }

    // A malformed mesh (an index pointing past the end of `positions`) must never crash -- reaching
    // this point at all is the proof; the malformed triangle is also expected to be skipped, leaving
    // the buffer entirely background since it is the ONLY triangle.
    {
        Ui::MeshPreviewRasterizeSettings settings;
        settings.viewportWidth = 32;
        settings.viewportHeight = 32;
        Sys::SanmodelMesh mesh;
        mesh.positions = { 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f };   // 3 vertices
        mesh.triangleIndices = { 0, 1, 5 };   // index 5 is out of range
        Ui::MeshPreviewCameraState camera;
        Ui::FitCameraToBounds(camera, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        std::vector<unsigned char> pixels;
        Ui::RasterizeMeshPreview(mesh, camera, settings, pixels);   // must not crash
        bool bAllBackground = true;
        for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(32) * 32; ++pixel)
            bAllBackground = bAllBackground && IsBackgroundPixel(pixels, pixel, settings.backgroundColor);
        Check(bAllBackground, "malformed index: the bad triangle is skipped, nothing drawn");
    }

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
