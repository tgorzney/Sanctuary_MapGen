// RigidTransformPivot_MATH_Test.cpp — acceptance test for RigidTransformPivot_MATH (STEP120).
//   g++ -O2 -std=c++17 RigidTransformPivot_MATH_Test.cpp -o t && ./t
// Own main(), plain std::printf/exit-code, no imgui — mirrors Trigonometry_MATH_Test.cpp's shape.
#include "RigidTransformPivot_MATH.h"
#include <cmath>
#include <cstdio>

using namespace SanmapGen::Math;

namespace {

int failures = 0;
const float pi = 3.14159265358979323846f;
const float kTolerance = 1e-4f;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failures; }
}

bool NearlyEqual(float a, float b, float tolerance = kTolerance) {
    return std::fabs(a - b) <= tolerance;
}

void TestRotatePointAroundPivot() {
    // A point at (pivotX+1, pivotZ) rotated 90 degrees lands at (pivotX, pivotZ+1) — the
    // counter-clockwise convention matching AppendRadialTurns.
    const float pivotX = 3.0f, pivotZ = -2.0f;
    float outX = 0.0f, outZ = 0.0f;
    RotatePointAroundPivot(pivotX + 1.0f, pivotZ, pivotX, pivotZ, pi * 0.5f, outX, outZ);
    Check(NearlyEqual(outX, pivotX) && NearlyEqual(outZ, pivotZ + 1.0f),
         "RotatePointAroundPivot: 90 degrees lands at (pivotX, pivotZ+1)");

    // A 360 degree rotation returns to the original point within tolerance.
    const float startX = pivotX + 5.0f, startZ = pivotZ - 3.0f;
    RotatePointAroundPivot(startX, startZ, pivotX, pivotZ, 2.0f * pi, outX, outZ);
    Check(NearlyEqual(outX, startX) && NearlyEqual(outZ, startZ),
         "RotatePointAroundPivot: 360 degrees returns to the original point");

    // A point AT the pivot stays at the pivot for any angle.
    RotatePointAroundPivot(pivotX, pivotZ, pivotX, pivotZ, 1.23456f, outX, outZ);
    Check(NearlyEqual(outX, pivotX) && NearlyEqual(outZ, pivotZ),
         "RotatePointAroundPivot: a point at the pivot stays at the pivot");
}

void TestYawQuaternion() {
    float outX = 0.0f, outY = 0.0f, outZ = 0.0f, outW = 0.0f;
    YawQuaternion(0.0f, outX, outY, outZ, outW);
    Check(NearlyEqual(outX, 0.0f) && NearlyEqual(outY, 0.0f) && NearlyEqual(outZ, 0.0f) && NearlyEqual(outW, 1.0f),
         "YawQuaternion: angle 0 returns the identity quaternion");

    YawQuaternion(pi, outX, outY, outZ, outW);
    Check(NearlyEqual(outX, 0.0f) && NearlyEqual(outY, 1.0f) && NearlyEqual(outZ, 0.0f) && NearlyEqual(outW, 0.0f),
         "YawQuaternion: angle pi returns (0,1,0,0)");
}

void TestMultiplyQuaternions() {
    float outX = 0.0f, outY = 0.0f, outZ = 0.0f, outW = 0.0f;
    // Multiplying by the identity quaternion (as either operand) returns the other operand unchanged.
    const float qX = 0.1f, qY = 0.2f, qZ = 0.3f, qW = 0.9f;
    MultiplyQuaternions(qX, qY, qZ, qW, 0.0f, 0.0f, 0.0f, 1.0f, outX, outY, outZ, outW);
    Check(NearlyEqual(outX, qX) && NearlyEqual(outY, qY) && NearlyEqual(outZ, qZ) && NearlyEqual(outW, qW),
         "MultiplyQuaternions: identity as the second operand returns the first unchanged");
    MultiplyQuaternions(0.0f, 0.0f, 0.0f, 1.0f, qX, qY, qZ, qW, outX, outY, outZ, outW);
    Check(NearlyEqual(outX, qX) && NearlyEqual(outY, qY) && NearlyEqual(outZ, qZ) && NearlyEqual(outW, qW),
         "MultiplyQuaternions: identity as the first operand returns the second unchanged");

    // Multiplying two opposite 180 degree yaw quaternions returns the identity (within tolerance).
    float yawX = 0.0f, yawY = 0.0f, yawZ = 0.0f, yawW = 0.0f;
    YawQuaternion(pi, yawX, yawY, yawZ, yawW);
    float negativeYawX = 0.0f, negativeYawY = 0.0f, negativeYawZ = 0.0f, negativeYawW = 0.0f;
    YawQuaternion(-pi, negativeYawX, negativeYawY, negativeYawZ, negativeYawW);
    MultiplyQuaternions(yawX, yawY, yawZ, yawW, negativeYawX, negativeYawY, negativeYawZ, negativeYawW,
                       outX, outY, outZ, outW);
    // W may land at +1 or -1 (both represent the identity rotation); check both magnitudes.
    Check(NearlyEqual(outX, 0.0f) && NearlyEqual(outZ, 0.0f) && NearlyEqual(std::fabs(outW), 1.0f)
         && NearlyEqual(outY, 0.0f, 2e-4f),
         "MultiplyQuaternions: two opposite 180 degree yaws compose to the identity");
}

} // namespace

int main() {
    TestRotatePointAroundPivot();
    TestYawQuaternion();
    TestMultiplyQuaternions();
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
