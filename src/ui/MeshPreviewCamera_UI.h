// MeshPreviewCamera_UI.h — a minimal orbit camera + row-major 4x4 matrix math for the Props-tab
// mesh preview (PropMeshPreview_UI.h). Layer: UI. Deliberately NOT a general-purpose MATH
// primitive: this is authoring-time debug-viewport math (host-only, no determinism requirement —
// the Auto-NavMesh reference doc's own §2.4 ruling for one-shot, human-triggered tooling), not a
// simulation stage MATH would own. Uses Math::Sine/Cosine (not std::sin/cos) purely by convention
// with the rest of this codebase, not because determinism is required here.
#pragma once

namespace SanmapGen {
namespace Ui {

// Row-major 4x4: m[row*4+column].
struct Mat4x4 {
    float m[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
};

struct MeshPreviewCameraState {
    float yawRadians   = 0.7f;
    float pitchRadians = 0.35f;
    float distance     = 1.0f;                               // recomputed to fit the mesh once loaded
    float targetX = 0.0f, targetY = 0.0f, targetZ = 0.0f;    // mesh bounding-box center, local space
};

Mat4x4 MultiplyMat4x4(const Mat4x4& a, const Mat4x4& b);
void   TransformPointByMat4x4(const Mat4x4& matrix, float x, float y, float z,
                              float& outX, float& outY, float& outZ, float& outW);

// Right-handed look-at, +Y up convention — matches this mesh reader's own "no axis conversion"
// stance (SanmodelRead_SYS.h): the mesh is viewed exactly as its local-space Y-up data says.
Mat4x4 BuildLookAtMatrix(float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ);
Mat4x4 BuildPerspectiveMatrix(float verticalFieldOfViewRadians, float aspectRatio,
                              float nearPlane, float farPlane);

// Orbit eye position around `camera.target*` at `camera.distance`, driven by yaw/pitch.
void ComputeOrbitEyePosition(const MeshPreviewCameraState& camera,
                             float& outEyeX, float& outEyeY, float& outEyeZ);

// Recenters/redistances `camera` so a mesh occupying [minX..maxX]x[minY..maxY]x[minZ..maxZ] fits
// comfortably in view. Degenerates safely (a floor on distance) for a point-sized or empty box.
void FitCameraToBounds(MeshPreviewCameraState& camera, float minX, float minY, float minZ,
                       float maxX, float maxY, float maxZ);

} // namespace Ui
} // namespace SanmapGen
