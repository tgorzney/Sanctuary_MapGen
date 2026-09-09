// MarkersTab_TypeSectionBody_UI.h — STEP254: `DrawMarkerTypeSection`, extracted out of
// `DrawMarkersTab`'s own per-Type-section loop body (MarkersTab_UI.cpp's file-size-ceiling split) so
// `DrawMarkersTab` itself stays a thin Globals+Links+loop wrapper. Calls into the 4 sibling files the
// rest of this ticket relocates (MarkersTab_TypeSectionAddActions_UI.h,
// MarkersTab_TypeSectionPendingApply_UI.h, MarkersTab_TypeSectionHeaderButtons_UI.h,
// MarkersTab_BaseInstanceList_UI.h) plus the pre-existing DrawMarkerLayerBundleTree/
// DrawRuleLayerListBody/DrawManualMarkerLayerListBody/ApplyAddLinkAction/NotifyPlacementChange, every
// one of those call sites UNCHANGED, just relocated.
#pragma once
#include <functional>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState;
struct IconAtlasManifest;
class  IconAtlasPairingLookup;   // real declaration is `class`, not `struct` (IconAtlasPairing_UI.h)

// One Type-section's full collapsible body — header buttons, the Group/Layer/Instance tree, the
// pending-apply mutations the tree's own header-extra "X"/drop targets recorded this frame, the
// flat Rule/Manual-layer list bodies, and the base "no Layer" instance list. Extracted out of
// DrawMarkersTab's own per-Type-section loop body (STEP254) so DrawMarkersTab itself stays a thin
// Globals+Links+loop wrapper. `rowIndex`/`typeName` are the SAME loop variables the body used to
// close over inline.
void DrawMarkerTypeSection(Params::MapRecipe& recipe, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                           const IconAtlasPairingLookup* pairingLookup,
                           const Data::PlacementInstances* placedMarkers, int rowIndex, const char* typeName,
                           const std::function<void(int clickedInstanceIdentifier,
                                                    const std::vector<int>& selectedInstanceIdentifiers)>&
                               selectManualMarkerInstanceCallback,
                           const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                               selectProceduralMarkerInstanceCallback);

} // namespace Ui
} // namespace SanmapGen
