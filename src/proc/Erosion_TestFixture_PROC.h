// Erosion_TestFixture_PROC.h — the shared fixture for the erosion acceptance tests.
// Layer: PROC (test-only). Both Erosion_PROC_Test.cpp (Cpu) and Erosion_Gpu_PROC_Test.cpp
// (Cpu/Gpu parity) must start from the IDENTICAL terrain and the IDENTICAL settings, or a
// parity number means nothing — so the terrain builder and the stage configuration live here
// once instead of being copy-pasted and drifting apart.
#pragma once
#include "Erosion_PROC.h"
#include <cstdio>
#include <vector>

namespace SanmapGen {
namespace ErosionTest {

inline int& FailureCount() { static int failures = 0; return failures; }
inline void Check(bool bOk, const char* label) {
    if (!bOk) { std::printf("FAIL: %s\n", label); ++FailureCount(); }
}

// A deterministic bumpy cone: enough slope to erode, enough noise to braid. Soft topsoil
// (stratum 0) over a hard base (stratum 3), so differential hardness is exercised.
inline void BuildTestTerrain(Data::MapFields& fields, int vertexSize) {
    fields.Resize(vertexSize, 0.0f);
    const float halfSize = static_cast<float>(vertexSize - 1) * 0.5f;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            const float offsetX = (static_cast<float>(x) - halfSize) / halfSize;
            const float offsetY = (static_cast<float>(y) - halfSize) / halfSize;
            const float cone = 0.8f - 0.6f * (offsetX * offsetX + offsetY * offsetY);
            const float bump = Proc::HashRandomUnitFloat(Proc::HashRandomCombine(
                static_cast<unsigned int>(x / 4), static_cast<unsigned int>(y / 4))) * 0.08f;
            fields.heightfield.Set(x, y, cone + bump < 0.02f ? 0.02f : cone + bump);
            fields.materialProportions[0].Set(x, y, 0.7f);
            fields.materialProportions[3].Set(x, y, 0.3f);
        }
}

inline void ConfigureStage(Proc::ErosionStage& stage) {
    stage.Material(0).hardness = 0.1f;      // topsoil: erodes fast
    stage.Material(3).hardness = 0.85f;     // bedrock: barely moves
    Proc::ErosionLayerSettings& settings = stage.LayerSettings(0);
    settings.bEnabled = true;
    settings.dropletCount = 6000;
    settings.maximumLifetime = 30;
    settings.meanderStrength = 0.3f;
    settings.slopeAdherence = 0.6f;
    settings.bUseRainNoise = true;
}

inline std::vector<float> HeightfieldCopy(const Data::MapFields& fields) {
    const int vertexSize = fields.VertexSize();
    return std::vector<float>(fields.heightfield.Data(),
                              fields.heightfield.Data() + static_cast<std::size_t>(vertexSize) * vertexSize);
}

inline double TotalVolume(const std::vector<float>& heights) {
    double total = 0.0;
    for (float height : heights) total += height;
    return total;
}

} // namespace ErosionTest
} // namespace SanmapGen
