// MeshPreviewRasterize_UI.h — a software triangle rasterizer for the Props-tab mesh preview
// (PropMeshPreview_UI.h). Layer: UI.
//
// This is a CPU rasterizer into a plain RGBA8 pixel buffer, deliberately NOT a GL vertex/fragment
// draw call: this codebase's only established GPU pattern is compute-dispatch
// (Sys::GpuResourceManager / DISPATCH_INTERFACE_SPEC) — every existing GPU path here is a compute
// kernel writing an image, never a traditional raster pipeline. Inventing a new VAO/vertex-shader
// GPU resource kind in SYS for one debug viewport would be a materially new SYS capability; CPU
// rasterization into a plain byte buffer needs none of that; the existing
// `Sys::GpuResourceManager::EnsureTexture`/`UploadTexture` surface already knows how to host exactly
// this byte layout (GpuTextureFormat::Rgba8, tightly packed R,G,B,A per pixel) — see
// PropMeshPreview_UI.cpp for the upload/display side.
//
// No lighting model, no backface culling: the `.sanmodel` format's triangle winding convention is
// unconfirmed (SanmodelRead_SYS.h carries no normals to cross-check against), and culling the wrong
// direction would silently render nothing. Shading is `abs(faceNormal . viewDirection)` with a
// small ambient floor, and a depth buffer alone resolves per-pixel occlusion regardless of winding.
#pragma once
#include "MeshPreviewCamera_UI.h"
#include "../sys/SanmodelMesh_SYS.h"
#include <vector>

namespace SanmapGen {
namespace Ui {

struct MeshPreviewRasterizeSettings {
    int   viewportWidth  = 512;
    int   viewportHeight = 512;
    float verticalFieldOfViewRadians = 0.9f;   // ~52 degrees
    float nearPlane = 0.01f;
    float farPlane  = 1000.0f;
    unsigned char backgroundColor[4] = { 32, 32, 36, 255 };
    unsigned char meshColor[4]       = { 200, 200, 210, 255 };
};

// Sizes `outRgba8Pixels` to `settings.viewportWidth * viewportHeight * 4` and fills it: cleared to
// `backgroundColor`, then every triangle in `mesh` flat-shaded and depth-tested against `camera`'s
// current orbit position. Safe against a malformed mesh (an out-of-range index is skipped, never
// read out of bounds — Constitution §6).
void RasterizeMeshPreview(const Sys::SanmodelMesh& mesh, const MeshPreviewCameraState& camera,
                          const MeshPreviewRasterizeSettings& settings,
                          std::vector<unsigned char>& outRgba8Pixels);

} // namespace Ui
} // namespace SanmapGen
