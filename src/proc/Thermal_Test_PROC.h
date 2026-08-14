// Thermal_Test_PROC.h — the shared harness for the Thermal_PROC acceptance tests (M3-4).
// Layer: PROC (test support only — no product code includes this). Keeps the fixture, the field
// builders and the slope/mass measurements in one place so every test unit measures the SAME
// things, and so each test file stays inside the ARCH §1.5 ceiling.
#pragma once
#include "Thermal_PROC.h"
#include <cstdio>

namespace SanmapGen {
namespace ThermalTest {

constexpr int   testVertexSize        = 33;     // mapSize 32
constexpr float testTerrainMaxHeight  = 14.0f;  // puts a 35 degree talus near 0.05 height units
constexpr float testPeakHeight        = 0.9f;
constexpr float decoyAngleDegrees     = 5.0f;   // every stratum a test is NOT selecting
constexpr int   convergenceIterations = 900;
constexpr float slopeTolerance        = 1.0e-3f;

inline int& FailureCount() { static int failures = 0; return failures; }
inline void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++FailureCount(); }
}

inline Params::Geometry MakeGeometry(int vertexSize) {
    Params::Geometry geometry;
    geometry.mapSize = vertexSize - 1;
    geometry.terrainMaxHeight = testTerrainMaxHeight;
    return geometry;
}

// A single one-cell spike on flat ground — the classic talus test case.
inline void BuildSpikeField(Data::MapFields& fields, int vertexSize, float peakHeight) {
    fields.Resize(vertexSize, 0.0f);
    fields.heightfield.Set(vertexSize / 2, vertexSize / 2, peakHeight);
}

// A deterministic rough field with varied material coverage — exercises every kernel branch.
inline void BuildRoughField(Data::MapFields& fields, int vertexSize) {
    fields.Resize(vertexSize, 0.0f);
    unsigned int state = 2463534242u;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            state ^= state << 13; state ^= state >> 17; state ^= state << 5;
            fields.heightfield.Set(x, y, static_cast<float>(state % 1000u) * 0.001f);
            const int stratum = (x / 4 + y / 4) % Data::MapFields::stratumCount;
            fields.materialMasks[stratum].Set(x, y, 1.0f);
        }
}

// The steepest single-neighbour drop anywhere in the field: what the talus angle caps.
inline float MaximumNeighbourDrop(const Data::FloatField& field) {
    float maximumDrop = 0.0f;
    for (int y = 0; y < field.Height(); ++y)
        for (int x = 0; x < field.Width(); ++x)
            for (int step = 0; step < Proc::thermalNeighbourCount; ++step) {
                const int neighbourX = x + Proc::thermalNeighbourOffsetX[step];
                const int neighbourY = y + Proc::thermalNeighbourOffsetY[step];
                if (neighbourX < 0 || neighbourX >= field.Width()) continue;
                if (neighbourY < 0 || neighbourY >= field.Height()) continue;
                const float drop = field.Get(x, y) - field.Get(neighbourX, neighbourY);
                if (drop > maximumDrop) maximumDrop = drop;
            }
    return maximumDrop;
}

inline double TotalHeight(const Data::FloatField& field) {
    double total = 0.0;
    for (std::size_t cell = 0; cell < field.CellCount(); ++cell) total += field.Data()[cell];
    return total;
}

inline float MaximumFieldDifference(const Data::FloatField& left, const Data::FloatField& right) {
    float maximumDifference = 0.0f;
    for (std::size_t cell = 0; cell < left.CellCount(); ++cell) {
        float difference = left.Data()[cell] - right.Data()[cell];
        if (difference < 0.0f) difference = -difference;
        if (difference > maximumDifference) maximumDifference = difference;
    }
    return maximumDifference;
}

struct SpikeResult { float talusThreshold; float maximumDrop; double massDrift; };

// Relaxes a one-cell spike on the Cpu. stratumIndex < 0 leaves the masks empty (stratum-0
// fallback); otherwise the whole field is covered by that stratum, so only ITS angle may be used
// — every other stratum is set to the decoy angle.
inline SpikeResult RelaxSpike(float angleDegrees, int iterationCount, int stratumIndex) {
    Data::MapFields fields;
    BuildSpikeField(fields, testVertexSize, testPeakHeight);
    const int targetStratum = stratumIndex < 0 ? 0 : stratumIndex;
    if (stratumIndex >= 0)
        for (int y = 0; y < testVertexSize; ++y)
            for (int x = 0; x < testVertexSize; ++x)
                fields.materialMasks[stratumIndex].Set(x, y, 1.0f);

    const Params::Geometry geometry = MakeGeometry(testVertexSize);
    const double massBefore = TotalHeight(fields.heightfield);
    Proc::ThermalStage stage(geometry, fields);
    stage.Constants().iterationCount = iterationCount;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        stage.Constants().talusAngleDegrees[stratum] = decoyAngleDegrees;
    stage.Constants().talusAngleDegrees[targetStratum] = angleDegrees;
    stage.Run();

    return SpikeResult{ stage.ResolvedTalusThresholds()[targetStratum],
                        MaximumNeighbourDrop(fields.heightfield),
                        TotalHeight(fields.heightfield) - massBefore };
}

// The sibling test units.
void RunThermalParameterChecks();                              // Thermal_Params_PROC_Test.cpp
int  RunThermalGpuParityChecks(const char* shaderDirectory);   // Thermal_Gpu_PROC_Test.cpp

} // namespace ThermalTest
} // namespace SanmapGen
