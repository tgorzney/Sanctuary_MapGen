// MarkersTab_TypeSectionAddHelpers_UI.cpp — see MarkersTab_TypeSectionAddHelpers_UI.h.
#include "MarkersTab_TypeSectionAddHelpers_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Ui {

Params::MarkerInstanceGroup& FindOrCreateMarkerInstanceGroupByName(
        std::vector<Params::MarkerInstanceGroup>& markers, const std::string& name) {
    for (Params::MarkerInstanceGroup& group : markers)
        if (Params::CanonicalMarkerTypeSectionName(group.name) == name) return group;
    Params::MarkerInstanceGroup group;
    group.name = name;
    markers.push_back(group);
    return markers.back();
}

float MapCenterWorldUnits(const Params::Geometry& geometry) {
    return static_cast<float>(geometry.mapSize) * geometry.worldUnitsPerGenerationCell * 0.5f;
}

int ResolveSelectedParentBundleIdentifier(const std::vector<Params::MarkerLayerBundle>& bundles,
                                          int selectedBundleIdentifier, const std::string& typeName) {
    if (selectedBundleIdentifier < 0) return -1;
    for (const Params::MarkerLayerBundle& bundle : bundles)
        if (bundle.identifier == selectedBundleIdentifier)
            return bundle.markerTypeName == typeName ? selectedBundleIdentifier : -1;
    return -1;
}

} // namespace Ui
} // namespace SanmapGen
