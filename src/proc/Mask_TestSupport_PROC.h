// Mask_TestSupport_PROC.h — shared scaffolding for the Mask stage acceptance test.
// Test-only: the deterministic inputs every aspect test runs against, the pass/fail counter,
// the field assertions more than one aspect needs, and the entry points of the aspect test
// units. Expected values inside the aspect tests are derived INDEPENDENTLY (plain tan /
// central-difference arithmetic written out), never by calling the kernel headers under test.
#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../data/MapFields_DATA.h"
#include "../data/StratumArt_DATA.h"

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

// A deterministic post-sim material-proportion field set, as the sim block would leave it.
inline void FillTestMaterialProportions(Data::MapFields& fields, int vertexSize) {
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        for (int y = 0; y < vertexSize; ++y)
            for (int x = 0; x < vertexSize; ++x)
                fields.materialProportions[stratum].Set(x, y,
                    static_cast<float>((x * 7 + y * 13 + stratum * 29) % 100) * 0.01f);
}

// No imported art at all — the common case (every stratum purely procedural).
inline std::vector<Data::StratumArt> NoStratumArt() {
    return std::vector<Data::StratumArt>(Data::MapFields::stratumCount);
}

// One stratum's imported art as a Data::FloatField (loaded pixels are DATA, ARCH §7.1).
inline void SetImportedMask(Data::StratumArt& art, const float* values, int width, int height) {
    art.importedMask.Resize(width, height, 0.0f);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) art.importedMask.Set(x, y, values[y * width + x]);
}

inline bool FieldsAreByteIdentical(const Data::FloatField& first, const Data::FloatField& second) {
    if (first.CellCount() != second.CellCount()) return false;
    return std::memcmp(first.Data(), second.Data(), first.CellCount() * sizeof(float)) == 0;
}

// ARCH §7.2.3, on whichever backend ran: the physical field must come out untouched.
inline void CheckProportionsUntouched(const Data::MapFields& processed,
                                      const Data::MapFields& reference, const char* label) {
    bool bUntouched = true;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        if (!FieldsAreByteIdentical(processed.materialProportions[stratum],
                                    reference.materialProportions[stratum])) bUntouched = false;
    Check(bUntouched, label);
}

// Guards every comparison against triviality: the resolved weights must differ from the
// proportions they came from and span a range instead of collapsing to a constant.
inline void CheckWeightsAreResolved(const Data::MapFields& fields, int vertexSize) {
    int changedCellCount = 0;
    float smallestWeight = 2.0f, largestWeight = -1.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        for (int y = 0; y < vertexSize; ++y)
            for (int x = 0; x < vertexSize; ++x) {
                const float weight = fields.surfaceStratumWeights[stratum].Get(x, y);
                if (std::fabs(weight - fields.materialProportions[stratum].Get(x, y)) > 1e-6f)
                    ++changedCellCount;
                if (weight < smallestWeight) smallestWeight = weight;
                if (weight > largestWeight) largestWeight = weight;
            }
    Check(changedCellCount > 1000, "the resolved weights really differ from the proportions");
    Check(largestWeight - smallestWeight > 0.5f, "the resolved weights span a range");
}

void RunSlopeGateTests();   // Mask_Slope_PROC_Test.cpp
void RunWorldScaleTests();  // Mask_WorldScale_PROC_Test.cpp
void RunMergeTests();       // Mask_Merge_PROC_Test.cpp
void RunPurityTests();      // Mask_Purity_PROC_Test.cpp
void RunDirtyHashTests();   // Mask_DirtyHash_PROC_Test.cpp
void RunParityTests(const char* shaderDirectory);   // Mask_Parity_PROC_Test.cpp

} // namespace MaskTest
} // namespace SanmapGen
