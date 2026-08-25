// MarkerSymmetryFixCommand_UI.cpp — see MarkerSymmetryFixCommand_UI.h for the full contract.
#include "MarkerSymmetryFixCommand_UI.h"
#include "../pipeline/MarkerSymmetryDetection_PIPELINE.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Which real `(groupIndex, transformIndex)` a candidate pool entry (a plain offset into the
// caller's own arrays, per Pipeline::FindMarkerSymmetryMatches's own contract) came from.
struct CandidateOrigin {
    int groupIndex     = -1;
    int transformIndex = -1;
};

Params::MarkerTransform& TransformAt(std::vector<Params::MarkerInstanceGroup>& markers,
                                     const CandidateOrigin& origin) {
    return markers[static_cast<std::size_t>(origin.groupIndex)]
        .transforms[static_cast<std::size_t>(origin.transformIndex)];
}

// Step 1 (§3b): every `(groupIndex, transformIndex)` pair whose transform is on `layerIndex`,
// zeroing `symmetryGroupIdentifier` as it walks in overwrite mode, and returning the highest id seen
// AFTER that zeroing (so it is always 0 going in for overwrite mode). Sentinel is 0 here, NOT -1 —
// the divergence from `NextMarkerLayerId`'s own `-1` sentinel is deliberate (see the header).
std::vector<CandidateOrigin> CollectInLayerOrigins(std::vector<Params::MarkerInstanceGroup>& markers,
                                                   int layerIndex, bool bOverwrite, int& outMaximumId) {
    std::vector<CandidateOrigin> origins;
    outMaximumId = 0;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()); ++groupIndex) {
        std::vector<Params::MarkerTransform>& transforms =
            markers[static_cast<std::size_t>(groupIndex)].transforms;
        for (int transformIndex = 0; transformIndex < static_cast<int>(transforms.size()); ++transformIndex) {
            Params::MarkerTransform& transform = transforms[static_cast<std::size_t>(transformIndex)];
            if (transform.layerIndex != layerIndex) continue;
            if (bOverwrite) transform.symmetryGroupIdentifier = 0;
            origins.push_back(CandidateOrigin{ groupIndex, transformIndex });
            if (transform.symmetryGroupIdentifier > outMaximumId) outMaximumId = transform.symmetryGroupIdentifier;
        }
    }
    return origins;
}

// Step 2 (§3b): the parallel position/origin arrays `Pipeline::FindMarkerSymmetryMatches` consumes.
// Skip mode only includes still-ungrouped entries; overwrite mode includes every in-layer entry
// (all already zeroed by CollectInLayerOrigins above).
void BuildCandidatePool(const std::vector<Params::MarkerInstanceGroup>& markers,
                        const std::vector<CandidateOrigin>& inLayerOrigins, bool bOverwrite,
                        std::vector<float>& outPositionX, std::vector<float>& outPositionZ,
                        std::vector<CandidateOrigin>& outOrigin) {
    for (const CandidateOrigin& origin : inLayerOrigins) {
        const Params::MarkerTransform& transform =
            markers[static_cast<std::size_t>(origin.groupIndex)]
                .transforms[static_cast<std::size_t>(origin.transformIndex)];
        if (!bOverwrite && transform.symmetryGroupIdentifier != 0) continue;
        outPositionX.push_back(transform.transform.positionX);
        outPositionZ.push_back(transform.transform.positionZ);
        outOrigin.push_back(origin);
    }
}

// Step 4 (§3b): allocates one fresh id per confirmed match (pre-increment, so the first confirmed
// set in a fully-fresh layer gets 1) and writes it into every member's real MarkerTransform.
void WriteConfirmedMatches(std::vector<Params::MarkerInstanceGroup>& markers,
                           const std::vector<CandidateOrigin>& candidateOrigin,
                           const std::vector<Pipeline::MarkerSymmetryOrbitMatch>& matches,
                           int existingMaximumId, MarkerSymmetryFixResult& result) {
    for (const Pipeline::MarkerSymmetryOrbitMatch& match : matches) {
        const int nextId = ++existingMaximumId;
        TransformAt(markers, candidateOrigin[static_cast<std::size_t>(match.seedCandidateIndex)])
            .symmetryGroupIdentifier = nextId;
        for (int matchedIndex : match.matchedCandidateIndices)
            TransformAt(markers, candidateOrigin[static_cast<std::size_t>(matchedIndex)]).symmetryGroupIdentifier =
                nextId;
        ++result.confirmedGroupCount;
    }
}

} // namespace

MarkerSymmetryFixResult FixMarkerLayerSymmetry(std::vector<Params::MarkerInstanceGroup>& markers,
                                               const Params::Geometry& geometry, int layerIndex,
                                               int effectiveSymmetryMask, int effectiveRadialRepeatCount,
                                               float distanceTolerance, bool bOverwrite) {
    MarkerSymmetryFixResult result;
    int existingMaximumId = 0;
    const std::vector<CandidateOrigin> inLayerOrigins =
        CollectInLayerOrigins(markers, layerIndex, bOverwrite, existingMaximumId);

    std::vector<float> candidatePositionX;
    std::vector<float> candidatePositionZ;
    std::vector<CandidateOrigin> candidateOrigin;
    BuildCandidatePool(markers, inLayerOrigins, bOverwrite, candidatePositionX, candidatePositionZ,
                       candidateOrigin);

    const std::vector<Pipeline::MarkerSymmetryOrbitMatch> matches = Pipeline::FindMarkerSymmetryMatches(
        geometry, effectiveSymmetryMask, effectiveRadialRepeatCount, candidatePositionX.data(),
        candidatePositionZ.data(), static_cast<int>(candidatePositionX.size()), distanceTolerance,
        &result.unmatchedSlotCount);

    WriteConfirmedMatches(markers, candidateOrigin, matches, existingMaximumId, result);
    return result;
}

} // namespace Ui
} // namespace SanmapGen
