// MarkerInstanceCreateSymmetric_UI.cpp — see the header's own comment for the full "why".
#include "MarkerInstanceCreateSymmetric_UI.h"
#include "MarkerDragGesture_UI.h"
#include "MarkerInstanceId_UI.h"
#include "MarkersTab_Manual_UI.h"
#include "UniqueNameList_UI.h"
#include "../pipeline/SymmetryOrbitQuery_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

int CreateSymmetricManualMarkerInstances(Params::MarkerInstanceGroup& group,
                                         const std::vector<Params::MarkerInstanceGroup>& markers,
                                         const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                         const Params::Geometry& geometry, int globalSymmetryMask,
                                         int globalRadialRepeatCount, int layerIndex,
                                         float worldX, float worldY, float worldZ) {
    int effectiveMask = 0, effectiveRadialRepeatCount = 0;
    ResolveEffectiveMarkerSymmetry(markerLayers, layerIndex, globalSymmetryMask, globalRadialRepeatCount,
                                   effectiveMask, effectiveRadialRepeatCount);

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(geometry, effectiveMask, effectiveRadialRepeatCount,
                                                             worldX, worldZ, orbitPoints,
                                                             Params::symmetryOrbitMaximum);
    // A 1-point orbit (symmetry off, or a mask/layer that resolves to none) stays ungrouped —
    // byte-identical to the pre-fix single-instance push_back.
    const int symmetryGroupIdentifier = orbitCount > 1 ? NextMarkerSymmetryGroupIdentifier(markers) : 0;

    int nextInstanceIdentifier = NextMarkerInstanceIdentifier(markers);
    int sourceInstanceIdentifier = -1;
    for (int slotIndex = 0; slotIndex < orbitCount; ++slotIndex) {
        Params::MarkerTransform materialized;
        materialized.name                    = NextMarkerInstanceName(static_cast<int>(group.transforms.size()));
        materialized.instanceIdentifier      = nextInstanceIdentifier++;
        materialized.transform.positionX     = orbitPoints[slotIndex].worldPositionX;
        materialized.transform.positionZ     = orbitPoints[slotIndex].worldPositionZ;
        materialized.transform.positionY     = worldY;
        materialized.layerIndex              = layerIndex;
        materialized.symmetryGroupIdentifier = symmetryGroupIdentifier;
        if (slotIndex == 0) sourceInstanceIdentifier = materialized.instanceIdentifier;
        group.transforms.push_back(materialized);
    }
    MakeNamesUnique(group.transforms);
    return sourceInstanceIdentifier;
}

} // namespace Ui
} // namespace SanmapGen
