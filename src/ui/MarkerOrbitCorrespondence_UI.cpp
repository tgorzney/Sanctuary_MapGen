// MarkerOrbitCorrespondence_UI.cpp — see MarkerOrbitCorrespondence_UI.h for the full rationale.
#include "MarkerOrbitCorrespondence_UI.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {
namespace {

struct OrbitMatchCandidate {
    float distanceSquared = 0.0f;
    int   entryIndex       = -1;
    int   slotIndex        = -1;
};

} // namespace

int MatchCorrespondenceToOrbit(std::vector<MarkerOrbitCorrespondence>& correspondence,
                               const Pipeline::WorldSymmetryOrbitPoint* orbitPoints, int orbitCount,
                               std::vector<int>* outUnclaimedSlots) {
    for (MarkerOrbitCorrespondence& entry : correspondence) entry.lastMatchedOrbitSlot = -1;
    if (orbitPoints == nullptr || orbitCount <= 1 || correspondence.empty()) {
        if (outUnclaimedSlots != nullptr)
            for (int slotIndex = 1; slotIndex < orbitCount; ++slotIndex) outUnclaimedSlots->push_back(slotIndex);
        return static_cast<int>(correspondence.size());
    }

    std::vector<OrbitMatchCandidate> candidates;
    candidates.reserve(correspondence.size() * static_cast<std::size_t>(orbitCount - 1));
    for (std::size_t entryIndex = 0; entryIndex < correspondence.size(); ++entryIndex) {
        const MarkerOrbitCorrespondence& entry = correspondence[entryIndex];
        for (int slotIndex = 1; slotIndex < orbitCount; ++slotIndex) {
            const float deltaX = orbitPoints[slotIndex].worldPositionX - entry.referenceWorldX;
            const float deltaZ = orbitPoints[slotIndex].worldPositionZ - entry.referenceWorldZ;
            candidates.push_back({deltaX * deltaX + deltaZ * deltaZ, static_cast<int>(entryIndex), slotIndex});
        }
    }
    std::sort(candidates.begin(), candidates.end(),
             [](const OrbitMatchCandidate& a, const OrbitMatchCandidate& b) {
                 return a.distanceSquared < b.distanceSquared;
             });

    std::vector<bool> entryClaimed(correspondence.size(), false);
    std::vector<bool> slotClaimed(static_cast<std::size_t>(orbitCount), false);
    int unmatchedCount = static_cast<int>(correspondence.size());
    for (const OrbitMatchCandidate& candidate : candidates) {
        if (entryClaimed[static_cast<std::size_t>(candidate.entryIndex)]
            || slotClaimed[static_cast<std::size_t>(candidate.slotIndex)]) continue;
        entryClaimed[static_cast<std::size_t>(candidate.entryIndex)] = true;
        slotClaimed[static_cast<std::size_t>(candidate.slotIndex)]  = true;
        correspondence[static_cast<std::size_t>(candidate.entryIndex)].lastMatchedOrbitSlot = candidate.slotIndex;
        --unmatchedCount;
    }
    if (outUnclaimedSlots != nullptr)
        for (int slotIndex = 1; slotIndex < orbitCount; ++slotIndex)
            if (!slotClaimed[static_cast<std::size_t>(slotIndex)]) outUnclaimedSlots->push_back(slotIndex);
    return unmatchedCount;
}

} // namespace Ui
} // namespace SanmapGen
