// Erosion_Spawn_PROC.cpp — droplet birth: rejection-sample the rain field until EXACTLY
// dropletCount drops exist (SIM_ALGORITHMS_SPEC "Spawn"). Layer: PROC (Cpu).
// The random stream is the shared integer hash driven by an explicit trial counter — not
// std::mt19937 + uniform_real_distribution, whose bit pattern is library-defined and so
// cannot be reproduced across machines (DETERMINISM_SPEC). The SAME spawn list is handed to
// both backends, so a Cpu/Gpu parity comparison starts from identical rain.
#include "Erosion_PROC.h"

namespace SanmapGen {
namespace Proc {

void ErosionStage::BuildDropletSpawns(int stratumIndex) {
    const ErosionLayerSettings& settings = layerSettings[stratumIndex];
    const int dropletCount = settings.dropletCount < 0 ? 0 : settings.dropletCount;
    dropletSpawns.clear();
    dropletSpawns.reserve(static_cast<std::size_t>(dropletCount) * 2);
    if (dropletCount == 0 || vertexSize < 4) return;

    // Normalise the rejection probability against the wettest cell.
    float rainMaximum = constants.rainMapMinimumMaximum;
    for (int cellIndex = 0; cellIndex < cellCount; ++cellIndex)
        if (rainMap[cellIndex] > rainMaximum) rainMaximum = rainMap[cellIndex];
    const float rainReciprocal = 1.0f / rainMaximum;

    const float lowestCoordinate = constants.boundaryMargin;
    const float coordinateSpan = static_cast<float>(vertexSize - 2) - lowestCoordinate;
    const unsigned int spawnSeed = static_cast<unsigned int>(static_cast<int>(geometry.seed)
                                                           + constants.spawnSeedOffset + stratumIndex);
    unsigned int trial = 0u;
    int rejectionRun = 0;
    while (static_cast<int>(dropletSpawns.size() / 2) < dropletCount) {
        const unsigned int trialSeed = HashRandomCombine(spawnSeed, trial);
        ++trial;
        const float positionX = lowestCoordinate + HashRandomUnitFloat(trialSeed) * coordinateSpan;
        const float positionY = lowestCoordinate + HashRandomUnitFloat(trialSeed + 1u) * coordinateSpan;
        const int cellIndex = static_cast<int>(positionY) * vertexSize + static_cast<int>(positionX);
        const float acceptance = rainMap[cellIndex] * rainReciprocal;
        const bool bForced = rejectionRun > constants.spawnRejectionSafetyLimit;   // rain field is dry
        if (HashRandomUnitFloat(trialSeed + 2u) <= acceptance || bForced) {
            dropletSpawns.push_back(positionX);
            dropletSpawns.push_back(positionY);
            rejectionRun = 0;
        } else {
            ++rejectionRun;
        }
    }
}

} // namespace Proc
} // namespace SanmapGen
