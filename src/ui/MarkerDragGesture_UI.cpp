// MarkerDragGesture_UI.cpp — gesture-start (BeginMarkerDragGesture) and the roster-slider one-shot
// counterpart (RepositionSymmetryGroupMember). Per-frame update/release live in
// MarkerDragGesture_Frame_UI.cpp (ARCH §1.5 — one class of methods split across two files behind
// one header, the same posture MarkersTab_Manual_UI.h/.cpp + MarkersTab_ManualInstance_UI.cpp use).
#include "MarkerDragGesture_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Every sibling in `group` sharing `symmetryGroupIdentifier` (excluding `excludedTransformIndex`,
// the dragged/moved member itself), seeded with its OWN current position as the match reference —
// exact-value equality with the freshly-built orbit by the gesture-start proof (R2 §1).
std::vector<MarkerOrbitCorrespondence> BuildCorrespondenceSeed(const Params::MarkerInstanceGroup& group,
                                                                int symmetryGroupIdentifier,
                                                                int excludedTransformIndex) {
    std::vector<MarkerOrbitCorrespondence> correspondence;
    for (int index = 0; index < static_cast<int>(group.transforms.size()); ++index) {
        if (index == excludedTransformIndex) continue;
        const Params::MarkerTransform& sibling = group.transforms[static_cast<std::size_t>(index)];
        if (sibling.symmetryGroupIdentifier != symmetryGroupIdentifier) continue;
        MarkerOrbitCorrespondence entry;
        entry.transformIndex  = index;
        entry.referenceWorldX = sibling.transform.positionX;
        entry.referenceWorldZ = sibling.transform.positionZ;
        correspondence.push_back(entry);
    }
    return correspondence;
}

} // namespace

bool BeginMarkerDragGesture(MarkerDragGestureState& state,
                            const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const Params::Geometry& geometry, int globalSymmetryMask,
                            int globalRadialRepeatCount, int groupIndex, int transformIndex) {
    state = MarkerDragGestureState{};
    if (groupIndex < 0 || groupIndex >= static_cast<int>(markers.size())) return false;
    const Params::MarkerInstanceGroup& group = markers[static_cast<std::size_t>(groupIndex)];
    if (transformIndex < 0 || transformIndex >= static_cast<int>(group.transforms.size())) return false;
    const Params::MarkerTransform& dragged = group.transforms[static_cast<std::size_t>(transformIndex)];

    state.bActive               = true;
    state.groupIndex            = groupIndex;
    state.draggedTransformIndex = transformIndex;
    state.symmetryGroupIdentifier = dragged.symmetryGroupIdentifier;
    state.bSpawnGroup           = IsSpawnMarkerGroup(group);
    state.lastValidDraggedWorldX = dragged.transform.positionX;
    state.lastValidDraggedWorldZ = dragged.transform.positionZ;

    if (state.symmetryGroupIdentifier == 0) {          // ungrouped: free drag, zero orbit calls
        state.gestureStartOrbitCount = 1;
        state.lastValidOrbitCount    = 1;
        return true;
    }

    ResolveEffectiveMarkerSymmetry(markerLayers, dragged.layerIndex, globalSymmetryMask,
                                   globalRadialRepeatCount, state.effectiveSymmetryMask,
                                   state.effectiveRadialRepeatCount);

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(
        geometry, state.effectiveSymmetryMask, state.effectiveRadialRepeatCount,
        dragged.transform.positionX, dragged.transform.positionZ, orbitPoints, Params::symmetryOrbitMaximum);

    state.correspondence = BuildCorrespondenceSeed(group, state.symmetryGroupIdentifier, transformIndex);
    MatchCorrespondenceToOrbit(state.correspondence, orbitPoints, orbitCount, nullptr);
    state.gestureStartOrbitCount = orbitCount;
    state.lastValidOrbitCount    = orbitCount;
    return true;
}

bool RepositionSymmetryGroupMember(std::vector<Params::MarkerInstanceGroup>& markers,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount, int groupIndex,
                                   int movedTransformIndex, float newWorldX, float newWorldZ) {
    Params::MarkerInstanceGroup* const group = SelectedMarkerGroup(markers, groupIndex);
    if (group == nullptr) return false;
    Params::MarkerTransform* const dragged = SelectedMarkerInstance(group->transforms, movedTransformIndex);
    if (dragged == nullptr) return false;

    if (dragged->symmetryGroupIdentifier == 0) {
        dragged->transform.positionX = newWorldX; dragged->transform.positionZ = newWorldZ;
        return true;
    }

    int effectiveMask = 0, effectiveRadialRepeatCount = 0;
    ResolveEffectiveMarkerSymmetry(markerLayers, dragged->layerIndex, globalSymmetryMask,
                                   globalRadialRepeatCount, effectiveMask, effectiveRadialRepeatCount);

    Pipeline::WorldSymmetryOrbitPoint startOrbit[Params::symmetryOrbitMaximum];
    const int startCount = Pipeline::BuildWorldSymmetryOrbit(
        geometry, effectiveMask, effectiveRadialRepeatCount, dragged->transform.positionX,
        dragged->transform.positionZ, startOrbit, Params::symmetryOrbitMaximum);
    std::vector<MarkerOrbitCorrespondence> correspondence =
        BuildCorrespondenceSeed(*group, dragged->symmetryGroupIdentifier, movedTransformIndex);
    MatchCorrespondenceToOrbit(correspondence, startOrbit, startCount, nullptr);

    Pipeline::WorldSymmetryOrbitPoint newOrbit[Params::symmetryOrbitMaximum];
    const int newCount = Pipeline::BuildWorldSymmetryOrbit(geometry, effectiveMask, effectiveRadialRepeatCount,
                                                            newWorldX, newWorldZ, newOrbit, Params::symmetryOrbitMaximum);
    dragged->transform.positionX = newWorldX;
    dragged->transform.positionZ = newWorldZ;
    if (newCount != startCount) return false;    // no gesture to defer a create/delete to — refuse

    MatchCorrespondenceToOrbit(correspondence, newOrbit, newCount, nullptr);
    for (const MarkerOrbitCorrespondence& entry : correspondence) {
        if (entry.lastMatchedOrbitSlot < 0) continue;
        Params::MarkerTransform* const sibling = SelectedMarkerInstance(group->transforms, entry.transformIndex);
        if (sibling == nullptr) continue;
        sibling->transform.positionX = newOrbit[entry.lastMatchedOrbitSlot].worldPositionX;
        sibling->transform.positionZ = newOrbit[entry.lastMatchedOrbitSlot].worldPositionZ;
    }
    return true;
}

} // namespace Ui
} // namespace SanmapGen
