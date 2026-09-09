// MarkersTab_BaseInstanceList_UI.h — STEP254: `DrawBaseSectionManualInstanceList`, relocated
// verbatim out of MarkersTab_UI.cpp's file-size-ceiling split. Mirrors the sibling body-drawer
// pattern already established for the Rule/Manual layer lists (MarkersTab_RuleLayers_UI.h/.cpp,
// MarkersTab_ManualLayerRowBody_UI.h/.cpp).
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// STEP138/146 — this Type's own instances whose `layerIndex` does not resolve to any Layer OF THIS
// TYPE (no manual Layer exists yet for it, `-1` — genuinely unassigned, STEP146 — or a legacy/
// cross-type stale reference), rendered at the base of the section, after every Group and Layer,
// still indented under the collapsible Type-section. STEP146 (human's own bug report — dragging an
// instance here from a Layer did nothing) makes this list a real drop target too, reassigning to
// `layerIndex = -1`: this IS the one "no Layer" case the current data model can represent; an
// instance "in a Group but no Layer" (human's other stated case — a direct Group reference
// independent of any Layer) still needs a real PARAMS+IO field this ticket does not add (out of
// scope, flagged not guessed).
void DrawBaseSectionManualInstanceList(std::vector<Params::MarkerInstanceGroup>& markers,
                                       const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                       const std::string& typeName, int& selectedManualInstanceIdentifier,
                                       std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                       const std::function<void(int clickedInstanceIdentifier,
                                                                const std::vector<int>& selectedInstanceIdentifiers)>&
                                           selectManualMarkerInstanceCallback);

} // namespace Ui
} // namespace SanmapGen
