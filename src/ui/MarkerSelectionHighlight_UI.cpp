// MarkerSelectionHighlight_UI.cpp
#include "MarkerSelectionHighlight_UI.h"
#include "MarkerDragGesture_UI.h"                    // ResolveEffectiveMarkerSymmetry
#include "../params/Symmetry_PARAMS.h"                // symmetryOrbitMaximum
#include "../pipeline/SymmetryOrbitQuery_PIPELINE.h"   // BuildWorldSymmetryOrbit

namespace SanmapGen {
namespace Ui {

std::vector<int> ComputeManualMarkerSelectionHighlight(
        const std::vector<Params::MarkerInstanceGroup>& markers,
        const std::vector<Params::MarkerInstanceLayer>& markerLayers,
        const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
        float distanceTolerance, int selectedInstanceIdentifier) {
    std::vector<int> result;
    if (selectedInstanceIdentifier < 0) return result;

    int selectedGroupIndex = -1, selectedTransformIndex = -1;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()) && selectedGroupIndex < 0; ++groupIndex) {
        const std::vector<Params::MarkerTransform>& transforms =
            markers[static_cast<std::size_t>(groupIndex)].transforms;
        for (int transformIndex = 0; transformIndex < static_cast<int>(transforms.size()); ++transformIndex)
            if (transforms[static_cast<std::size_t>(transformIndex)].instanceIdentifier == selectedInstanceIdentifier) {
                selectedGroupIndex = groupIndex; selectedTransformIndex = transformIndex; break;
            }
    }
    if (selectedGroupIndex < 0) return result;   // stale identifier — Constitution §6, never a crash

    const Params::MarkerInstanceGroup& group = markers[static_cast<std::size_t>(selectedGroupIndex)];
    const Params::MarkerTransform& selected = group.transforms[static_cast<std::size_t>(selectedTransformIndex)];
    result.push_back(selected.instanceIdentifier);

    int effectiveMask = 0, effectiveRepeatCount = 0;
    ResolveEffectiveMarkerSymmetry(markerLayers, selected.layerIndex, globalSymmetryMask,
                                   globalRadialRepeatCount, effectiveMask, effectiveRepeatCount);

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(geometry, effectiveMask, effectiveRepeatCount,
        selected.transform.positionX, selected.transform.positionZ, orbitPoints, Params::symmetryOrbitMaximum);
    if (orbitCount <= 1) return result;   // no siblings — highlight only the selected instance

    const float toleranceSquared = distanceTolerance * distanceTolerance;
    for (int orbitIndex = 1; orbitIndex < orbitCount; ++orbitIndex) {
        const Pipeline::WorldSymmetryOrbitPoint& orbitPoint = orbitPoints[orbitIndex];
        for (int transformIndex = 0; transformIndex < static_cast<int>(group.transforms.size()); ++transformIndex) {
            if (transformIndex == selectedTransformIndex) continue;
            const Params::MarkerTransform& candidate = group.transforms[static_cast<std::size_t>(transformIndex)];
            const float deltaX = candidate.transform.positionX - orbitPoint.worldPositionX;
            const float deltaZ = candidate.transform.positionZ - orbitPoint.worldPositionZ;
            if (deltaX * deltaX + deltaZ * deltaZ <= toleranceSquared) {
                result.push_back(candidate.instanceIdentifier);
                break;   // first match wins — same tie posture as HitTestManualMarkers
            }
        }
    }
    return result;
}

std::vector<int> ComputeManualMarkerMultiSelectionHighlight(
        const std::vector<Params::MarkerInstanceGroup>& markers,
        const std::vector<Params::MarkerInstanceLayer>& markerLayers,
        const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
        float distanceTolerance, const std::vector<int>& selectedInstanceIdentifiers) {
    std::vector<int> result;
    for (int selectedInstanceIdentifier : selectedInstanceIdentifiers) {
        const std::vector<int> perInstance = ComputeManualMarkerSelectionHighlight(
            markers, markerLayers, geometry, globalSymmetryMask, globalRadialRepeatCount,
            distanceTolerance, selectedInstanceIdentifier);
        for (int identifier : perInstance) {
            bool bAlreadyPresent = false;
            for (int existing : result) if (existing == identifier) { bAlreadyPresent = true; break; }
            if (!bAlreadyPresent) result.push_back(identifier);
        }
    }
    return result;
}

} // namespace Ui
} // namespace SanmapGen
