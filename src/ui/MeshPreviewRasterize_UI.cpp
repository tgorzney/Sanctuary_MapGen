// MeshPreviewRasterize_UI.cpp — see the header for the contract and the "why CPU, not GL" note.
#include "MeshPreviewRasterize_UI.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace SanmapGen {
namespace Ui {
namespace {

constexpr float kAmbientFloor = 0.15f;

struct ProjectedVertex {
    float screenX = 0.0f, screenY = 0.0f, ndcDepth = 0.0f;
    bool  bValid = false;   // false when the source vertex is behind the eye (clipW <= epsilon)
};

ProjectedVertex ProjectVertex(const Mat4x4& viewProjection, float x, float y, float z,
                              int viewportWidth, int viewportHeight) {
    ProjectedVertex projected;
    float clipX, clipY, clipZ, clipW;
    TransformPointByMat4x4(viewProjection, x, y, z, clipX, clipY, clipZ, clipW);
    if (clipW <= 1e-5f) return projected;   // behind the eye -- caller skips the whole triangle
    const float inverseW = 1.0f / clipW;
    const float ndcX = clipX * inverseW, ndcY = clipY * inverseW;
    projected.ndcDepth = clipZ * inverseW;
    projected.screenX  = (ndcX * 0.5f + 0.5f) * static_cast<float>(viewportWidth);
    projected.screenY  = (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(viewportHeight);
    projected.bValid   = true;
    return projected;
}

float EdgeFunction(float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void WritePixel(std::vector<unsigned char>& pixels, int viewportWidth, int pixelX, int pixelY,
               const unsigned char color[4], float intensity) {
    const std::size_t offset = (static_cast<std::size_t>(pixelY) * viewportWidth + pixelX) * 4;
    for (int channel = 0; channel < 3; ++channel)
        pixels[offset + channel] = static_cast<unsigned char>(color[channel] * intensity);
    pixels[offset + 3] = color[3];
}

// Flat shading intensity: abs(faceNormal . viewDirection), both computed in the mesh's own local
// space (the camera's eye/target already live there — see PropMeshPreview_UI.cpp) so no extra
// world-transform is needed for this debug view.
float FaceShadeIntensity(float ax, float ay, float az, float bx, float by, float bz,
                         float cx, float cy, float cz, float eyeX, float eyeY, float eyeZ) {
    float edge1X = bx - ax, edge1Y = by - ay, edge1Z = bz - az;
    float edge2X = cx - ax, edge2Y = cy - ay, edge2Z = cz - az;
    float normalX = edge1Y * edge2Z - edge1Z * edge2Y;
    float normalY = edge1Z * edge2X - edge1X * edge2Z;
    float normalZ = edge1X * edge2Y - edge1Y * edge2X;
    const float normalLength = std::sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ);
    if (normalLength < 1e-8f) return kAmbientFloor;   // degenerate (zero-area) triangle
    normalX /= normalLength; normalY /= normalLength; normalZ /= normalLength;

    const float centroidX = (ax + bx + cx) / 3.0f, centroidY = (ay + by + cy) / 3.0f, centroidZ = (az + bz + cz) / 3.0f;
    float viewX = eyeX - centroidX, viewY = eyeY - centroidY, viewZ = eyeZ - centroidZ;
    const float viewLength = std::sqrt(viewX * viewX + viewY * viewY + viewZ * viewZ);
    if (viewLength < 1e-8f) return 1.0f;
    viewX /= viewLength; viewY /= viewLength; viewZ /= viewLength;

    const float dot = std::fabs(normalX * viewX + normalY * viewY + normalZ * viewZ);
    return std::max(kAmbientFloor, dot);
}

void RasterizeTriangle(const ProjectedVertex& v0, const ProjectedVertex& v1, const ProjectedVertex& v2,
                       float intensity, const MeshPreviewRasterizeSettings& settings,
                       std::vector<float>& depthBuffer, std::vector<unsigned char>& outPixels) {
    const float area = EdgeFunction(v0.screenX, v0.screenY, v1.screenX, v1.screenY, v2.screenX, v2.screenY);
    if (std::fabs(area) < 1e-6f) return;   // degenerate in screen space

    const int minX = std::max(0, static_cast<int>(std::floor(std::min({ v0.screenX, v1.screenX, v2.screenX }))));
    const int maxX = std::min(settings.viewportWidth - 1,
                              static_cast<int>(std::ceil(std::max({ v0.screenX, v1.screenX, v2.screenX }))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({ v0.screenY, v1.screenY, v2.screenY }))));
    const int maxY = std::min(settings.viewportHeight - 1,
                              static_cast<int>(std::ceil(std::max({ v0.screenY, v1.screenY, v2.screenY }))));

    for (int pixelY = minY; pixelY <= maxY; ++pixelY) {
        for (int pixelX = minX; pixelX <= maxX; ++pixelX) {
            const float sampleX = pixelX + 0.5f, sampleY = pixelY + 0.5f;
            const float w0 = EdgeFunction(v1.screenX, v1.screenY, v2.screenX, v2.screenY, sampleX, sampleY);
            const float w1 = EdgeFunction(v2.screenX, v2.screenY, v0.screenX, v0.screenY, sampleX, sampleY);
            const float w2 = EdgeFunction(v0.screenX, v0.screenY, v1.screenX, v1.screenY, sampleX, sampleY);
            const bool bInside = (w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0);
            if (!bInside) continue;

            const float b0 = w0 / area, b1 = w1 / area, b2 = w2 / area;
            const float depth = b0 * v0.ndcDepth + b1 * v1.ndcDepth + b2 * v2.ndcDepth;
            const std::size_t depthIndex = static_cast<std::size_t>(pixelY) * settings.viewportWidth + pixelX;
            if (depth >= depthBuffer[depthIndex]) continue;   // something nearer already drawn here
            depthBuffer[depthIndex] = depth;
            WritePixel(outPixels, settings.viewportWidth, pixelX, pixelY, settings.meshColor, intensity);
        }
    }
}

} // namespace

void RasterizeMeshPreview(const Sys::SanmodelMesh& mesh, const MeshPreviewCameraState& camera,
                          const MeshPreviewRasterizeSettings& settings,
                          std::vector<unsigned char>& outRgba8Pixels) {
    const int width = std::max(1, settings.viewportWidth), height = std::max(1, settings.viewportHeight);
    outRgba8Pixels.assign(static_cast<std::size_t>(width) * height * 4, 0);
    for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(width) * height; ++pixel)
        for (int channel = 0; channel < 4; ++channel)
            outRgba8Pixels[pixel * 4 + channel] = settings.backgroundColor[channel];
    std::vector<float> depthBuffer(static_cast<std::size_t>(width) * height, std::numeric_limits<float>::max());

