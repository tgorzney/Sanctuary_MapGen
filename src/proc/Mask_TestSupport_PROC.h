// Mask_TestSupport_PROC.h — shared scaffolding for the Mask stage acceptance test.
// Test-only: the deterministic inputs every aspect test runs against, the pass/fail counter,
// and the entry points of the aspect test units. Expected values inside the aspect tests are
// derived INDEPENDENTLY (plain tan/central-difference arithmetic written out), never by calling
// the kernel headers under test.
#pragma once
#include <cmath>
#include <cstdio>
#include "../data/MapFields_DATA.h"

namespace SanmapGen {
namespace MaskTest {

inline int& FailureCount() { static int failures = 0; return failures; }

inline void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++FailureCount(); }
}
inline void CheckNear(float actual, float expected, float tolerance, const char* label) {
    if (!(std::fabs(actual - expected) <= tolerance)) {
        std::printf("FAIL: %s (actual %.7f, expected %.7f, tolerance %.7f)\n",
                    label, actual, expected, tolerance);
        ++FailureCount();
    }
}

// A smoothly varying heightfield (normalized 0..1) whose gradient sweeps a wide slope range.
inline void FillTestHeightfield(Data::MapFields& fields, int vertexSize) {
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            fields.heightfield.Set(x, y, 0.5f + 0.25f * std::sin(static_cast<float>(x) * 0.11f)
                                                     * std::cos(static_cast<float>(y) * 0.07f)
                                             + 0.1f * static_cast<float>(x) / static_cast<float>(vertexSize - 1));
}

// A deterministic "already occluded" procedural mask set, as the noise/blend stage would leave it.
inline void FillTestProceduralMasks(Data::MapFields& fields, int vertexSize) {
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        for (int y = 0; y < vertexSize; ++y)
            for (int x = 0; x < vertexSize; ++x)
                fields.materialMasks[stratum].Set(x, y,
                    static_cast<float>((x * 7 + y * 13 + stratum * 29) % 100) * 0.01f);
}

void RunSlopeGateTests();   // Mask_Slope_PROC_Test.cpp
void RunMergeTests();       // Mask_Merge_PROC_Test.cpp
void RunDirtyHashTests();   // Mask_DirtyHash_PROC_Test.cpp
void RunParityTests(const char* shaderDirectory);   // Mask_Parity_PROC_Test.cpp

} // namespace MaskTest
} // namespace SanmapGen
