// ManualInstanceDelete_UI.cpp — see ManualInstanceDelete_UI.h for the full rationale. Explicit
// instantiation for the three concrete manual-instance group types below is what lets this stay a
// normal, separately-compiled translation unit rather than header-only (mirrors
// ManualInstanceHitTest_UI.cpp exactly).
#include "ManualInstanceDelete_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"   // IsMarkerInstanceLocked
#include "PropsTab_Manual_UI.h"                 // IsPropInstanceLayerLocked
#include "DecalsTab_Manual_UI.h"                // IsDecalInstanceLayerLocked
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {

template<typename GroupT>
int DeleteManualInstancesById(std::vector<GroupT>& instances, const std::vector<int>& identifiers,
                              const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked) {
    if (identifiers.empty()) return 0;
    int erasedCount = 0;
    for (GroupT& group : instances) {
        auto& transforms = group.transforms;
        transforms.erase(
            std::remove_if(transforms.begin(), transforms.end(),
                           [&](const auto& transform) {
                               const bool bTargeted =
                                   std::find(identifiers.begin(), identifiers.end(),
                                            transform.instanceIdentifier) != identifiers.end();
                               if (!bTargeted) return false;
                               if (isInstanceLocked && isInstanceLocked(transform)) return false;
                               ++erasedCount;
                               return true;
                           }),
            transforms.end());
    }
    return erasedCount;
}

int DeleteSelectedManualMarkerInstances(std::vector<Params::MarkerInstanceGroup>& markers,
                                        const std::vector<int>& identifiers,
                                        const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                        const std::vector<Params::MarkerLink>& markerLinks) {
    return DeleteManualInstancesById(markers, identifiers,
        [&markerLayers, &markerLinks](const Params::MarkerTransform& transform) {
            return IsMarkerInstanceLocked(transform, markerLayers, markerLinks);
        });
}

int DeleteSelectedManualPropInstances(std::vector<Params::PropInstanceGroup>& props,
                                      const std::vector<int>& identifiers,
                                      const std::vector<Params::PropInstanceLayer>& propLayers) {
    return DeleteManualInstancesById(props, identifiers,
        [&propLayers](const Params::PropTransform& transform) {
            return IsPropInstanceLayerLocked(propLayers, transform.layerIndex);
        });
}

int DeleteSelectedManualDecalInstances(std::vector<Params::DecalInstanceGroup>& decals,
                                       const std::vector<int>& identifiers,
                                       const std::vector<Params::DecalInstanceLayer>& decalLayers) {
    return DeleteManualInstancesById(decals, identifiers,
        [&decalLayers](const Params::DecalTransform& transform) {
            return IsDecalInstanceLayerLocked(decalLayers, transform.layerIndex);
        });
}

template int DeleteManualInstancesById<Params::MarkerInstanceGroup>(
    std::vector<Params::MarkerInstanceGroup>&, const std::vector<int>&,
    const std::function<bool(const Params::MarkerTransform&)>&);
template int DeleteManualInstancesById<Params::PropInstanceGroup>(
    std::vector<Params::PropInstanceGroup>&, const std::vector<int>&,
    const std::function<bool(const Params::PropTransform&)>&);
template int DeleteManualInstancesById<Params::DecalInstanceGroup>(
    std::vector<Params::DecalInstanceGroup>&, const std::vector<int>&,
    const std::function<bool(const Params::DecalTransform&)>&);

} // namespace Ui
} // namespace SanmapGen