    float eyeX, eyeY, eyeZ;
    ComputeOrbitEyePosition(camera, eyeX, eyeY, eyeZ);
    const Mat4x4 view = BuildLookAtMatrix(eyeX, eyeY, eyeZ, camera.targetX, camera.targetY, camera.targetZ);
    const Mat4x4 projection = BuildPerspectiveMatrix(settings.verticalFieldOfViewRadians,
                                                     static_cast<float>(width) / static_cast<float>(height),
                                                     settings.nearPlane, settings.farPlane);
    const Mat4x4 viewProjection = MultiplyMat4x4(projection, view);

    const std::size_t vertexCount = mesh.positions.size() / 3;
    for (std::size_t triangle = 0; triangle + 2 < mesh.triangleIndices.size(); triangle += 3) {
        const std::uint32_t indexA = mesh.triangleIndices[triangle];
        const std::uint32_t indexB = mesh.triangleIndices[triangle + 1];
        const std::uint32_t indexC = mesh.triangleIndices[triangle + 2];
        if (indexA >= vertexCount || indexB >= vertexCount || indexC >= vertexCount) continue;

        const float* pa = &mesh.positions[indexA * 3];
        const float* pb = &mesh.positions[indexB * 3];
        const float* pc = &mesh.positions[indexC * 3];
        const ProjectedVertex v0 = ProjectVertex(viewProjection, pa[0], pa[1], pa[2], width, height);
        const ProjectedVertex v1 = ProjectVertex(viewProjection, pb[0], pb[1], pb[2], width, height);
        const ProjectedVertex v2 = ProjectVertex(viewProjection, pc[0], pc[1], pc[2], width, height);
        if (!v0.bValid || !v1.bValid || !v2.bValid) continue;   // behind the eye -- no near clip

        const float intensity = FaceShadeIntensity(pa[0], pa[1], pa[2], pb[0], pb[1], pb[2],
                                                    pc[0], pc[1], pc[2], eyeX, eyeY, eyeZ);
        RasterizeTriangle(v0, v1, v2, intensity, settings, depthBuffer, outRgba8Pixels);
    }
}

} // namespace Ui
} // namespace SanmapGen
