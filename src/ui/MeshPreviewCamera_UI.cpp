// MeshPreviewCamera_UI.cpp — see the header for the contract.
#include "MeshPreviewCamera_UI.h"
#include "../math/Trigonometry_MATH.h"
#include <cmath>

namespace SanmapGen {
namespace Ui {
namespace {

void Normalize3(float& x, float& y, float& z) {
    const float length = std::sqrt(x * x + y * y + z * z);
    if (length > 1e-8f) { x /= length; y /= length; z /= length; }
}
void Cross3(float ax, float ay, float az, float bx, float by, float bz,
           float& outX, float& outY, float& outZ) {
    outX = ay * bz - az * by;
    outY = az * bx - ax * bz;
    outZ = ax * by - ay * bx;
}

} // namespace

Mat4x4 MultiplyMat4x4(const Mat4x4& a, const Mat4x4& b) {
    Mat4x4 result;
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a.m[row * 4 + k] * b.m[k * 4 + column];
            result.m[row * 4 + column] = sum;
        }
    return result;
}

void TransformPointByMat4x4(const Mat4x4& matrix, float x, float y, float z,
                            float& outX, float& outY, float& outZ, float& outW) {
    const float* m = matrix.m;
    outX = m[0] * x + m[1] * y + m[2] * z + m[3];
    outY = m[4] * x + m[5] * y + m[6] * z + m[7];
    outZ = m[8] * x + m[9] * y + m[10] * z + m[11];
    outW = m[12] * x + m[13] * y + m[14] * z + m[15];
}

Mat4x4 BuildLookAtMatrix(float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ) {
    float forwardX = targetX - eyeX, forwardY = targetY - eyeY, forwardZ = targetZ - eyeZ;
    Normalize3(forwardX, forwardY, forwardZ);
    float rightX, rightY, rightZ;
    Cross3(forwardX, forwardY, forwardZ, 0.0f, 1.0f, 0.0f, rightX, rightY, rightZ);
    // Degenerate case: looking straight up/down along world +Y -- fall back to a world-Z up hint
    // so `right` never collapses to zero.
    if (rightX * rightX + rightY * rightY + rightZ * rightZ < 1e-8f)
        Cross3(forwardX, forwardY, forwardZ, 0.0f, 0.0f, 1.0f, rightX, rightY, rightZ);
    Normalize3(rightX, rightY, rightZ);
    float upX, upY, upZ;
    Cross3(rightX, rightY, rightZ, forwardX, forwardY, forwardZ, upX, upY, upZ);

    Mat4x4 result;
    result.m[0] = rightX;    result.m[1] = rightY;    result.m[2] = rightZ;
    result.m[3] = -(rightX * eyeX + rightY * eyeY + rightZ * eyeZ);
    result.m[4] = upX;       result.m[5] = upY;       result.m[6] = upZ;
    result.m[7] = -(upX * eyeX + upY * eyeY + upZ * eyeZ);
    result.m[8]  = -forwardX; result.m[9]  = -forwardY; result.m[10] = -forwardZ;
    result.m[11] = (forwardX * eyeX + forwardY * eyeY + forwardZ * eyeZ);
    result.m[12] = 0.0f; result.m[13] = 0.0f; result.m[14] = 0.0f; result.m[15] = 1.0f;
    return result;
}

Mat4x4 BuildPerspectiveMatrix(float verticalFieldOfViewRadians, float aspectRatio,
                              float nearPlane, float farPlane) {
    const float tanHalfFov = std::tan(verticalFieldOfViewRadians * 0.5f);
    Mat4x4 result;
    for (float& value : result.m) value = 0.0f;
    result.m[0]  = 1.0f / (aspectRatio * tanHalfFov);
    result.m[5]  = 1.0f / tanHalfFov;
    result.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    result.m[11] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    result.m[14] = -1.0f;
    return result;
}

void ComputeOrbitEyePosition(const MeshPreviewCameraState& camera,
                             float& outEyeX, float& outEyeY, float& outEyeZ) {
    const float cosinePitch = Math::Cosine(camera.pitchRadians);
    outEyeX = camera.targetX + camera.distance * cosinePitch * Math::Sine(camera.yawRadians);
    outEyeY = camera.targetY + camera.distance * Math::Sine(camera.pitchRadians);
    outEyeZ = camera.targetZ + camera.distance * cosinePitch * Math::Cosine(camera.yawRadians);
}

void FitCameraToBounds(MeshPreviewCameraState& camera, float minX, float minY, float minZ,
                       float maxX, float maxY, float maxZ) {
    camera.targetX = (minX + maxX) * 0.5f;
    camera.targetY = (minY + maxY) * 0.5f;
    camera.targetZ = (minZ + maxZ) * 0.5f;
    const float extentX = maxX - minX, extentY = maxY - minY, extentZ = maxZ - minZ;
    const float diagonal = std::sqrt(extentX * extentX + extentY * extentY + extentZ * extentZ);
    camera.distance = diagonal > 1e-4f ? diagonal * 1.5f : 1.0f;   // 1.0f floor: a point-sized mesh
}

} // namespace Ui
} // namespace SanmapGen
